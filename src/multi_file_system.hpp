#pragma once

#include "file_system_interface.hpp"

#include <sys/stat.h>
#include <sys/types.h>

#include <list>
#include <memory>
#include <unordered_map>
#include <variant>

#include "file.hpp"
#include "symlink.hpp"

namespace multifs
{

class MultiFileSystem final : public IFileSystem
{
private:
    uid_t owner_uid_;
    gid_t owner_gid_;
    std::list<std::shared_ptr<IFileSystem>> fss_;

    using INode = std::variant<File, Symlink>;
    std::unordered_map<std::string, std::shared_ptr<INode>> inodes_;

    struct statvfs statvfs_;

    void statvs_init() noexcept;

public:
    template <typename InputIt>
    explicit MultiFileSystem(uid_t owner_uid, gid_t owner_gid, InputIt begin, InputIt end)
        : owner_uid_(owner_uid)
        , owner_gid_(owner_gid)
        , fss_(begin, end)
    {
        statvs_init();
    }
    ~MultiFileSystem() override = default;

    MultiFileSystem(MultiFileSystem const&)            = delete;
    MultiFileSystem& operator=(MultiFileSystem const&) = delete;

    MultiFileSystem(MultiFileSystem&&) noexcept            = default;
    MultiFileSystem& operator=(MultiFileSystem&&) noexcept = default;

    int getattr(char const* path, struct stat* stbuf, struct fuse_file_info* fi) const override;
    int readlink(char const* path, char* buf, size_t size) const override;
    int mknod(char const* path, mode_t mode, dev_t rdev) override;
    int mkdir(char const* path, mode_t mode) override;
    int rmdir(char const* path) override;
    int symlink(char const* from, char const* to) override;
    int rename(char const* from, char const* to, unsigned int flags) override;
    int link(char const* from, char const* to) override;
    int access(char const* path, int mask) const override;
    int readdir(char const* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, fuse_readdir_flags flags) const override;
    int unlink(char const* path) override;
    int chmod(char const* path, mode_t mode, struct fuse_file_info* fi) override;
    int chown(char const* path, uid_t uid, gid_t gid, struct fuse_file_info* fi) override;
    int truncate(char const* path, off_t size, struct fuse_file_info* fi) override;
    int open(char const* path, struct fuse_file_info* fi) override;
    int create(char const* path, mode_t mode, struct fuse_file_info* fi) override;
    ssize_t read(char const* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi) const override;
    ssize_t write(char const* path, char const* buf, size_t size, off_t offset, struct fuse_file_info* fi) override;
    int statfs(char const* path, struct statvfs* stbuf) const override;
    int release(char const* path, struct fuse_file_info* fi) override;
    int fsync(char const* path, int isdatasync, struct fuse_file_info* fi) override;
#ifdef HAVE_UTIMENSAT
    int utimens(char const* path, const struct timespec ts[2], struct fuse_file_info* fi) override;
#endif // HAVE_UTIMENSAT
#ifdef HAVE_POSIX_FALLOCATE
    int fallocate(char const* path, int mode, off_t offset, off_t length, struct fuse_file_info* fi) override;
#endif // HAVE_POSIX_FALLOCATE
    off_t lseek(char const* path, off_t off, int whence, struct fuse_file_info* fi) const override;
};

} // namespace multifs
