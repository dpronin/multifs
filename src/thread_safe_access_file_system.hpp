#pragma once

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <utility>

#include "file_system_interface.hpp"

namespace multifs
{

class LockExclusive
{
public:
    void lock() { m_.lock(); }
    void unlock() { m_.unlock(); }
    bool try_lock() { return m_.try_lock(); }

    void lock_shared() { lock(); }
    void unlock_shared() { unlock(); }
    bool try_lock_shared() { return try_lock(); }

private:
    std::mutex m_;
};

class RWLock
{
public:
    void lock() { m_.lock(); }
    void unlock() { m_.unlock(); }
    bool try_lock() { return m_.try_lock(); }

    void lock_shared() { m_.lock_shared(); }
    void unlock_shared() { m_.unlock_shared(); }
    bool try_lock_shared() { return m_.try_lock_shared(); }

private:
    std::shared_mutex m_;
};

template <typename Lock>
class ThreadSafeAccessFileSystem final : public IFileSystem
{
private:
    mutable Lock lock_;
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

    int getattr(std::filesystem::path const& path, struct stat& stbuf, struct fuse_file_info* fi) const override
    {
        std::shared_lock g{lock_};
        return fs_->getattr(path, stbuf, fi);
    }

    int readlink(std::filesystem::path const& path, std::span<char> buf) const override
    {
        std::shared_lock g{lock_};
        return fs_->readlink(path, buf);
    }

    int mknod(std::filesystem::path const& path, mode_t mode, dev_t rdev) override
    {
        std::lock_guard g{lock_};
        return fs_->mknod(path, mode, rdev);
    }

    int mkdir(std::filesystem::path const& path, mode_t mode) override
    {
        std::lock_guard g{lock_};
        return fs_->mkdir(path, mode);
    }

    int rmdir(std::filesystem::path const& path) override
    {
        std::lock_guard g{lock_};
        return fs_->rmdir(path);
    }

    int symlink(std::filesystem::path const& from, std::filesystem::path const& to) override
    {
        std::lock_guard g{lock_};
        return fs_->symlink(from, to);
    }

    int rename(std::filesystem::path const& from, std::filesystem::path const& to, unsigned int flags) override
    {
        std::lock_guard g{lock_};
        return fs_->rename(from, to, flags);
    }

    int link(std::filesystem::path const& from, std::filesystem::path const& to) override
    {
        std::lock_guard g{lock_};
        return fs_->link(from, to);
    }

    int access(std::filesystem::path const& path, int mask) const override
    {
        std::shared_lock g{lock_};
        return fs_->access(path, mask);
    }

    int readdir(
        std::filesystem::path const& path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, fuse_readdir_flags flags) const override
    {
        std::shared_lock g{lock_};
        return fs_->readdir(path, buf, filler, offset, fi, flags);
    }

    int unlink(std::filesystem::path const& path) override
    {
        std::lock_guard g{lock_};
        return fs_->unlink(path);
    }

    int chmod(std::filesystem::path const& path, mode_t mode, struct fuse_file_info* fi) noexcept override
    {
        std::lock_guard g{lock_};
        return fs_->chmod(path, mode, fi);
    }

    int chown(std::filesystem::path const& path, uid_t uid, gid_t gid, struct fuse_file_info* fi) noexcept override
    {
        std::lock_guard g{lock_};
        return fs_->chown(path, uid, gid, fi);
    }

    int truncate(std::filesystem::path const& path, off_t size, struct fuse_file_info* fi) override
    {
        std::lock_guard g{lock_};
        return fs_->truncate(path, size, fi);
    }

    int open(std::filesystem::path const& path, struct fuse_file_info* fi) override
    {
        std::lock_guard g{lock_};
        return fs_->open(path, fi);
    }

    int create(std::filesystem::path const& path, mode_t mode, struct fuse_file_info* fi) override
    {
        std::lock_guard g{lock_};
        return fs_->create(path, mode, fi);
    }

    ssize_t read(std::filesystem::path const& path, std::span<std::byte> buf, off_t offset, struct fuse_file_info* fi) const override
    {
        std::shared_lock g{lock_};
        return fs_->read(path, buf, offset, fi);
    }

    ssize_t write(std::filesystem::path const& path, std::span<std::byte const> buf, off_t offset, struct fuse_file_info* fi) override
    {
        std::lock_guard g{lock_};
        return fs_->write(path, buf, offset, fi);
    }

    int statfs(std::filesystem::path const& path, struct statvfs& stbuf) const override
    {
        std::shared_lock g{lock_};
        return fs_->statfs(path, stbuf);
    }

    int release(std::filesystem::path const& path, struct fuse_file_info* fi) noexcept override
    {
        std::lock_guard g{lock_};
        return fs_->release(path, fi);
    }

    int fsync(std::filesystem::path const& path, int isdatasync, struct fuse_file_info* fi) noexcept override
    {
        std::lock_guard g{lock_};
        return fs_->fsync(path, isdatasync, fi);
    }

#ifdef HAVE_UTIMENSAT
    int utimens(std::filesystem::path const& path, const struct timespec tv[2], struct fuse_file_info* fi) noexcept override
    {
        std::lock_guard g{lock_};
        return fs_->utimens(path, tv, fi);
    }
#endif // HAVE_UTIMENSAT

#ifdef HAVE_POSIX_FALLOCATE
    int fallocate(std::filesystem::path const& path, int mode, off_t offset, off_t length, struct fuse_file_info* fi) override
    {
        std::lock_guard g{lock_};
        return fs_->fallocate(path, mode, offset, length, fi);
    }
#endif // HAVE_POSIX_FALLOCATE

    off_t lseek(std::filesystem::path const& path, off_t off, int whence, struct fuse_file_info* fi) const override
    {
        std::shared_lock g{lock_};
        return fs_->lseek(path, off, whence, fi);
    }
};

} // namespace multifs
