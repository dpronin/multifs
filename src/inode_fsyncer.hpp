#pragma once

#include <cerrno>

#include "file.hpp"
#include "symlink.hpp"
#include "utilities.hpp"

extern "C" {
#include "fuse.h"
} // extern "C"

namespace multifs
{

class INodeFsyncer
{
private:
    int isdatasync_;
    struct fuse_file_info* fi_;

public:
    explicit INodeFsyncer(int isdatasync, struct fuse_file_info* fi) noexcept
        : isdatasync_(isdatasync)
        , fi_(fi)
    {
    }

    int operator()(File& file) const noexcept { return file.fsync(isdatasync_, fi_); }
    int operator()(Symlink&) const noexcept
    {
        // symlinks cannot be fsync-ed
        return -EINVAL;
    }

    template <typename T>
    int operator()(T&&) const noexcept
    {
        static_assert(dependent_false_v<T>, "unhandled type T to handle 'fsync'");
        return 0;
    }
};

} // namespace multifs
