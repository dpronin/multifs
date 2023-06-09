#include "file.hpp"

#include <cassert>
#include <cstdio>

#include <fcntl.h>

#include <algorithm>
#include <limits>
#include <system_error>

#include <fuse.h>

#include "utilities.hpp"


using namespace multifs;

void File::init_desc(mode_t mode, struct fuse_file_info* fi) noexcept
{
    auto const* ctx = fuse_get_context();
    assert(ctx);
    desc_.size      = 0;
    desc_.owner_uid = ctx->uid;
    desc_.owner_gid = ctx->gid;
    desc_.mode      = S_IFREG | mode;
    if (fi && 0x0 == fi->fh) {
        fi->fh = reinterpret_cast<uintptr_t>(new std::vector<fuse_file_info>{fss_.size(), *fi});
    }
    desc_.atime = current_time();
    desc_.mtime = desc_.atime;
    desc_.ctime = desc_.atime;
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
    for (int i = 0; i < chunks_.size(); ++i) {
        fuse_file_info mfi{};
        if (fi && fi->fh) {
            auto* v = reinterpret_cast<std::vector<fuse_file_info>*>(fi->fh);
            mfi.fh  = (*v)[i].fh;
        }
        if (auto const r = chunks_[i].fs->chmod(path_.c_str(), mode, fi ? &mfi : nullptr))
            return r;
    }
    desc_.mode  = S_IFREG | mode;
    desc_.ctime = current_time();
    return 0;
}

int File::chown(uid_t uid, gid_t gid, struct fuse_file_info* fi) noexcept
{
    for (int i = 0; i < chunks_.size(); ++i) {
        fuse_file_info mfi{};
        if (fi && fi->fh) {
            auto* v = reinterpret_cast<std::vector<fuse_file_info>*>(fi->fh);
            mfi.fh  = (*v)[i].fh;
        }
        if (auto const r = chunks_[i].fs->chown(path_.c_str(), uid, gid, fi ? &mfi : nullptr))
            return r;
    }
    desc_.owner_uid = uid;
    desc_.owner_gid = gid;
    desc_.ctime     = current_time();
    return 0;
}

int File::truncate(size_t new_size, struct fuse_file_info* fi)
{
    for (int i = 0; i < chunks_.size(); ++i) {
        fuse_file_info mfi{};
        if (fi && fi->fh) {
            auto* v = reinterpret_cast<std::vector<fuse_file_info>*>(fi->fh);
            mfi.fh  = (*v)[i].fh;
        }
        if (auto const r = chunks_[i].fs->truncate(path_.c_str(), new_size, fi ? &mfi : nullptr))
            return r;
    }
    return 0;
}

int File::open(struct fuse_file_info* fi)
{
    std::vector<fuse_file_info>* v{nullptr};
    if (fi) {
        if (0x0 == fi->fh)
            fi->fh = reinterpret_cast<uintptr_t>(new std::vector<fuse_file_info>{fss_.size(), *fi});
        v = reinterpret_cast<std::vector<fuse_file_info>*>(fi->fh);
    }
    for (int i = 0; i < chunks_.size(); ++i) {
        fuse_file_info mfi{};
        if (fi)
            mfi.flags = (*v)[i].flags;
        if (auto const r = chunks_[i].fs->open(path_.c_str(), fi ? &mfi : nullptr))
            return r;
        if (v)
            (*v)[i].fh = mfi.fh;
    }
    if (fi && (fi->flags & O_TRUNC) && (fi->flags & (O_WRONLY | O_RDWR)))
        truncate(0);
    return 0;
}

int File::release(struct fuse_file_info* fi) noexcept
{
    for (int i = 0; i < chunks_.size(); ++i) {
        fuse_file_info mfi{};
        if (fi && fi->fh) {
            auto* v = reinterpret_cast<std::vector<fuse_file_info>*>(fi->fh);
            mfi.fh  = (*v)[i].fh;
        }
        if (auto const r = chunks_[i].fs->release(path_.c_str(), fi ? &mfi : nullptr))
            return r;
    }
    if (fi && fi->fh) {
        delete reinterpret_cast<std::vector<fuse_file_info>*>(fi->fh);
        fi->fh = 0x0;
    }
    return 0;
}

