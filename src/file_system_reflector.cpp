#include "file_system_reflector.hpp"

#include <cerrno>
#include <cstddef>
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

int FileSystemReflector::getattr(char const* path, struct stat* stbuf, struct fuse_file_info* /*fi*/) const
{
    auto const res = ::lstat(to_path(path).c_str(), stbuf);
    return res == -1 ? -errno : 0;
}

int FileSystemReflector::readlink(char const* path, char* buf, size_t size) const
{
    auto const res = ::readlink(to_path(path).c_str(), buf, size - 1);
    if (res == -1)
        return -errno;
    buf[res] = '\0';
    return 0;
}

int FileSystemReflector::mknod(char const* path, mode_t mode, dev_t rdev)
{
    auto const res = mknod_wrapper(AT_FDCWD, to_path(path).c_str(), NULL, mode, rdev);
    return res == -1 ? -errno : 0;
}

int FileSystemReflector::mkdir(char const* path, mode_t mode)
{
    auto const res = ::mkdir(to_path(path).c_str(), mode);
    return res == -1 ? -errno : 0;
}

int FileSystemReflector::rmdir(char const* path)
{
    auto const res = ::rmdir(to_path(path).c_str());
    return res == -1 ? -errno : 0;
}

int FileSystemReflector::symlink(char const* from, char const* to)
{
    auto const res = ::symlink(to_path(from).c_str(), to_path(to).c_str());
    return res == -1 ? -errno : 0;
}

int FileSystemReflector::rename(char const* from, char const* to, unsigned int flags)
{
    if (flags)
        return -EINVAL;
    auto const res = ::rename(to_path(from).c_str(), to_path(to).c_str());
    return res == -1 ? -errno : 0;
}

int FileSystemReflector::link(char const* from, char const* to)
{
    auto const res = ::link(to_path(from).c_str(), to_path(to).c_str());
    return res == -1 ? -errno : 0;
}

int FileSystemReflector::access(char const* path, int mask) const
{
    auto const res = ::access(to_path(path).c_str(), mask);
    return res == -1 ? -errno : 0;
}

int FileSystemReflector::readdir(
    char const* path, void* buf, fuse_fill_dir_t filler, off_t /*offset*/, struct fuse_file_info* /*fi*/, fuse_readdir_flags /*flags*/) const
{
    auto* dp = ::opendir(to_path(path).c_str());
    if (dp == NULL)
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

int FileSystemReflector::unlink(char const* path)
{
    auto const res = ::unlink(to_path(path).c_str());
    return res == -1 ? -errno : 0;
}

int FileSystemReflector::chmod(char const* path, mode_t mode, struct fuse_file_info* /*fi*/)
{
    auto const res = ::chmod(to_path(path).c_str(), mode);
    return res == -1 ? -errno : 0;
}

int FileSystemReflector::chown(char const* path, uid_t uid, gid_t gid, struct fuse_file_info* /*fi*/)
{
    auto const res = ::lchown(to_path(path).c_str(), uid, gid);
    return res == -1 ? -errno : 0;
}

int FileSystemReflector::truncate(char const* path, off_t size, struct fuse_file_info* fi)
{
    auto const res = fi != NULL ? ::ftruncate(fi->fh, size) : ::truncate(to_path(path).c_str(), size);
    return res == -1 ? -errno : 0;
}

int FileSystemReflector::open(char const* path, struct fuse_file_info* fi)
{
    auto const res = ::open(to_path(path).c_str(), fi->flags);
    if (res == -1)
        return -errno;
    fi->fh = res;
    return 0;
}

int FileSystemReflector::create(char const* path, mode_t mode, struct fuse_file_info* fi)
{
    auto const res = ::open(to_path(path).c_str(), fi->flags | O_CREAT, mode);
    if (res == -1)
        return -errno;
    fi->fh = res;
    return 0;
}

ssize_t FileSystemReflector::read(char const* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi) const
{
    auto const fd = fi ? fi->fh : ::open(to_path(path).c_str(), O_RDONLY);
    if (fd == -1)
        return -errno;

    auto res = ::pread(fd, buf, size, offset);
    if (res == -1)
        res = -errno;

    if (!fi)
        ::close(fd);

    return res;
}

ssize_t FileSystemReflector::write(char const* path, char const* buf, size_t size, off_t offset, struct fuse_file_info* fi)
{
    auto const fd = fi ? fi->fh : ::open(to_path(path).c_str(), O_WRONLY);
    if (fd == -1)
        return -errno;

    auto res = ::pwrite(fd, buf, size, offset);
    if (res == -1)
        res = -errno;

    if (!fi)
        ::close(fd);

    return res;
}

int FileSystemReflector::statfs(char const* path, struct statvfs* stbuf) const
{
    auto const res = ::statvfs(to_path(path).c_str(), stbuf);
    return res == -1 ? -errno : 0;
}

int FileSystemReflector::release(char const*, struct fuse_file_info* fi)
{
    ::close(fi->fh);
    fi->fh = 0;
    return 0;
}

int FileSystemReflector::fsync(char const* path, int /*isdatasync*/, struct fuse_file_info* fi)
{
    auto const fd = fi ? fi->fh : ::open(to_path(path).c_str(), O_RDWR);
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
int FileSystemReflector::utimens(char const* path, const struct timespec ts[2], struct fuse_file_info* /*fi*/)
{
    /* don't use utime/utimes since they follow symlinks */
    auto const res = utimensat(0, to_path(path).c_str(), ts, AT_SYMLINK_NOFOLLOW);
    return res == -1 ? -errno : 0;
}
#endif // HAVE_UTIMENSAT

#ifdef HAVE_POSIX_FALLOCATE
int FileSystemReflector::fallocate(char const* path, int mode, off_t offset, off_t length, struct fuse_file_info* fi)
{
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

off_t FileSystemReflector::lseek(char const* path, off_t off, int whence, struct fuse_file_info* fi) const
{
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
