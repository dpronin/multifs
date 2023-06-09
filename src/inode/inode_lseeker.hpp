#pragma once

#include <cerrno>
#include <cstddef>

#include <sys/stat.h>

#include "file.hpp"
#include "symlink.hpp"
#include "utilities.hpp"

extern "C" {
#include "fuse.h"
} // extern "C"

namespace multifs
{

class INodeLseeker
{
private:
    off_t off_;
    int whence_;
    struct fuse_file_info* fi_;

public:
    explicit INodeLseeker(off_t off, int whence, struct fuse_file_info* fi) noexcept
        : off_(off)
        , whence_(whence)
        , fi_(fi)
    {
    }

    off_t operator()(File const& file) const noexcept { return file.lseek(off_, whence_, fi_); }

    off_t operator()(Symlink const&) const noexcept
    {
        // lseek symlinks is impossible
        return -EINVAL;
    }

    template <typename T>
    off_t operator()(T const&) const noexcept
    {
        static_assert(dependent_false_v<T>, "unhandled type T to handle 'lseek'");
        return 0;
    }
};

} // namespace multifs
