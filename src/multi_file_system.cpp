#include "multi_file_system.hpp"

#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <ctime>

#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <filesystem>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <variant>

#include "inode_chmodder.hpp"
#include "inode_chowner.hpp"
#include "inode_fsyncer.hpp"
#include "inode_link_reader.hpp"
#include "inode_lseeker.hpp"
#include "inode_opener.hpp"
#include "inode_reader.hpp"
#include "inode_releaser.hpp"
#include "inode_truncater.hpp"
#include "inode_unlinker.hpp"
#include "inode_writer.hpp"
#ifdef HAVE_UTIMENSAT
#include "inode_utimenser.hpp"
#endif
#ifdef HAVE_POSIX_FALLOCATE
#include "inode_fallocater.hpp"
#endif
#include "utilities.hpp"

using namespace multifs;

namespace
{

INodeUnlinker __unlinker__;

// constexpr unsigned long kFUSESuperMagic = 0x65735546;
constexpr unsigned long kBlockSize = 4 * 1024UL;
constexpr unsigned long kMaxName   = 255;
// constexpr unsigned long kMaxBlocks      = 1024 * 1024UL;
// constexpr unsigned long kMaxInode       = 1024 * 1024UL;
constexpr unsigned long kFSID = 0x0123456789098765;

} // anonymous namespace

void MultiFileSystem::statvs_init() noexcept
{
    statvfs_ = {
        .f_bsize  = kBlockSize, // Filesystem block size
        .f_frsize = kBlockSize, // Fragment size

        .f_blocks = {} /*kMaxBlocks*/, // Size of fs in f_frsize units
        .f_bfree  = {} /*kMaxBlocks*/, // Number of free blocks
        .f_bavail = {} /*kMaxBlocks*/, // Number of free blocks for unprivileged users

        .f_files  = {} /*kMaxInode*/, // Number of inodes
        .f_ffree  = {} /*kMaxInode*/, // Number of free inodes
        .f_favail = {} /*kMaxInode*/, // Number of free inodes for unprivileged users

        .f_fsid    = kFSID,    // Filesystem ID
        .f_namemax = kMaxName, // Maximum filename length
    };
}

int MultiFileSystem::getattr(char const* path, struct stat* stbuf, struct fuse_file_info* /*fi*/) const noexcept
{
    std::memset(stbuf, 0, sizeof(struct stat));

    if (std::strcmp(path, "/") == 0) {
        stbuf->st_uid   = owner_uid_;
        stbuf->st_gid   = owner_gid_;
        stbuf->st_mode  = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        stbuf->st_ctime = stbuf->st_atime = stbuf->st_mtime = std::time(NULL);
    } else if (auto const it = inodes_.find(path); inodes_.end() != it) {
        stbuf->st_nlink = it->second.use_count();
        std::visit(
            [stbuf](auto const& item) {
                using T = std::decay_t<decltype(item)>;
                if constexpr (std::is_same_v<T, File>) {
                    auto const& fdesc = item.desc();
                    stbuf->st_size    = fdesc.size;
                    stbuf->st_mode    = fdesc.mode;
                    stbuf->st_uid     = fdesc.owner_uid;
                    stbuf->st_gid     = fdesc.owner_gid;
                    stbuf->st_atim    = fdesc.atime;
                    stbuf->st_mtim    = fdesc.mtime;
                    stbuf->st_ctim    = fdesc.ctime;
                } else if constexpr (std::is_same_v<T, Symlink>) {
                    stbuf->st_size    = item.target().string().size();
                    auto const& ldesc = item.desc();
                    stbuf->st_mode    = ldesc.mode;
                    stbuf->st_uid     = ldesc.owner_uid;
                    stbuf->st_gid     = ldesc.owner_gid;
                    stbuf->st_atim    = ldesc.atime;
                    stbuf->st_mtim    = ldesc.mtime;
                    stbuf->st_ctim    = ldesc.ctime;
                } else {
                    static_assert(dependent_false_v<T>, "unhandled type");
                }
            },
            *it->second);
    } else {
        return -ENOENT;
    }

    return 0;
}

