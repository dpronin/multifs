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

class INodeReleaser
{
private:
    struct fuse_file_info* fi_;

public:
    explicit INodeReleaser(struct fuse_file_info* fi) noexcept
        : fi_(fi)
    {
    }

    int operator()(File& file) const { return file.release(fi_); }
    int operator()(Symlink&) const noexcept
    {
        // symlinks cannot be released
        return -EINVAL;
    }

    template <typename T>
    int operator()(T&&) const noexcept
    {
        static_assert(dependent_false_v<T>, "unhandled type T to handle 'release'");
        return 0;
    }
};

} // namespace multifs
