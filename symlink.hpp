#pragma once

#include <cstddef>

#include <filesystem>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "file_system_interface.hpp"
#include "utilities.hpp"

extern "C" {
#include "fuse.h"
} // extern "C"

namespace multifs
{

class Symlink
{
public:
    struct Descriptor {
        mode_t mode{S_IFLNK | 0777};
        uid_t owner_uid;
        gid_t owner_gid;
        mutable timespec atime;
        timespec mtime;
        timespec ctime;
    };

private:
    std::filesystem::path target_;

    Descriptor desc_;

public:
    Symlink() = default;
    explicit Symlink(std::filesystem::path target);

    Symlink(Symlink const&) = default;
    Symlink& operator=(Symlink const&) = default;

    Symlink(Symlink&&) noexcept = default;
    Symlink& operator=(Symlink&&) noexcept = default;

    [[nodiscard]] auto const& target() const noexcept { return target_; }
    [[nodiscard]] auto const& desc() const noexcept { return desc_; }

    int chown(uid_t uid, gid_t gid, struct fuse_file_info* fi) noexcept;

    int utimens(const struct timespec ts[2], struct fuse_file_info* fi) noexcept;
};

} // namespace multifs
