#pragma once

#include <cstddef>

#include <filesystem>
#include <list>
#include <memory>
#include <span>
#include <utility>
#include <vector>

#include <sys/stat.h>
#include <sys/types.h>

#include <fuse.h>

#include "file_system_interface.hpp"

namespace multifs
{

class File
{
public:
    struct Descriptor {
        size_t size;
        uid_t owner_uid;
        gid_t owner_gid;
        mode_t mode;
        int flags;
        mutable timespec atime;
        timespec mtime;
        timespec ctime;
    };

private:
    struct Chunk {
        std::pair<size_t, size_t> offset_range;
        std::shared_ptr<IFileSystem> fs;
    };

    std::filesystem::path path_;
    std::list<std::shared_ptr<IFileSystem>> fss_;
    std::list<std::shared_ptr<IFileSystem>>::iterator fs_next_it_;
    std::vector<Chunk> chunks_;
    Descriptor desc_;

    void init_desc(mode_t mode, struct fuse_file_info* fi) noexcept;
    void truncate(size_t new_size) noexcept;

public:
    File() = default;
    template <typename InputIterator>
    explicit File(std::filesystem::path path, mode_t mode, InputIterator begin, InputIterator end, struct fuse_file_info* fi)
        : path_(std::move(path))
        , fss_(begin, end)
        , fs_next_it_(fss_.begin())
    {
        init_desc(mode, fi);
    }
    ~File() = default;

    File(File const&)            = default;
    File& operator=(File const&) = default;

    File(File&&) noexcept            = default;
    File& operator=(File&&) noexcept = default;

    [[nodiscard]] auto const& desc() const noexcept { return desc_; }

    int unlink();
    int chmod(mode_t mode, struct fuse_file_info* fi) noexcept;
    int chown(uid_t uid, gid_t gid, struct fuse_file_info* fi) noexcept;
    int truncate(size_t new_size, struct fuse_file_info* fi);
    int open(struct fuse_file_info* fi);
    ssize_t write(std::span<std::byte const> buf, off_t offset, struct fuse_file_info* fi);
    ssize_t read(std::span<std::byte> buf, off_t offset, struct fuse_file_info* fi) const noexcept;
    int release(struct fuse_file_info* fi) noexcept;
    int fsync(int isdatasync, struct fuse_file_info* fi) noexcept;

#ifdef HAVE_UTIMENSAT
    int utimens(const struct timespec ts[2], struct fuse_file_info* fi) noexcept;
#endif

#ifdef HAVE_POSIX_FALLOCATE
    off_t fallocate(int mode, off_t offset, off_t length, struct fuse_file_info* fi);
#endif

    off_t lseek(off_t off, int whence, struct fuse_file_info* fi) const noexcept;
};

} // namespace multifs
