#pragma once

#include "file_system_interface.hpp"

#include <sys/stat.h>
#include <sys/types.h>

#include <list>
#include <memory>
#include <unordered_map>
#include <utility>
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

    template <typename Range>
    explicit MultiFileSystem(uid_t owner_uid, gid_t owner_gid, Range&& range)
        : MultiFileSystem(owner_uid, owner_gid, std::begin(std::forward<Range>(range)), std::end(std::forward<Range>(range)))
    {
    }

    ~MultiFileSystem() override = default;

    MultiFileSystem(MultiFileSystem const&)            = delete;
    MultiFileSystem& operator=(MultiFileSystem const&) = delete;

    MultiFileSystem(MultiFileSystem&&) noexcept            = default;
    MultiFileSystem& operator=(MultiFileSystem&&) noexcept = default;

    int getattr(std::filesystem::path const& path, struct stat& stbuf, struct fuse_file_info* fi) const override;
    int readlink(std::filesystem::path const& path, std::span<char>) const override;
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
