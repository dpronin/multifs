#pragma once

#include <cstddef>
#include <ctime>

#include <span>
#include <string_view>

#include <sys/stat.h>
#include <sys/types.h>

#include <fuse.h>

namespace multifs
{

class IFileSystemNoexcept
{
public:
    IFileSystemNoexcept()          = default;
    virtual ~IFileSystemNoexcept() = default;

    IFileSystemNoexcept(IFileSystemNoexcept const&)            = default;
    IFileSystemNoexcept& operator=(IFileSystemNoexcept const&) = default;

    IFileSystemNoexcept(IFileSystemNoexcept&&) noexcept            = default;
    IFileSystemNoexcept& operator=(IFileSystemNoexcept&&) noexcept = default;

    virtual int getattr(std::string_view path, struct stat& stbuf, struct fuse_file_info* fi) const noexcept = 0;
    virtual int readlink(std::string_view path, std::span<char> buf) const noexcept                          = 0;
    virtual int mknod(std::string_view path, mode_t mode, dev_t rdev) noexcept                               = 0;
    virtual int mkdir(std::string_view path, mode_t mode) noexcept                                           = 0;
    virtual int rmdir(std::string_view path) noexcept                                                        = 0;
    virtual int symlink(std::string_view from, std::string_view to) noexcept                                 = 0;
    virtual int rename(std::string_view from, std::string_view to, unsigned int flags) noexcept              = 0;
    virtual int link(std::string_view from, std::string_view to) noexcept                                    = 0;
    virtual int access(std::string_view path, int mask) const noexcept                                       = 0;
    virtual int
    readdir(std::string_view path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, fuse_readdir_flags flags) const noexcept = 0;
    virtual int unlink(std::string_view path) noexcept                                                                                                  = 0;
    virtual int chmod(std::string_view path, mode_t mode, struct fuse_file_info* fi) noexcept                                                           = 0;
    virtual int chown(std::string_view path, uid_t uid, gid_t gid, struct fuse_file_info* fi) noexcept                                                  = 0;
    virtual int truncate(std::string_view path, off_t size, struct fuse_file_info* fi) noexcept                                                         = 0;
    virtual int open(std::string_view path, struct fuse_file_info* fi) noexcept                                                                         = 0;
    virtual int create(std::string_view path, mode_t mode, struct fuse_file_info* fi) noexcept                                                          = 0;
    virtual ssize_t read(std::string_view path, std::span<std::byte> buf, off_t offset, struct fuse_file_info*) const noexcept                          = 0;
    virtual ssize_t write(std::string_view path, std::span<std::byte const> buf, off_t offset, struct fuse_file_info* fi) noexcept                      = 0;
    virtual int statfs(std::string_view path, struct statvfs& stbuf) const noexcept                                                                     = 0;
    virtual int release(std::string_view path, struct fuse_file_info* fi) noexcept                                                                      = 0;
    virtual int fsync(std::string_view path, int isdatasync, struct fuse_file_info* fi) noexcept                                                        = 0;
#ifdef HAVE_UTIMENSAT
    virtual int utimens(std::string_view path, const struct timespec ts[2], struct fuse_file_info* fi) noexcept = 0;
#endif // HAVE_UTIMENSAT
#ifdef HAVE_POSIX_FALLOCATE
    virtual int fallocate(std::string_view path, int mode, off_t offset, off_t length, struct fuse_file_info* fi) noexcept = 0;
#endif // HAVE_POSIX_FALLOCATE
    virtual off_t lseek(std::string_view path, off_t off, int whence, struct fuse_file_info* fi) const noexcept = 0;
};

} // namespace multifs
