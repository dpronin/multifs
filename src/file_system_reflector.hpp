#pragma once

#include <filesystem>

#include "file_system_interface.hpp"

namespace multifs
{

class FileSystemReflector final : public IFileSystem
{
private:
    std::filesystem::path mp_;

    std::filesystem::path to_path(std::filesystem::path const& path) const { return mp_ / path.relative_path(); }

public:
    explicit FileSystemReflector(std::filesystem::path mount_point);
    ~FileSystemReflector() override = default;

    FileSystemReflector(FileSystemReflector const&)            = default;
    FileSystemReflector& operator=(FileSystemReflector const&) = default;

    FileSystemReflector(FileSystemReflector&&) noexcept            = default;
    FileSystemReflector& operator=(FileSystemReflector&&) noexcept = default;

    int getattr(std::filesystem::path const& path, struct stat& stbuf, struct fuse_file_info* fi) const override;
    int readlink(std::filesystem::path const& path, std::span<char> buf) const override;
    int mknod(std::filesystem::path const& path, mode_t mode, dev_t rdev) override;
    int mkdir(std::filesystem::path const& path, mode_t mode) override;
    int rmdir(std::filesystem::path const& path) override;
    int symlink(std::filesystem::path const& from, std::filesystem::path const& to) override;
    int rename(std::filesystem::path const& from, std::filesystem::path const& to, unsigned int flags) override;
    int link(std::filesystem::path const& from, std::filesystem::path const& to) override;
    int access(std::filesystem::path const& path, int mask) const override;
    int readdir(
        std::filesystem::path const& path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, fuse_readdir_flags flags) const override;
    int unlink(std::filesystem::path const& path) override;
    int chmod(std::filesystem::path const& path, mode_t mode, struct fuse_file_info* fi) override;
    int chown(std::filesystem::path const& path, uid_t uid, gid_t gid, struct fuse_file_info* fi) override;
    int truncate(std::filesystem::path const& path, off_t size, struct fuse_file_info* fi) override;
    int open(std::filesystem::path const& path, struct fuse_file_info* fi) override;
    int create(std::filesystem::path const& path, mode_t mode, struct fuse_file_info* fi) override;
    ssize_t read(std::filesystem::path const& path, std::span<std::byte> buf, off_t offset, struct fuse_file_info* fi) const override;
    ssize_t write(std::filesystem::path const& path, std::span<std::byte const> buf, off_t offset, struct fuse_file_info* fi) override;
    int statfs(std::filesystem::path const& path, struct statvfs& stbuf) const override;
    int release(std::filesystem::path const& path, struct fuse_file_info* fi) override;
    int fsync(std::filesystem::path const& path, int isdatasync, struct fuse_file_info* fi) override;
#ifdef HAVE_UTIMENSAT
    int utimens(std::filesystem::path const& path, const struct timespec ts[2], struct fuse_file_info* fi) override;
#endif // HAVE_UTIMENSAT
#ifdef HAVE_POSIX_FALLOCATE
    int fallocate(std::filesystem::path const& path, int mode, off_t offset, off_t length, struct fuse_file_info* fi) override;
#endif // HAVE_POSIX_FALLOCATE
    off_t lseek(std::filesystem::path const& path, off_t off, int whence, struct fuse_file_info* fi) const override;
};

} // namespace multifs
