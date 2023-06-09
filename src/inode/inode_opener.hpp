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

class INodeOpener
{
private:
    struct fuse_file_info* fi_;

public:
    explicit INodeOpener(struct fuse_file_info* fi) noexcept
        : fi_(fi)
    {
    }

    int operator()(File& file) const { return file.open(fi_); }
    int operator()(Symlink&) const noexcept
    {
        // symlinks cannot be opened
        return -EINVAL;
    }

    template <typename T>
    int operator()(T&&) const noexcept
    {
        static_assert(dependent_false_v<T>, "unhandled type T to handle 'open'");
        return 0;
    }
};

} // namespace multifs
