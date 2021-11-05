#include "file.hpp"

#include <cassert>
#include <cstdio>

#include <fcntl.h>

#include <algorithm>
#include <limits>
#include <system_error>

#include "utilities.hpp"

extern "C" {
#include "fuse.h"
} // extern "C"

using namespace multifs;

void File::init_desc(mode_t mode, int flags) noexcept
{
    auto const* ctx = fuse_get_context();
    assert(ctx);
    desc_.size      = 0;
    desc_.owner_uid = ctx->uid;
    desc_.owner_gid = ctx->gid;
    desc_.mode      = S_IFREG | mode;
    desc_.flags     = flags;
    desc_.atime     = current_time();
    desc_.mtime     = desc_.atime;
    desc_.ctime     = desc_.atime;
}

void File::truncate(size_t new_size) noexcept
{
    desc_.size  = new_size;
    desc_.ctime = current_time();
    desc_.mtime = desc_.ctime;
}

int File::unlink()
{
    for (auto const& chunk : chunks_)
        chunk.fs->unlink(path_.c_str());
    return 0;
}

int File::chmod(mode_t mode, struct fuse_file_info* fi) noexcept
{
    for (auto& chunk : chunks_) {
        if (auto const r = chunk.fs->chmod(path_.c_str(), mode, fi ? &chunk.fi : nullptr))
            return r;
    }
    desc_.mode  = S_IFREG | mode;
    desc_.ctime = current_time();
    return 0;
}

int File::chown(uid_t uid, gid_t gid, struct fuse_file_info* fi) noexcept
{
    for (auto& chunk : chunks_) {
        if (auto const r = chunk.fs->chown(path_.c_str(), uid, gid, fi ? &chunk.fi : nullptr))
            return r;
    }
    desc_.owner_uid = uid;
    desc_.owner_gid = gid;
    desc_.ctime     = current_time();
    return 0;
}

int File::truncate(size_t new_size, struct fuse_file_info* fi)
{
    for (auto& chunk : chunks_) {
        if (auto const r = chunk.fs->truncate(path_.c_str(), new_size, fi ? &chunk.fi : nullptr))
            return r;
    }
    truncate(new_size);
    return 0;
}

int File::open(struct fuse_file_info* fi)
{
    for (auto& chunk : chunks_) {
        if (fi) {
            chunk.fi.flags = desc_.flags = fi->flags;
            fi                           = &chunk.fi;
        }
        if (auto const r = chunk.fs->open(path_.c_str(), fi))
            return r;
    }
    if (fi && (fi->flags & O_TRUNC) && (fi->flags & (O_WRONLY | O_RDWR)))
        truncate(0);
    return 0;
}

int File::release(struct fuse_file_info* fi) noexcept
{
    for (auto& chunk : chunks_) {
        if (fi)
            fi = &chunk.fi;
        if (auto const r = chunk.fs->release(path_.c_str(), fi))
            return r;
    }
    desc_.flags = 0x0;
    return 0;
}

#ifdef HAVE_UTIMENSAT
int File::utimens(const struct timespec ts[2], struct fuse_file_info* fi) noexcept
{
    for (auto& chunk : chunks_) {
        if (fi)
            fi = &chunk.fi;
        chunk.fs->utimens(path_.c_str(), ts, fi);
    }

    auto const cur_time = current_time();

    if (UTIME_NOW == ts[0].tv_nsec)
        desc_.atime = cur_time;
    else if (UTIME_OMIT != ts[0].tv_nsec)
        desc_.atime = ts[0];

    if (UTIME_NOW == ts[1].tv_nsec)
        desc_.mtime = cur_time;
    else if (UTIME_OMIT != ts[1].tv_nsec)
        desc_.mtime = ts[1];

    if (UTIME_OMIT != ts[0].tv_nsec || UTIME_OMIT != ts[1].tv_nsec)
        desc_.ctime = cur_time;

    return 0;
}
#endif

#ifdef HAVE_POSIX_FALLOCATE
off_t File::fallocate(int mode, off_t offset, off_t length, struct fuse_file_info* fi)
{
    // TODO: implement the function
    return -EINVAL;
}
#endif

ssize_t File::write(char const* buf, size_t size, off_t offset, struct fuse_file_info* fi)
{
    ssize_t res = 0;

    size_t const initial_offset = offset = std::min(static_cast<size_t>(offset), desc_.size);

    for (auto chunk_it = std::upper_bound(chunks_.begin(), chunks_.end(), offset, [](off_t offset, auto const&chunk) { return offset < chunk.end_off; });
         0 != size && chunks_.end() != chunk_it;
         ++chunk_it) {
        if (fi) {
            chunk_it->fi.flags = fi->flags;
            fi                 = &chunk_it->fi;
        }

        auto const size_to_write = std::min(size, chunk_it->end_off - offset);

        auto const r = chunk_it->fs->write(path_.c_str(), buf, size_to_write, offset - chunk_it->start_off, fi);
        if (r < 0)
            return r;
        else if (size_to_write != r && chunk_it != std::prev(chunks_.end())) {
            return res;
        }

        res += r;
        offset += r;
        buf += r;
        size -= r;
    }

    while (0 != size && fs_next_it_ != fss_.end()) {
        chunks_.push_back(Chunk{static_cast<size_t>(offset), std::numeric_limits<size_t>::max(), {}, *fs_next_it_++});
        auto chunk_it = chunks_.end() - 1;
        if (chunks_.size() >= 2)
            chunk_it[-1].end_off = chunk_it->start_off;

        if (fi) {
            chunk_it->fi.flags = desc_.flags;
            fi                 = &chunk_it->fi;
        }

        if (chunk_it->fs->create(path_.c_str(), desc_.mode, fi)) {
            chunks_.erase(chunk_it);
            break;
        }

        if (fi) {
            chunk_it->fi.flags = fi->flags;
            fi                 = &chunk_it->fi;
        }

        auto const r = chunk_it->fs->write(path_.c_str(), buf, size, 0, fi);
        if (r < 0)
            return r;

        res += r;
        offset += r;
        buf += r;
        size -= r;
    }

    if (res > 0) {
        desc_.size = std::max(desc_.size, static_cast<decltype(desc_.size)>(initial_offset + res));
    }

    return res;
}

ssize_t File::read(char* buf, size_t size, off_t offset, struct fuse_file_info* fi) const noexcept
{
    ssize_t res = 0;

    offset = std::min(static_cast<size_t>(offset), desc_.size);
    size   = std::min(size, desc_.size - offset);

    for (auto chunk_it = std::upper_bound(chunks_.begin(), chunks_.end(), offset, [](off_t offset, auto const&chunk) { return offset < chunk.end_off; });
         0 != size && chunks_.end() != chunk_it;
         ++chunk_it) {
        if (fi) {
            chunk_it->fi.flags = fi->flags;
            fi                 = &chunk_it->fi;
        }
        auto const size_to_read = std::min(size, chunk_it->end_off - offset);
        auto const r            = chunk_it->fs->read(path_.c_str(), buf, size_to_read, offset - chunk_it->start_off, fi);
        if (r < 0)
            return r;
        res += r;
        offset += r;
        buf += r;
        size -= r;
        if (size_to_read != r)
            return res;
    }

    return res;
}

off_t File::lseek(off_t off, int whence, struct fuse_file_info* /*fi*/) const noexcept
{
    switch (whence) {
        case SEEK_DATA:
            return off;
        case SEEK_HOLE:
            return desc_.size;
        default:
            return -EINVAL;
    }
}