#ifdef HAVE_UTIMENSAT
int File::utimens(const struct timespec ts[2], struct fuse_file_info* fi) noexcept
{
    for (int i = 0; i < chunks_.size(); ++i) {
        fuse_file_info mfi{};
        if (fi && fi->fh) {
            auto* v = reinterpret_cast<std::vector<fuse_file_info>*>(fi->fh);
            mfi.fh  = (*v)[i].fh;
        }
        chunks_[i].fs->utimens(path_.c_str(), ts, fi ? &mfi : nullptr);
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

    std::vector<fuse_file_info>* v{nullptr};
    if (fi && 0 != fi->fh)
        v = reinterpret_cast<std::vector<fuse_file_info>*>(fi->fh);

    for (auto chunk_it = std::upper_bound(chunks_.begin(), chunks_.end(), offset, [](off_t offset, auto const& chunk) { return offset < chunk.end_off; });
         0 != size && chunks_.end() != chunk_it;
         ++chunk_it) {

        fuse_file_info mfi{};
        if (v) {
            mfi.fh    = (*v)[chunk_it - chunks_.begin()].fh;
            mfi.flags = fi->flags;
        }

        auto const size_to_write = std::min(size, chunk_it->end_off - offset);

        auto const r = chunk_it->fs->write(path_.c_str(), buf, size_to_write, offset - chunk_it->start_off, fi ? &mfi : nullptr);
        if (r < 0) {
            if (-ENOSPC == r)
                break;
            return r;
        } else if (size_to_write != r && chunk_it != std::prev(chunks_.end())) {
            return res;
        }

        res += r;
        offset += r;
        buf += r;
        size -= r;
    }

    while (0 != size && fs_next_it_ != fss_.end()) {
        chunks_.push_back(Chunk{static_cast<size_t>(offset), std::numeric_limits<size_t>::max(), *fs_next_it_++});
        auto chunk_it = chunks_.end() - 1;
        if (chunks_.size() >= 2)
            chunk_it[-1].end_off = chunk_it->start_off;

        fuse_file_info mfi{};
        if (v)
            mfi.flags = (*v)[chunk_it - chunks_.begin()].flags;

        if (chunk_it->fs->create(path_.c_str(), desc_.mode, fi ? &mfi : nullptr)) {
            chunks_.erase(chunk_it);
            break;
        }

        if (v)
            (*v)[chunk_it - chunks_.begin()].fh = mfi.fh;

        if (fi)
            mfi.flags = fi->flags;

        auto const r = chunk_it->fs->write(path_.c_str(), buf, size, 0, fi ? &mfi : nullptr);
        if (r < 0) {
            if (-ENOSPC == r)
                continue;
            return r;
        }

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

    std::vector<fuse_file_info>* v;
    if (fi && 0 != fi->fh)
        v = reinterpret_cast<std::vector<fuse_file_info>*>(fi->fh);

    for (auto chunk_it = std::upper_bound(chunks_.begin(), chunks_.end(), offset, [](off_t offset, auto const& chunk) { return offset < chunk.end_off; });
         0 != size && chunks_.end() != chunk_it;
         ++chunk_it) {

        fuse_file_info mfi{};
        if (v) {
            mfi.fh    = (*v)[chunk_it - chunks_.begin()].fh;
            mfi.flags = fi->flags;
        }

        auto const size_to_read = std::min(size, chunk_it->end_off - offset);
        auto const r            = chunk_it->fs->read(path_.c_str(), buf, size_to_read, offset - chunk_it->start_off, fi ? &mfi : nullptr);
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

int File::fsync(int isdatasync, struct fuse_file_info* fi) noexcept
{
    for (auto const& chunk : chunks_) {
        if (auto const r = chunk.fs->fsync(path_.c_str(), isdatasync, nullptr))
            return r;
    }
    return 0;
}
