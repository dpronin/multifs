#pragma once

#include <filesystem>
#include <fstream>
#include <memory>
#include <utility>

#include "file_system_interface.hpp"

namespace multifs
{

class LoggedFileSystem final : public IFileSystem
{
private:
    std::shared_ptr<IFileSystem> fs_;
    mutable std::ofstream out_;

public:
    explicit LoggedFileSystem(std::shared_ptr<IFileSystem> fs, std::filesystem::path const& logpath)
        : fs_(std::move(fs))
        , out_(logpath)
    {
        if (!fs_)
            throw std::invalid_argument("fs provided cannot be empty");
    }
    ~LoggedFileSystem() override = default;

    LoggedFileSystem(LoggedFileSystem const&)            = delete;
    LoggedFileSystem& operator=(LoggedFileSystem const&) = delete;

    LoggedFileSystem(LoggedFileSystem&&)            = delete;
    LoggedFileSystem& operator=(LoggedFileSystem&&) = delete;

    int getattr(std::filesystem::path const& path, struct stat& stbuf, struct fuse_file_info* fi) const override
    {
        out_ << "multifs: getattr, path " << path << std::endl;
        return fs_->getattr(path, stbuf, fi);
    }

    int readlink(std::filesystem::path const& path, std::span<char> buf) const override
    {
        out_ << "multifs: readlink, path " << path << ", buf " << buf.data() << ", size " << buf.size() << std::endl;
        return fs_->readlink(path, buf);
    }

    int mknod(std::filesystem::path const& path, mode_t mode, dev_t rdev) override
    {
        out_ << "multifs: mknod, path " << path << ", mode 0" << std::oct << mode << std::dec << std::endl;
        return fs_->mknod(path, mode, rdev);
    }

    int mkdir(std::filesystem::path const& path, mode_t mode) override
    {
        out_ << "multifs: mkdir, path " << path << ", mode 0" << std::oct << mode << std::dec << std::endl;
        return fs_->mkdir(path, mode);
    }

    int rmdir(std::filesystem::path const& path) override
    {
        out_ << "multifs: rmdir, path " << path << std::endl;
        return fs_->rmdir(path);
    }

    int symlink(std::filesystem::path const& from, std::filesystem::path const& to) override
    {
        out_ << "multifs: symlink, from " << from << ", to " << to << std::endl;
        return fs_->symlink(from, to);
    }

    int rename(std::filesystem::path const& from, std::filesystem::path const& to, unsigned int flags) override
    {
        out_ << "multifs: rename, from " << from << ", to " << to << ", flags 0x" << std::hex << flags << std::dec << std::endl;
        return fs_->rename(from, to, flags);
    }

    int link(std::filesystem::path const& from, std::filesystem::path const& to) override
    {
        out_ << "multifs: link, from " << from << ", to " << to << std::endl;
        return fs_->link(from, to);
    }

    int access(std::filesystem::path const& path, int mask) const override
    {
        out_ << "multifs: access, path " << path << ", mask 0" << std::oct << mask << std::dec << std::endl;
        return fs_->access(path, mask);
    }

    int readdir(
        std::filesystem::path const& path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, fuse_readdir_flags flags) const override
    {
        out_ << "multifs: readdir, path " << path << ", buf " << buf << ", off " << offset << ", fi " << fi;
        if (fi)
            out_ << ", fi->flags 0" << std::oct << fi->flags << std::dec;
        out_ << ", flags 0x" << std::hex << static_cast<int>(flags) << std::dec << std::endl;
        return fs_->readdir(path, buf, filler, offset, fi, flags);
    }

    int unlink(std::filesystem::path const& path) override
    {
        out_ << "multifs: unlink, path " << path << std::endl;
        return fs_->unlink(path);
    }

    int chmod(std::filesystem::path const& path, mode_t mode, struct fuse_file_info* fi) override
    {
        out_ << "multifs: chmod, path " << path << ", mode 0" << std::oct << mode << std::dec << ", fi " << fi;
        if (fi)
            out_ << ", fi->flags 0" << std::oct << fi->flags << std::dec;
        out_ << std::endl;
        return fs_->chmod(path, mode, fi);
    }

    int chown(std::filesystem::path const& path, uid_t uid, gid_t gid, struct fuse_file_info* fi) override
    {
        out_ << "multifs: chown, path " << path << ", uid " << uid << ", gid " << gid << ", fi " << fi;
        if (fi)
            out_ << ", fi->flags 0" << std::oct << fi->flags << std::dec;
        out_ << std::endl;
        return fs_->chown(path, uid, gid, fi);
    }

