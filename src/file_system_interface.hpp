#pragma once

#include <cstddef>
#include <ctime>

#include <filesystem>
#include <span>

#include <sys/stat.h>
#include <sys/types.h>

#include <fuse.h>

namespace multifs
{

class IFileSystem
{
public:
    IFileSystem()          = default;
    virtual ~IFileSystem() = default;

    IFileSystem(IFileSystem const&)            = default;
    IFileSystem& operator=(IFileSystem const&) = default;

    IFileSystem(IFileSystem&&) noexcept            = default;
    IFileSystem& operator=(IFileSystem&&) noexcept = default;

    virtual int getattr(std::filesystem::path const& path, struct stat& stbuf, struct fuse_file_info* fi) const = 0;
    virtual int readlink(std::filesystem::path const& path, std::span<char> buf) const                          = 0;
    virtual int mknod(std::filesystem::path const& path, mode_t mode, dev_t rdev)                               = 0;
    virtual int mkdir(std::filesystem::path const& path, mode_t mode)                                           = 0;
    virtual int rmdir(std::filesystem::path const& path)                                                        = 0;
    virtual int symlink(std::filesystem::path const& from, std::filesystem::path const& to)                     = 0;
    virtual int rename(std::filesystem::path const& from, std::filesystem::path const& to, unsigned int flags)  = 0;
    virtual int link(std::filesystem::path const& from, std::filesystem::path const& to)                        = 0;
    virtual int access(std::filesystem::path const& path, int mask) const                                       = 0;
    virtual int
    readdir(std::filesystem::path const& path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, fuse_readdir_flags flags) const = 0;
    virtual int unlink(std::filesystem::path const& path)                                                                                                  = 0;
    virtual int chmod(std::filesystem::path const& path, mode_t mode, struct fuse_file_info* fi)                                                           = 0;
    virtual int chown(std::filesystem::path const& path, uid_t uid, gid_t gid, struct fuse_file_info* fi)                                                  = 0;
    virtual int truncate(std::filesystem::path const& path, off_t size, struct fuse_file_info* fi)                                                         = 0;
    virtual int open(std::filesystem::path const& path, struct fuse_file_info* fi)                                                                         = 0;
    virtual int create(std::filesystem::path const& path, mode_t mode, struct fuse_file_info* fi)                                                          = 0;
    virtual ssize_t read(std::filesystem::path const& path, std::span<std::byte> buf, off_t offset, struct fuse_file_info*) const                          = 0;
    virtual ssize_t write(std::filesystem::path const& path, std::span<std::byte const> buf, off_t offset, struct fuse_file_info* fi)                      = 0;
    virtual int statfs(std::filesystem::path const& path, struct statvfs& stbuf) const                                                                     = 0;
    virtual int release(std::filesystem::path const& path, struct fuse_file_info* fi)                                                                      = 0;
    virtual int fsync(std::filesystem::path const& path, int isdatasync, struct fuse_file_info* fi)                                                        = 0;
#ifdef HAVE_UTIMENSAT
    virtual int utimens(std::filesystem::path const& path, const struct timespec ts[2], struct fuse_file_info* fi) = 0;
#endif // HAVE_UTIMENSAT
#ifdef HAVE_POSIX_FALLOCATE
    virtual int fallocate(std::filesystem::path const& path, int mode, off_t offset, off_t length, struct fuse_file_info* fi) = 0;
#endif // HAVE_POSIX_FALLOCATE
    virtual off_t lseek(std::filesystem::path const& path, off_t off, int whence, struct fuse_file_info* fi) const = 0;
};

} // namespace multifs
