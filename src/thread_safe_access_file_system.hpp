#pragma once

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <utility>

#include "file_system_interface.hpp"

namespace multifs
{

class ThreadSafeAccessFileSystem final : public IFileSystem
{
private:
    mutable std::shared_mutex lock_;
    std::shared_ptr<IFileSystem> fs_;

public:
    explicit ThreadSafeAccessFileSystem(std::shared_ptr<IFileSystem> fs)
        : fs_(std::move(fs))
    {
        if (!fs_)
            throw std::invalid_argument("fs provided cannot be empty");
    }
    ~ThreadSafeAccessFileSystem() override = default;

    ThreadSafeAccessFileSystem(ThreadSafeAccessFileSystem const&)            = delete;
    ThreadSafeAccessFileSystem& operator=(ThreadSafeAccessFileSystem const&) = delete;

    ThreadSafeAccessFileSystem(ThreadSafeAccessFileSystem&&)            = delete;
    ThreadSafeAccessFileSystem& operator=(ThreadSafeAccessFileSystem&&) = delete;

    int getattr(char const* path, struct stat* stbuf, struct fuse_file_info* fi) const noexcept override
    {
        std::shared_lock g{lock_};
        return fs_->getattr(path, stbuf, fi);
    }

    int readlink(char const* path, char* buf, size_t size) const noexcept override
    {
        std::shared_lock g{lock_};
        return fs_->readlink(path, buf, size);
    }

    int mknod(char const* path, mode_t mode, dev_t rdev) override
    {
        std::lock_guard g{lock_};
        return fs_->mknod(path, mode, rdev);
    }

    int mkdir(char const* path, mode_t mode) override
    {
        std::lock_guard g{lock_};
        return fs_->mkdir(path, mode);
    }

    int rmdir(char const* path) override
    {
        std::lock_guard g{lock_};
        return fs_->rmdir(path);
    }

    int symlink(char const* from, char const* to) override
    {
        std::lock_guard g{lock_};
        return fs_->symlink(from, to);
    }

    int rename(char const* from, char const* to, unsigned int flags) override
    {
        std::lock_guard g{lock_};
        return fs_->rename(from, to, flags);
    }

    int link(char const* from, char const* to) override
    {
        std::lock_guard g{lock_};
        return fs_->link(from, to);
    }

    int access(char const* path, int mask) const noexcept override
    {
        std::shared_lock g{lock_};
        return fs_->access(path, mask);
    }

    int readdir(char const* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, fuse_readdir_flags flags) const override
    {
        std::shared_lock g{lock_};
        return fs_->readdir(path, buf, filler, offset, fi, flags);
    }

    int unlink(char const* path) override
    {
        std::lock_guard g{lock_};
        return fs_->unlink(path);
    }

    int chmod(char const* path, mode_t mode, struct fuse_file_info* fi) noexcept override
    {
        std::lock_guard g{lock_};
        return fs_->chmod(path, mode, fi);
    }

    int chown(char const* path, uid_t uid, gid_t gid, struct fuse_file_info* fi) noexcept override
    {
        std::lock_guard g{lock_};
        return fs_->chown(path, uid, gid, fi);
    }

    int truncate(char const* path, off_t size, struct fuse_file_info* fi) override
    {
        std::lock_guard g{lock_};
        return fs_->truncate(path, size, fi);
    }

    int open(char const* path, struct fuse_file_info* fi) override
    {
        std::lock_guard g{lock_};
        return fs_->open(path, fi);
    }

    int create(char const* path, mode_t mode, struct fuse_file_info* fi) override
    {
        std::lock_guard g{lock_};
        return fs_->create(path, mode, fi);
    }

    ssize_t read(char const* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi) const noexcept override
    {
        std::shared_lock g{lock_};
        return fs_->read(path, buf, size, offset, fi);
    }

    ssize_t write(char const* path, char const* buf, size_t size, off_t offset, struct fuse_file_info* fi) override
    {
        std::lock_guard g{lock_};
        return fs_->write(path, buf, size, offset, fi);
    }

    int statfs(char const* path, struct statvfs* stbuf) const noexcept override
    {
        std::shared_lock g{lock_};
        return fs_->statfs(path, stbuf);
    }

    int release(char const* path, struct fuse_file_info* fi) noexcept override
    {
        std::lock_guard g{lock_};
        return fs_->release(path, fi);
    }

    int fsync(char const* path, int isdatasync, struct fuse_file_info* fi) noexcept override
    {
        std::lock_guard g{lock_};
        return fs_->fsync(path, isdatasync, fi);
    }

#ifdef HAVE_UTIMENSAT
    int utimens(char const* path, const struct timespec tv[2], struct fuse_file_info* fi) noexcept override
    {
        std::lock_guard g{lock_};
        return fs_->utimens(path, tv, fi);
    }
#endif // HAVE_UTIMENSAT

#ifdef HAVE_POSIX_FALLOCATE
    int fallocate(char const* path, int mode, off_t offset, off_t length, struct fuse_file_info* fi) override
    {
        std::lock_guard g{lock_};
        return fs_->fallocate(path, mode, offset, length, fi);
    }
#endif // HAVE_POSIX_FALLOCATE

    off_t lseek(char const* path, off_t off, int whence, struct fuse_file_info* fi) const noexcept override
    {
        std::shared_lock g{lock_};
        return fs_->lseek(path, off, whence, fi);
    }
};

} // namespace multifs
