#pragma once

#include <memory>
#include <utility>

#include "file_system_interface.hpp"
#include "file_system_noexcept_interface.hpp"

#include "wrap.hpp"

namespace multifs
{

class FileSystemNoexcept final : public IFileSystemNoexcept
{
private:
    std::unique_ptr<IFileSystem> fs_;

public:
    explicit FileSystemNoexcept(std::unique_ptr<IFileSystem> fs)
        : fs_(std::move(fs))
    {
        if (!fs_)
            throw std::invalid_argument("fs provided cannot be empty");
    }
    ~FileSystemNoexcept() override = default;

    FileSystemNoexcept(FileSystemNoexcept const&)            = delete;
    FileSystemNoexcept& operator=(FileSystemNoexcept const&) = delete;

    FileSystemNoexcept(FileSystemNoexcept&&)            = delete;
    FileSystemNoexcept& operator=(FileSystemNoexcept&&) = delete;

    int getattr(std::string_view path, struct stat& stbuf, struct fuse_file_info* fi) const noexcept override
    {
        return wrap([this](auto&&... args) { return fs_->getattr(std::forward<decltype(args)>(args)...); }, path, stbuf, fi);
    }

    int readlink(std::string_view path, std::span<char> buf) const noexcept override
    {
        return wrap([this](auto&&... args) { return fs_->readlink(std::forward<decltype(args)>(args)...); }, path, buf);
    }

    int mknod(std::string_view path, mode_t mode, dev_t rdev) noexcept override
    {
        return wrap([this](auto&&... args) { return fs_->mknod(std::forward<decltype(args)>(args)...); }, path, mode, rdev);
    }

    int mkdir(std::string_view path, mode_t mode) noexcept override
    {
        return wrap([this](auto&&... args) { return fs_->mkdir(std::forward<decltype(args)>(args)...); }, path, mode);
    }

    int rmdir(std::string_view path) noexcept override
    {
        return wrap([this](auto&&... args) { return fs_->rmdir(std::forward<decltype(args)>(args)...); }, path);
    }

    int symlink(std::string_view from, std::string_view to) noexcept override
    {
        return wrap([this](auto&&... args) { return fs_->symlink(std::forward<decltype(args)>(args)...); }, from, to);
    }

    int rename(std::string_view from, std::string_view to, unsigned int flags) noexcept override
    {
        return wrap([this](auto&&... args) { return fs_->rename(std::forward<decltype(args)>(args)...); }, from, to, flags);
    }

    int link(std::string_view from, std::string_view to) noexcept override
    {
        return wrap([this](auto&&... args) { return fs_->link(std::forward<decltype(args)>(args)...); }, from, to);
    }

    int access(std::string_view path, int mask) const noexcept override
    {
        return wrap([this](auto&&... args) { return fs_->access(std::forward<decltype(args)>(args)...); }, path, mask);
    }

    int
    readdir(std::string_view path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, fuse_readdir_flags flags) const noexcept override
    {
        return wrap([this](auto&&... args) { return fs_->readdir(std::forward<decltype(args)>(args)...); }, path, buf, filler, offset, fi, flags);
    }

    int unlink(std::string_view path) noexcept override
    {
        return wrap([this](auto&&... args) { return fs_->unlink(std::forward<decltype(args)>(args)...); }, path);
    }

    int chmod(std::string_view path, mode_t mode, struct fuse_file_info* fi) noexcept override
    {
        return wrap([this](auto&&... args) { return fs_->chmod(std::forward<decltype(args)>(args)...); }, path, mode, fi);
    }

    int chown(std::string_view path, uid_t uid, gid_t gid, struct fuse_file_info* fi) noexcept override
    {
        return wrap([this](auto&&... args) { return fs_->chown(std::forward<decltype(args)>(args)...); }, path, uid, gid, fi);
    }

    int truncate(std::string_view path, off_t size, struct fuse_file_info* fi) noexcept override
    {
        return wrap([this](auto&&... args) { return fs_->truncate(std::forward<decltype(args)>(args)...); }, path, size, fi);
    }

    int open(std::string_view path, struct fuse_file_info* fi) noexcept override
    {
        return wrap([this](auto&&... args) { return fs_->open(std::forward<decltype(args)>(args)...); }, path, fi);
    }

    int create(std::string_view path, mode_t mode, struct fuse_file_info* fi) noexcept override
    {
        return wrap([this](auto&&... args) { return fs_->create(std::forward<decltype(args)>(args)...); }, path, mode, fi);
    }

    ssize_t read(std::string_view path, std::span<std::byte> buf, off_t offset, struct fuse_file_info* fi) const noexcept override
    {
        return wrap([this](auto&&... args) { return fs_->read(std::forward<decltype(args)>(args)...); }, path, buf, offset, fi);
    }

    ssize_t write(std::string_view path, std::span<std::byte const> buf, off_t offset, struct fuse_file_info* fi) noexcept override
    {
        return wrap([this](auto&&... args) { return fs_->write(std::forward<decltype(args)>(args)...); }, path, buf, offset, fi);
    }

    int statfs(std::string_view path, struct statvfs& stbuf) const noexcept override
    {
        return wrap([this](auto&&... args) { return fs_->statfs(std::forward<decltype(args)>(args)...); }, path, stbuf);
    }

    int release(std::string_view path, struct fuse_file_info* fi) noexcept override
    {
        return wrap([this](auto&&... args) { return fs_->release(std::forward<decltype(args)>(args)...); }, path, fi);
    }

    int fsync(std::string_view path, int isdatasync, struct fuse_file_info* fi) noexcept override
    {
        return wrap([this](auto&&... args) { return fs_->fsync(std::forward<decltype(args)>(args)...); }, path, isdatasync, fi);
    }

#ifdef HAVE_UTIMENSAT
    int utimens(std::string_view path, const struct timespec tv[2], struct fuse_file_info* fi) noexcept override
    {
        return wrap([this](auto&&... args) { return fs_->utimens(std::forward<decltype(args)>(args)...); }, path, tv, fi);
    }
#endif // HAVE_UTIMENSAT

#ifdef HAVE_POSIX_FALLOCATE
    int fallocate(std::string_view path, int mode, off_t offset, off_t length, struct fuse_file_info* fi) noexcept override
    {
        return wrap([this](auto&&... args) { return fs_->fallocate(std::forward<decltype(args)>(args)...); }, path, mode, offset, length, fi);
    }
#endif // HAVE_POSIX_FALLOCATE

    off_t lseek(std::string_view path, off_t off, int whence, struct fuse_file_info* fi) const noexcept override
    {
        return wrap([this](auto&&... args) { return fs_->lseek(std::forward<decltype(args)>(args)...); }, path, off, whence, fi);
    }
};

} // namespace multifs
