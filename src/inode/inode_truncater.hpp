#pragma once

#include <cerrno>

#include <sys/stat.h>

#include "file.hpp"
#include "symlink.hpp"
#include "utilities.hpp"

extern "C" {
#include "fuse.h"
} // extern "C"

namespace multifs
{

class INodeTruncater
{
private:
    off_t new_size_;
    struct fuse_file_info* fi_;

public:
    explicit INodeTruncater(off_t new_size, struct fuse_file_info* fi) noexcept
        : new_size_(new_size)
        , fi_(fi)
    {
    }

    int operator()(File& file) const { return file.truncate(new_size_, fi_); }
    int operator()(Symlink&) const noexcept
    {
        // symbolic links cannot be truncated
        return -EINVAL;
    }

    template <typename T>
    int operator()(T&&) const noexcept
    {
        static_assert(dependent_false_v<T>, "unhandled type T to handle 'truncate'");
        return 0;
    }
};

} // namespace multifs
