#pragma once

#include <memory>
#include <utility>

#include "file_system_interface.hpp"
#include "log.hpp"

namespace multifs
{

class LoggedFileSystem final : public IFileSystem
{
private:
    std::shared_ptr<IFileSystem> fs_;

public:
    explicit LoggedFileSystem(std::shared_ptr<IFileSystem> fs)
        : fs_(std::move(fs))
    {
        if (!fs_)
            throw std::invalid_argument("fs provided cannot be empty");
    }
    ~LoggedFileSystem() override = default;

    LoggedFileSystem(LoggedFileSystem const&)            = delete;
    LoggedFileSystem& operator=(LoggedFileSystem const&) = delete;

    LoggedFileSystem(LoggedFileSystem&&)            = delete;
    LoggedFileSystem& operator=(LoggedFileSystem&&) = delete;

    int getattr(char const* path, struct stat* stbuf, struct fuse_file_info* fi) const noexcept override
    {
#ifndef NDEBUG
        out << "getattr, path " << path << std::endl;
#endif
        return fs_->getattr(path, stbuf, fi);
    }

    int readlink(char const* path, char* buf, size_t size) const noexcept override
    {
#ifndef NDEBUG
        out << "readlink, path " << path << ", buf " << buf << ", size " << size << std::endl;
#endif
        return fs_->readlink(path, buf, size);
    }

    int mknod(char const* path, mode_t mode, dev_t rdev) override
    {
#ifndef NDEBUG
        out << "mknod, path " << path << ", mode 0" << std::oct << mode << std::dec << std::endl;
#endif
        return fs_->mknod(path, mode, rdev);
    }

    int mkdir(char const* path, mode_t mode) override
    {
#ifndef NDEBUG
        out << "mkdir, path " << path << ", mode 0" << std::oct << mode << std::dec << std::endl;
#endif
        return fs_->mkdir(path, mode);
    }

    int rmdir(char const* path) override
    {
#ifndef NDEBUG
        out << "rmdir, path " << path << std::endl;
#endif
        return fs_->rmdir(path);
    }

    int symlink(char const* from, char const* to) override
    {
#ifndef NDEBUG
        out << "symlink, from " << from << ", to " << to << std::endl;
#endif
        return fs_->symlink(from, to);
    }

    int rename(char const* from, char const* to, unsigned int flags) override
    {
#ifndef NDEBUG
        out << "rename, from " << from << ", to " << to << ", flags 0x" << std::hex << flags << std::dec << std::endl;
#endif
        return fs_->rename(from, to, flags);
    }

    int link(char const* from, char const* to) override
    {
#ifndef NDEBUG
        out << "link, from " << from << ", to " << to << std::endl;
#endif
        return fs_->link(from, to);
    }

    int access(char const* path, int mask) const noexcept override
    {
#ifndef NDEBUG
        out << "access, path " << path << ", mask 0" << std::oct << mask << std::dec << std::endl;
#endif
        return fs_->access(path, mask);
    }

    int readdir(char const* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, fuse_readdir_flags flags) const override
    {
#ifndef NDEBUG
        out << "readdir, path " << path << ", buf " << buf << ", off " << offset << ", fi " << fi;
        if (fi)
            out << ", fi->flags 0" << std::oct << fi->flags << std::dec;
        out << ", flags 0x" << std::hex << static_cast<int>(flags) << std::dec << std::endl;
#endif
        return fs_->readdir(path, buf, filler, offset, fi, flags);
    }

    int unlink(char const* path) override
    {
#ifndef NDEBUG
        out << "unlink, path " << path << std::endl;
#endif
        return fs_->unlink(path);
    }

    int chmod(char const* path, mode_t mode, struct fuse_file_info* fi) noexcept override
    {
#ifndef NDEBUG
        out << "chmod, path " << path << ", mode 0" << std::oct << mode << std::dec << ", fi " << fi;
        if (fi)
            out << ", fi->flags 0" << std::oct << fi->flags << std::dec;
        out << std::endl;
#endif
        return fs_->chmod(path, mode, fi);
    }

    int chown(char const* path, uid_t uid, gid_t gid, struct fuse_file_info* fi) noexcept override
    {
#ifndef NDEBUG
        out << "chown, path " << path << ", uid " << uid << ", gid " << gid << ", fi " << fi;
        if (fi)
            out << ", fi->flags 0" << std::oct << fi->flags << std::dec;
        out << std::endl;
#endif
        return fs_->chown(path, uid, gid, fi);
    }

