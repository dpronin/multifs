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
    std::shared_ptr<IFileSystem> fs_;

public:
    explicit FileSystemNoexcept(std::shared_ptr<IFileSystem> fs)
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

    int getattr(char const* path, struct stat* stbuf, struct fuse_file_info* fi) const noexcept override { return fs_->getattr(path, stbuf, fi); }

    int readlink(char const* path, char* buf, size_t size) const noexcept override { return fs_->readlink(path, buf, size); }

    int mknod(char const* path, mode_t mode, dev_t rdev) noexcept override
    {
        return wrap([this](auto... args) { return fs_->mknod(args...); }, path, mode, rdev);
    }

    int mkdir(char const* path, mode_t mode) noexcept override
    {
        return wrap([this](auto... args) { return fs_->mkdir(args...); }, path, mode);
    }

    int rmdir(char const* path) noexcept override
    {
        return wrap([this](auto... args) { return fs_->rmdir(args...); }, path);
    }

    int symlink(char const* from, char const* to) noexcept override
    {
        return wrap([this](auto... args) { return fs_->symlink(args...); }, from, to);
    }

    int rename(char const* from, char const* to, unsigned int flags) noexcept override
    {
        return wrap([this](auto... args) { return fs_->rename(args...); }, from, to, flags);
    }

    int link(char const* from, char const* to) noexcept override
    {
        return wrap([this](auto... args) { return fs_->link(args...); }, from, to);
    }

    int access(char const* path, int mask) const noexcept override { return fs_->access(path, mask); }

    int readdir(char const* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, fuse_readdir_flags flags) const noexcept override
    {
        return wrap([this](auto... args) { return fs_->readdir(args...); }, path, buf, filler, offset, fi, flags);
    }

    int unlink(char const* path) noexcept override
    {
        return wrap([this](auto... args) { return fs_->unlink(args...); }, path);
    }

    int chmod(char const* path, mode_t mode, struct fuse_file_info* fi) noexcept override { return fs_->chmod(path, mode, fi); }

    int chown(char const* path, uid_t uid, gid_t gid, struct fuse_file_info* fi) noexcept override { return fs_->chown(path, uid, gid, fi); }

    int truncate(char const* path, off_t size, struct fuse_file_info* fi) noexcept override
    {
        return wrap([this](auto... args) { return fs_->truncate(args...); }, path, size, fi);
    }

    int open(char const* path, struct fuse_file_info* fi) noexcept override
    {
        return wrap([this](auto... args) { return fs_->open(args...); }, path, fi);
    }

    int create(char const* path, mode_t mode, struct fuse_file_info* fi) noexcept override
    {
        return wrap([this](auto... args) { return fs_->create(args...); }, path, mode, fi);
    }

    ssize_t read(char const* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi) const noexcept override
    {
        return fs_->read(path, buf, size, offset, fi);
    }

    ssize_t write(char const* path, char const* buf, size_t size, off_t offset, struct fuse_file_info* fi) noexcept override
    {
        return wrap([this](auto... args) { return static_cast<int>(fs_->write(args...)); }, path, buf, size, offset, fi);
    }

    int statfs(char const* path, struct statvfs* stbuf) const noexcept override { return fs_->statfs(path, stbuf); }

    int release(char const* path, struct fuse_file_info* fi) noexcept override { return fs_->release(path, fi); }

    int fsync(char const* path, int isdatasync, struct fuse_file_info* fi) noexcept override { return fs_->fsync(path, isdatasync, fi); }

#ifdef HAVE_UTIMENSAT
    int utimens(char const* path, const struct timespec tv[2], struct fuse_file_info* fi) noexcept override { return fs_->utimens(path, tv, fi); }
#endif // HAVE_UTIMENSAT

#ifdef HAVE_POSIX_FALLOCATE
    int fallocate(char const* path, int mode, off_t offset, off_t length, struct fuse_file_info* fi) noexcept override
    {
        return wrap([this](auto... args) { return fs_->fallocate(args...); }, path, mode, offset, length, fi);
    }
#endif // HAVE_POSIX_FALLOCATE

    off_t lseek(char const* path, off_t off, int whence, struct fuse_file_info* fi) const noexcept override { return fs_->lseek(path, off, whence, fi); }
};

} // namespace multifs