    int truncate(std::filesystem::path const& path, off_t size, struct fuse_file_info* fi) override
    {
        out_ << "multifs: truncate, path " << path << ", size " << size << ", fi " << fi;
        if (fi)
            out_ << ", fi->flags 0" << std::oct << fi->flags << std::dec;
        out_ << std::endl;
        return fs_->truncate(path, size, fi);
    }

    int open(std::filesystem::path const& path, struct fuse_file_info* fi) override
    {
        out_ << "multifs: open, path " << path << ", fi " << fi;
        if (fi)
            out_ << ", fi->flags 0" << std::oct << fi->flags << std::dec;
        out_ << std::endl;
        return fs_->open(path, fi);
    }

    int create(std::filesystem::path const& path, mode_t mode, struct fuse_file_info* fi) override
    {
        out_ << "multifs: create, path " << path << ", mode 0" << std::oct << mode << std::dec << ", fi " << fi;
        if (fi)
            out_ << ", fi->flags 0" << std::oct << fi->flags << std::dec;
        out_ << std::endl;
        return fs_->create(path, mode, fi);
    }

    ssize_t read(std::filesystem::path const& path, std::span<std::byte> buf, off_t offset, struct fuse_file_info* fi) const override
    {
        out_ << "multifs: read, path " << path << ", buf " << buf.data() << ", size " << buf.size() << ", off " << offset << ", fi " << fi;
        if (fi)
            out_ << ", fi->flags 0" << std::oct << fi->flags << std::dec;
        out_ << std::endl;
        return fs_->read(path, buf, offset, fi);
    }

    ssize_t write(std::filesystem::path const& path, std::span<std::byte const> buf, off_t offset, struct fuse_file_info* fi) override
    {
        out_ << "multifs: write, path " << path << ", buf " << buf.data() << ", size " << buf.size() << ", off " << offset << ", fi " << fi;
        if (fi)
            out_ << ", fi->flags 0" << std::oct << fi->flags << std::dec;
        out_ << std::endl;
        return fs_->write(path, buf, offset, fi);
    }

    int statfs(std::filesystem::path const& path, struct statvfs& stbuf) const override
    {
        out_ << "multifs: statfs, path " << path << ", stbuf " << &stbuf << std::endl;
        return fs_->statfs(path, stbuf);
    }

    int release(std::filesystem::path const& path, struct fuse_file_info* fi) override
    {
        out_ << "multifs: release, path " << path << ", fi " << fi;
        if (fi)
            out_ << ", fi->flags 0" << std::oct << fi->flags << std::dec;
        out_ << std::endl;
        return fs_->release(path, fi);
    }

    int fsync(std::filesystem::path const& path, int isdatasync, struct fuse_file_info* fi) override
    {
        out_ << "multifs: fsync, path " << path << ", fi " << fi;
        if (fi)
            out_ << ", fi->flags 0" << std::oct << fi->flags << std::dec;
        out_ << std::endl;
        return fs_->fsync(path, isdatasync, fi);
    }

#ifdef HAVE_UTIMENSAT
    int utimens(std::filesystem::path const& path, const struct timespec tv[2], struct fuse_file_info* fi) override
    {
        out_ << "multifs: utimens, path " << path;
        if (fi)
            out_ << ", fi->flags 0" << std::oct << fi->flags << std::dec;
        out_ << std::endl;
        return fs_->utimens(path, tv, fi);
    }
#endif // HAVE_UTIMENSAT

#ifdef HAVE_POSIX_FALLOCATE
    int fallocate(std::filesystem::path const& path, int mode, off_t offset, off_t length, struct fuse_file_info* fi) override
    {
        out_ << "multifs: fallocate, path " << path << ", mode 0" << std::oct << mode << std::dec << ", fi " << fi;
        if (fi)
            out_ << ", fi->flags 0" << std::oct << fi->flags << std::dec;
        out_ << std::endl;
        return fs_->fallocate(path, mode, offset, length, fi);
    }
#endif // HAVE_POSIX_FALLOCATE

    off_t lseek(std::filesystem::path const& path, off_t off, int whence, struct fuse_file_info* fi) const override
    {
        out_ << "multifs: lseek, path " << path << std::endl;
        return fs_->lseek(path, off, whence, fi);
    }
};

} // namespace multifs