    int truncate(char const* path, off_t size, struct fuse_file_info* fi) override
    {
#ifndef NDEBUG
        out << "truncate, path " << path << ", size " << size << ", fi " << fi;
        if (fi)
            out << ", fi->flags 0" << std::oct << fi->flags << std::dec;
        out << std::endl;
#endif
        return fs_->truncate(path, size, fi);
    }

    int open(char const* path, struct fuse_file_info* fi) override
    {
#ifndef NDEBUG
        out << "open, path " << path << ", fi " << fi;
        if (fi)
            out << ", fi->flags 0" << std::oct << fi->flags << std::dec;
        out << std::endl;
#endif
        return fs_->open(path, fi);
    }

    int create(char const* path, mode_t mode, struct fuse_file_info* fi) override
    {
#ifndef NDEBUG
        out << "create, path " << path << ", mode 0" << std::oct << mode << std::dec << ", fi " << fi;
        if (fi)
            out << ", fi->flags 0" << std::oct << fi->flags << std::dec;
        out << std::endl;
#endif
        return fs_->create(path, mode, fi);
    }

    ssize_t read(char const* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi) const noexcept override
    {
#ifndef NDEBUG
        out << "read, path " << path << ", buf " << static_cast<void const*>(buf) << ", size " << size << ", off " << offset << ", fi " << fi;
        if (fi)
            out << ", fi->flags 0" << std::oct << fi->flags << std::dec;
        out << std::endl;
#endif
        return fs_->read(path, buf, size, offset, fi);
    }

    ssize_t write(char const* path, char const* buf, size_t size, off_t offset, struct fuse_file_info* fi) override
    {
#ifndef NDEBUG
        out << "write, path " << path << ", buf " << static_cast<void const*>(buf) << ", size " << size << ", off " << offset << ", fi " << fi;
        if (fi)
            out << ", fi->flags 0" << std::oct << fi->flags << std::dec;
        out << std::endl;
#endif
        return fs_->write(path, buf, size, offset, fi);
    }

    int statfs(char const* path, struct statvfs* stbuf) const noexcept override
    {
#ifndef NDEBUG
        out << "statfs, path " << path << ", stbuf " << stbuf << std::endl;
#endif
        return fs_->statfs(path, stbuf);
    }

    int release(char const* path, struct fuse_file_info* fi) noexcept override
    {
#ifndef NDEBUG
        out << "release, path " << path << ", fi " << fi;
        if (fi)
            out << ", fi->flags 0" << std::oct << fi->flags << std::dec;
        out << std::endl;
#endif
        return fs_->release(path, fi);
    }

    int fsync(char const* path, int isdatasync, struct fuse_file_info* fi) noexcept override
    {
#ifndef NDEBUG
        out << "fsync, path " << path << ", fi " << fi;
        if (fi)
            out << ", fi->flags 0" << std::oct << fi->flags << std::dec;
        out << std::endl;
#endif
        return fs_->fsync(path, isdatasync, fi);
    }

#ifdef HAVE_UTIMENSAT
    int utimens(char const* path, const struct timespec tv[2], struct fuse_file_info* fi) noexcept override
    {
#ifndef NDEBUG
        out << "utimens, path " << path;
        if (fi)
            out << ", fi->flags 0" << std::oct << fi->flags << std::dec;
        out << std::endl;
#endif
        return fs_->utimens(path, tv, fi);
    }
#endif // HAVE_UTIMENSAT

#ifdef HAVE_POSIX_FALLOCATE
    int fallocate(char const* path, int mode, off_t offset, off_t length, struct fuse_file_info* fi) override
    {
#ifndef NDEBUG
        out << "fallocate, path " << path << ", mode 0" << std::oct << mode << std::dec << ", fi " << fi;
        if (fi)
            out << ", fi->flags 0" << std::oct << fi->flags << std::dec;
        out << std::endl;
#endif
        return fs_->fallocate(path, mode, offset, length, fi);
    }
#endif // HAVE_POSIX_FALLOCATE

    off_t lseek(char const* path, off_t off, int whence, struct fuse_file_info* fi) const noexcept override
    {
#ifndef NDEBUG
        out << "lseek, path " << path << std::endl;
#endif
        return fs_->lseek(path, off, whence, fi);
    }
};

} // namespace multifs