int MultiFileSystem::readlink(char const* path, char* buf, size_t size) const noexcept
{
    auto it = inodes_.find(path);
    if (inodes_.end() == it)
        return -ENOENT;
    return std::visit(INodeLinkReader{buf, size}, *it->second);
}

int MultiFileSystem::mknod(char const* path, mode_t mode, dev_t rdev)
{
    // TODO: implement the function
    return -EINVAL;
}

int MultiFileSystem::mkdir(char const* path, mode_t mode)
{
    // TODO: implement the function
    return -EINVAL;
}

int MultiFileSystem::rmdir(char const* path)
{
    // TODO: implement the function
    return -EINVAL;
}

int MultiFileSystem::symlink(char const* from, char const* to) { return inodes_.emplace(to, std::make_shared<INode>(Symlink{from})).second ? 0 : -EEXIST; }

int MultiFileSystem::rename(char const* from, char const* to, unsigned int flags)
{
    auto from_it = inodes_.find(from);
    if (inodes_.end() == from_it)
        return -ENOENT;

    if (auto to_it = inodes_.find(to); flags & RENAME_NOREPLACE) {
        if (inodes_.end() != to_it)
            return -EEXIST;
        auto node  = inodes_.extract(from_it);
        node.key() = to;
        inodes_.insert(std::move(node));
    } else if (flags & RENAME_EXCHANGE) {
        if (inodes_.end() == to_it)
            return -ENOENT;
        using std::swap;
        swap(from_it->second, to_it->second);
    } else if (inodes_.end() != to_it) {
        to_it->second = std::move(from_it->second);
        inodes_.erase(from_it);
    } else {
        inodes_.emplace_hint(to_it, to, std::move(from_it->second));
    }

    return 0;
}

int MultiFileSystem::link(char const* from, char const* to)
{
    auto from_it = inodes_.find(from);
    if (inodes_.end() == from_it)
        return -ENOENT;
    return inodes_.emplace(to, from_it->second).second ? 0 : -EEXIST;
}

void* MultiFileSystem::init(struct fuse_conn_info* /*conn*/, struct fuse_config* cfg) noexcept
{
    cfg->kernel_cache = 1;
    return NULL;
}

int MultiFileSystem::access(char const* path, int mask) const noexcept
{
    if (std::strcmp(path, "/") == 0 || std::strcmp(path + 1, ".") == 0 || std::strcmp(path + 1, "..") == 0 || inodes_.count(path))
        return 0;
    return -ENOENT;
}

int MultiFileSystem::readdir(
    char const* path, void* buf, fuse_fill_dir_t filler, off_t /*offset*/, struct fuse_file_info* /*fi*/, fuse_readdir_flags /*flags*/) const
{
    if (std::strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0, static_cast<fuse_fill_dir_flags>(0));
    filler(buf, "..", NULL, 0, static_cast<fuse_fill_dir_flags>(0));

    for (auto const& [path, _] : inodes_) {
        auto name = std::string_view{path};
        name.remove_prefix(1);
        filler(buf, name.data(), NULL, 0, static_cast<fuse_fill_dir_flags>(0));
    }

    return 0;
}

int MultiFileSystem::unlink(char const* path)
{
    if (std::strcmp(path, "/") == 0 || std::strcmp(path + 1, ".") == 0 || std::strcmp(path + 1, "..") == 0)
        return -EBUSY;

    auto it = inodes_.find(path);
    if (inodes_.end() == it)
        return -ENOENT;

    auto const inode = std::move(it->second);
    inodes_.erase(it);
    return std::visit(__unlinker__, *inode);
}

int MultiFileSystem::chmod(char const* path, mode_t mode, struct fuse_file_info* fi) noexcept
{
    auto it = inodes_.find(path);
    if (inodes_.end() == it)
        return -ENOENT;
    return std::visit(INodeChmodder{mode, fi}, *it->second);
}

int MultiFileSystem::chown(char const* path, uid_t uid, gid_t gid, struct fuse_file_info* fi) noexcept
{
    auto it = inodes_.find(path);
    if (inodes_.end() == it)
        return -ENOENT;
    return std::visit(INodeChowner{uid, gid, fi}, *it->second);
}

