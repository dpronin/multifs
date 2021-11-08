#pragma once

#include <filesystem>

#include "file_system_interface.hpp"

namespace multifs
{

class FileSystemReflector final : public IFileSystem
{
private:
    std::filesystem::path mp_;

    std::filesystem::path to_path(std::string_view path) const
    {
        path.remove_prefix(1);
        return mp_ / path;
    }

public:
    explicit FileSystemReflector(std::filesystem::path mount_point);
    ~FileSystemReflector() override = default;

    FileSystemReflector(FileSystemReflector const&) = default;
    FileSystemReflector& operator=(FileSystemReflector const&) = default;

    FileSystemReflector(FileSystemReflector&&) noexcept = default;
    FileSystemReflector& operator=(FileSystemReflector&&) noexcept = default;

    int getattr(const char* path, struct stat* stbuf, struct fuse_file_info* fi) const noexcept override;
    int readlink(const char* path, char* buf, size_t size) const noexcept override;
    int mknod(const char* path, mode_t mode, dev_t rdev) override;
    int mkdir(const char* path, mode_t mode) override;
    int rmdir(const char* path) override;
    int symlink(const char* from, const char* to) override;
    int rename(const char* from, const char* to, unsigned int flags) override;
    int link(char const* from, char const* to) override;
    void* init(struct fuse_conn_info* conn, struct fuse_config* cfg) noexcept override;
    int access(char const* path, int mask) const noexcept override;
    int readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, fuse_readdir_flags flags) const override;
    int unlink(const char* path) override;
    int chmod(const char* path, mode_t mode, struct fuse_file_info* fi) noexcept override;
    int chown(const char* path, uid_t uid, gid_t gid, struct fuse_file_info* fi) noexcept override;
    int truncate(const char* path, off_t size, struct fuse_file_info* fi) override;
    int open(const char* path, struct fuse_file_info* fi) override;
    int create(const char* path, mode_t mode, struct fuse_file_info* fi) override;
    ssize_t read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi) const noexcept override;
    ssize_t write(const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi) override;
    int statfs(char const* path, struct statvfs* stbuf) const noexcept override;
    int release(char const* path, struct fuse_file_info* fi) noexcept override;
    int fsync(char const* path, int isdatasync, struct fuse_file_info* fi) noexcept override;
#ifdef HAVE_UTIMENSAT
    int utimens(const char* path, const struct timespec ts[2], struct fuse_file_info* fi) noexcept override;
#endif // HAVE_UTIMENSAT
#ifdef HAVE_POSIX_FALLOCATE
    int fallocate(char const* path, int mode, off_t offset, off_t length, struct fuse_file_info* fi) override;
#endif // HAVE_POSIX_FALLOCATE
    off_t lseek(char const* path, off_t off, int whence, struct fuse_file_info* fi) const noexcept override;
};

} // namespace multifs
