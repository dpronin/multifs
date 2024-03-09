#include "file_system_reflector.hpp"

#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <dirent.h>
#include <fcntl.h>
#include <filesystem>
#include <sys/stat.h>
#include <unistd.h>

#include <stdexcept>
#include <utility>

#include <fuse.h>

#include "passthrough_helpers.hpp"

using namespace multifs;

FileSystemReflector::FileSystemReflector(std::filesystem::path mount_point)
    : mp_(std::move(mount_point))
{
    if (!mp_.is_absolute())
        throw std::invalid_argument("mount point provided must be an absolute");
    if (!std::filesystem::is_directory(mp_))
        throw std::invalid_argument("mount point provided must be a path to a directory as a mount point");
}

int FileSystemReflector::getattr(std::filesystem::path const& path, struct stat& stbuf, struct fuse_file_info* /*fi*/) const
{
    assert(!path.empty());

    auto const res = ::lstat(to_path(path).c_str(), &stbuf);

    return res == -1 ? -errno : 0;
}

int FileSystemReflector::readlink(std::filesystem::path const& path, std::span<char> buf) const
{
    assert(!path.empty());
    assert(!buf.empty());

    auto const res = ::readlink(to_path(path).c_str(), buf.data(), buf.size() - 1);
    if (res == -1)
        return -errno;

    buf[res] = '\0';

    return 0;
}

int FileSystemReflector::mknod(std::filesystem::path const& path, mode_t mode, dev_t rdev)
{
    assert(!path.empty());

    auto const res = mknod_wrapper(AT_FDCWD, to_path(path).c_str(), nullptr, mode, rdev);

    return res == -1 ? -errno : 0;
}

int FileSystemReflector::mkdir(std::filesystem::path const& path, mode_t mode)
{
    assert(!path.empty());

    auto const res = ::mkdir(to_path(path).c_str(), mode);

    return res == -1 ? -errno : 0;
}

int FileSystemReflector::rmdir(std::filesystem::path const& path)
{
    assert(!path.empty());

    auto const res = ::rmdir(to_path(path).c_str());

    return res == -1 ? -errno : 0;
}

int FileSystemReflector::symlink(std::filesystem::path const& from, std::filesystem::path const& to)
{
    assert(!from.empty());
    assert(!to.empty());

    auto const res = ::symlink(to_path(from).c_str(), to_path(to).c_str());

    return res == -1 ? -errno : 0;
}

int FileSystemReflector::rename(std::filesystem::path const& from, std::filesystem::path const& to, unsigned int flags)
{
    assert(!from.empty());
    assert(!to.empty());

    if (flags)
        return -EINVAL;

    auto const res = ::rename(to_path(from).c_str(), to_path(to).c_str());

    return res == -1 ? -errno : 0;
}

int FileSystemReflector::link(std::filesystem::path const& from, std::filesystem::path const& to)
{
    assert(!from.empty());
    assert(!to.empty());

    auto const res = ::link(to_path(from).c_str(), to_path(to).c_str());

    return res == -1 ? -errno : 0;
}

int FileSystemReflector::access(std::filesystem::path const& path, int mask) const
{
    assert(!path.empty());

    auto const res = ::access(to_path(path).c_str(), mask);

    return res == -1 ? -errno : 0;
}

int FileSystemReflector::readdir(
    std::filesystem::path const& path, void* buf, fuse_fill_dir_t filler, off_t /*offset*/, struct fuse_file_info* /*fi*/, fuse_readdir_flags /*flags*/) const
{
    assert(!path.empty());
    assert(buf);

    auto* dp = ::opendir(to_path(path).c_str());
    if (!dp)
        return -errno;

    for (struct dirent* de; (de = ::readdir(dp));) {
        struct stat st;
        std::memset(&st, 0, sizeof(st));
        st.st_ino  = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0, FUSE_FILL_DIR_PLUS))
            break;
    }

    ::closedir(dp);

    return 0;
}

int FileSystemReflector::unlink(std::filesystem::path const& path)
{
    assert(!path.empty());

    auto const res = ::unlink(to_path(path).c_str());

    return res == -1 ? -errno : 0;
}

int FileSystemReflector::chmod(std::filesystem::path const& path, mode_t mode, struct fuse_file_info* /*fi*/)
{
    assert(!path.empty());

    auto const res = ::chmod(to_path(path).c_str(), mode);

    return res == -1 ? -errno : 0;
}

int FileSystemReflector::chown(std::filesystem::path const& path, uid_t uid, gid_t gid, struct fuse_file_info* /*fi*/)
{
    assert(!path.empty());

    auto const res = ::lchown(to_path(path).c_str(), uid, gid);

    return res == -1 ? -errno : 0;
}

int FileSystemReflector::truncate(std::filesystem::path const& path, off_t size, struct fuse_file_info* fi)
{
    assert(!path.empty());

    auto const res = fi ? ::ftruncate(fi->fh, size) : ::truncate(to_path(path).c_str(), size);

    return res == -1 ? -errno : 0;
}