int MultiFileSystem::truncate(char const* path, off_t size, struct fuse_file_info* fi)
{
    auto it = inodes_.find(path + 1);
    if (inodes_.end() == it)
        return -ENOENT;
    return std::visit(INodeTruncater{size, fi}, *it->second);
}

int MultiFileSystem::open(char const* path, struct fuse_file_info* fi)
{
    auto it = inodes_.find(path);
    if (inodes_.end() == it)
        return -ENOENT;
    return std::visit(INodeOpener{fi}, *it->second);
}

int MultiFileSystem::create(char const* path, mode_t mode, struct fuse_file_info* fi)
{
    return inodes_.emplace(path, std::make_shared<INode>(File{std::string{path} + ".chunk", mode, fss_.begin(), fss_.end(), fi})).second ? 0 : -EEXIST;
}

ssize_t MultiFileSystem::read(char const* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi) const noexcept
{
    auto const it = inodes_.find(path);
    if (inodes_.end() == it)
        return -ENOENT;
    return std::visit(INodeReader{buf, size, offset, fi}, *it->second);
}

ssize_t MultiFileSystem::write(char const* path, char const* buf, size_t size, off_t offset, struct fuse_file_info* fi)
{
    auto it = inodes_.find(path);
    if (inodes_.end() == it)
        return -ENOENT;
    return std::visit(INodeWriter{buf, size, offset, fi}, *it->second);
}

int MultiFileSystem::statfs(char const* path, struct statvfs* stbuf) const noexcept
{
    *stbuf = statvfs_;
    for (auto const& fs : fss_) {
        // clang-format off
        struct statvfs stbuf_leaf {};
        // clang-format on
        if (auto const res = fs->statfs(path, &stbuf_leaf))
            return res;
        stbuf->f_blocks += stbuf_leaf.f_blocks * stbuf_leaf.f_bsize / stbuf->f_bsize;
        stbuf->f_bfree += stbuf_leaf.f_bfree * stbuf_leaf.f_bsize / stbuf->f_bsize;
        stbuf->f_bavail += stbuf_leaf.f_bavail * stbuf_leaf.f_bsize / stbuf->f_bsize;
        stbuf->f_files += stbuf_leaf.f_files;
        stbuf->f_ffree += stbuf_leaf.f_ffree;
        stbuf->f_favail += stbuf_leaf.f_favail;
    }
    return 0;
}

int MultiFileSystem::release(char const* path, struct fuse_file_info* fi) noexcept
{
    auto it = inodes_.find(path);
    if (inodes_.end() == it)
        return -ENOENT;
    return std::visit(INodeReleaser{fi}, *it->second);
}

int MultiFileSystem::fsync(char const* path, int isdatasync, struct fuse_file_info* fi) noexcept
{
    auto it = inodes_.find(path);
    if (inodes_.end() == it)
        return -ENOENT;
    return std::visit(INodeFsyncer{isdatasync, fi}, *it->second);
}

#ifdef HAVE_UTIMENSAT
int MultiFileSystem::utimens(char const* path, const struct timespec ts[2], struct fuse_file_info* fi) noexcept
{
    auto it = inodes_.find(path);
    if (inodes_.end() == it)
        return -ENOENT;
    return std::visit(INodeUtimenser{ts, fi}, *it->second);
}
#endif // HAVE_UTIMENSAT

#ifdef HAVE_POSIX_FALLOCATE
int MultiFileSystem::fallocate(char const* path, int mode, off_t offset, off_t length, struct fuse_file_info* fi)
{
    auto it = inodes_.find(path);
    if (inodes_.end() == it)
        return -ENOENT;
    return std::visit(INodeFallocater{mode, offset, length, fi}, *it->second);
}
#endif // HAVE_POSIX_FALLOCATE

off_t MultiFileSystem::lseek(char const* path, off_t off, int whence, struct fuse_file_info* fi) const noexcept
{
    auto it = inodes_.find(path);
    if (inodes_.end() == it)
        return -ENOENT;
    return std::visit(INodeLseeker{off, whence, fi}, *it->second);
}
