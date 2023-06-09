#pragma once

#include <cerrno>

#include "file.hpp"
#include "symlink.hpp"
#include "utilities.hpp"

extern "C" {
#include "fuse.h"
} // extern "C"

namespace multifs::inode
{

class INodeUtimenser
{
private:
    struct timespec const* ts_;
    struct fuse_file_info* fi_;

public:
    explicit INodeUtimenser(struct timespec const ts[2], struct fuse_file_info* fi) noexcept
        : ts_(ts)
        , fi_(fi)
    {
    }

    int operator()(File& file) const noexcept { return file.utimens(ts_, fi_); }
    int operator()(Symlink& lnk) const noexcept { return lnk.utimens(ts_); }

    template <typename T>
    int operator()(T&&) const noexcept
    {
        static_assert(dependent_false_v<T>, "unhandled type T to handle 'utimens'");
        return 0;
    }
};

} // namespace multifs::inode
