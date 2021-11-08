#pragma once

#include <cstddef>
#include <ctime>

#include <sys/stat.h>
#include <sys/types.h>

extern "C" {
#include "fuse.h"
} // extern "C"

namespace multifs
{

class IFileSystem
{
public:
    IFileSystem()          = default;
    virtual ~IFileSystem() = default;

    IFileSystem(IFileSystem const&) = default;
    IFileSystem& operator=(IFileSystem const&) = default;

    IFileSystem(IFileSystem&&) noexcept = default;
    IFileSystem& operator=(IFileSystem&&) noexcept = default;

    virtual int getattr(char const* path, struct stat* stbuf, struct fuse_file_info* fi) const noexcept                                               = 0;
    virtual int readlink(char const* path, char* buf, size_t size) const noexcept                                                                     = 0;
    virtual int mknod(char const* path, mode_t mode, dev_t rdev)                                                                                      = 0;
    virtual int mkdir(char const* path, mode_t mode)                                                                                                  = 0;
    virtual int rmdir(char const* path)                                                                                                               = 0;
    virtual int symlink(char const* from, char const* to)                                                                                             = 0;
    virtual int rename(char const* from, char const* to, unsigned int flags)                                                                          = 0;
    virtual int link(char const* from, char const* to)                                                                                                = 0;
    virtual void* init(struct fuse_conn_info* conn, struct fuse_config* cfg) noexcept                                                                 = 0;
    virtual int access(char const* path, int mask) const noexcept                                                                                     = 0;
    virtual int readdir(char const* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, fuse_readdir_flags flags) const = 0;
    virtual int unlink(char const* path)                                                                                                              = 0;
    virtual int chmod(char const* path, mode_t mode, struct fuse_file_info* fi) noexcept                                                              = 0;
    virtual int chown(char const* path, uid_t uid, gid_t gid, struct fuse_file_info* fi) noexcept                                                     = 0;
    virtual int truncate(char const* path, off_t size, struct fuse_file_info* fi)                                                                     = 0;
    virtual int open(char const* path, struct fuse_file_info* fi)                                                                                     = 0;
    virtual int create(char const* path, mode_t mode, struct fuse_file_info* fi)                                                                      = 0;
    virtual ssize_t read(char const* path, char* buf, size_t size, off_t offset, struct fuse_file_info*) const noexcept                               = 0;
    virtual ssize_t write(char const* path, char const* buf, size_t size, off_t offset, struct fuse_file_info* fi)                                    = 0;
    virtual int statfs(char const* path, struct statvfs* stbuf) const noexcept                                                                        = 0;
    virtual int release(char const* path, struct fuse_file_info* fi) noexcept                                                                         = 0;
    virtual int fsync(char const* path, int isdatasync, struct fuse_file_info* fi) noexcept                                                           = 0;
#ifdef HAVE_UTIMENSAT
    virtual int utimens(char const* path, const struct timespec ts[2], struct fuse_file_info* fi) noexcept = 0;
#endif // HAVE_UTIMENSAT
#ifdef HAVE_POSIX_FALLOCATE
    virtual int fallocate(char const* path, int mode, off_t offset, off_t length, struct fuse_file_info* fi) = 0;
#endif // HAVE_POSIX_FALLOCATE
    virtual off_t lseek(char const* path, off_t off, int whence, struct fuse_file_info* fi) const noexcept = 0;
};

} // namespace multifs