int FileSystemReflector::open(std::filesystem::path const& path, struct fuse_file_info* fi)
{
    assert(!path.empty());
    assert(fi);

    auto const res = ::open(to_path(path).c_str(), fi->flags);
    if (res == -1)
        return -errno;

    fi->fh = res;

    return 0;
}

int FileSystemReflector::create(std::filesystem::path const& path, mode_t mode, struct fuse_file_info* fi)
{
    assert(!path.empty());
    assert(fi);

    auto const res = ::open(to_path(path).c_str(), fi->flags | O_CREAT, mode);
    if (res == -1)
        return -errno;

    fi->fh = res;

    return 0;
}

ssize_t FileSystemReflector::read(std::filesystem::path const& path, std::span<std::byte> buf, off_t offset, struct fuse_file_info* fi) const
{
    assert(!path.empty());
    assert(!buf.empty());

    auto const fd = fi ? fi->fh : ::open(to_path(path).c_str(), O_RDONLY);
    if (fd == -1)
        return -errno;

    std::byte* p_buf{buf.data()};
    if (fi && (fi->flags & O_DIRECT))
        p_buf = reinterpret_cast<std::byte*>(std::aligned_alloc(512, buf.size()));

    auto res = ::pread(fd, p_buf, buf.size(), offset);
    if (res == -1)
        res = -errno;

    if (fi && (fi->flags & O_DIRECT)) {
        std::memcpy(buf.data(), p_buf, buf.size());
        std::free(p_buf);
    }

    if (!fi)
        ::close(fd);

    return res;
}

ssize_t FileSystemReflector::write(std::filesystem::path const& path, std::span<std::byte const> buf, off_t offset, struct fuse_file_info* fi)
{
    assert(!path.empty());
    assert(!buf.empty());

    auto const fd = fi ? fi->fh : ::open(to_path(path).c_str(), O_WRONLY);
    if (fd == -1)
        return -errno;

    std::byte const* p_buf{buf.data()};
    std::byte* aligned_buf{nullptr};
    if (fi && (fi->flags & O_DIRECT)) {
        aligned_buf = reinterpret_cast<std::byte*>(std::aligned_alloc(512, buf.size()));
        std::memcpy(aligned_buf, buf.data(), buf.size());
        p_buf = aligned_buf;
    }

    auto res = ::pwrite(fd, p_buf, buf.size(), offset);
    if (res == -1)
        res = -errno;

    if (fi && (fi->flags & O_DIRECT))
        std::free(aligned_buf);

    if (!fi)
        ::close(fd);

    return res;
}

int FileSystemReflector::statfs(std::filesystem::path const& path, struct statvfs& stbuf) const
{
    assert(!path.empty());

    auto const res = ::statvfs(to_path(path).c_str(), &stbuf);

    return res == -1 ? -errno : 0;
}

int FileSystemReflector::release(std::filesystem::path const& path [[maybe_unused]], struct fuse_file_info* fi)
{
    assert(fi);

    ::close(fi->fh);

    fi->fh = 0x0;

    return 0;
}

int FileSystemReflector::fsync(std::filesystem::path const& path, int /*isdatasync*/, struct fuse_file_info* fi)
{
    assert(!path.empty());

    auto const fd = fi ? fi->fh : ::open(to_path(path).c_str(), O_WRONLY);
    if (fd == -1)
        return -errno;

    auto res = ::fsync(fd);
    if (res == -1)
        res = -errno;

    if (!fi)
        ::close(fd);

    return 0;
}

#ifdef HAVE_UTIMENSAT
int FileSystemReflector::utimens(std::filesystem::path const& path, const struct timespec ts[2], struct fuse_file_info* /*fi*/)
{
    assert(!path.empty());

    /* don't use utime/utimes since they follow symlinks */
    auto const res = utimensat(0, to_path(path).c_str(), ts, AT_SYMLINK_NOFOLLOW);

    return res == -1 ? -errno : 0;
}
#endif // HAVE_UTIMENSAT

#ifdef HAVE_POSIX_FALLOCATE
int FileSystemReflector::fallocate(std::filesystem::path const& path, int mode, off_t offset, off_t length, struct fuse_file_info* fi)
{
    assert(!path.empty());

    if (mode)
        return -EOPNOTSUPP;

    auto const fd = fi ? fi->fh : ::open(to_path(path).c_str(), O_WRONLY);
    if (fd == -1)
        return -errno;

    auto const res = -::posix_fallocate(fd, offset, length);

    if (!fi)
        ::close(fd);

    return res;
}
#endif // HAVE_POSIX_FALLOCATE

off_t FileSystemReflector::lseek(std::filesystem::path const& path, off_t off, int whence, struct fuse_file_info* fi) const
{
    assert(!path.empty());

    auto const fd = fi ? fi->fh : ::open(to_path(path).c_str(), O_RDONLY);
    if (fd == -1)
        return -errno;

    auto res = ::lseek(fd, off, whence);
    if (res == -1)
        res = -errno;

    if (!fi)
        ::close(fd);

    return res;
}
