#pragma once

#include <cerrno>
#include <cstddef>

#include <sys/types.h>

#include <fuse.h>

#include "file.hpp"
#include "symlink.hpp"
#include "utilities.hpp"

namespace multifs::inode
{

class Fallocater
{
private:
    int mode_;
    off_t offset_;
    off_t length_;
    struct fuse_file_info* fi_;

public:
    explicit Fallocater(int mode, off_t offset, off_t length, struct fuse_file_info* fi) noexcept
        : mode_(mode)
        , offset_(offset)
        , length_(length)
        , fi_(fi)
    {
    }

    int operator()(File& file) const noexcept { return file.fallocate(mode_, offset_, length_, fi_); }

    int operator()(Symlink&) const noexcept
    {
        // fallocating symlinks is impossible
        return -EINVAL;
    }

    template <typename T>
    int operator()(T&&) const noexcept
    {
        static_assert(dependent_false_v<T>, "unhandled type T to handle 'fallocate'");
        return 0;
    }
};

} // namespace multifs::inode
