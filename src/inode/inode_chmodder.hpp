#pragma once

#include <sys/types.h>

#include <fuse.h>

#include "file.hpp"
#include "symlink.hpp"
#include "utilities.hpp"

namespace multifs::inode
{

class INodeChmodder
{
private:
    mode_t mode_;
    struct fuse_file_info* fi_;

public:
    explicit INodeChmodder(mode_t mode, struct fuse_file_info* fi) noexcept
        : mode_(mode)
        , fi_(fi)
    {
    }

    int operator()(File& file) const noexcept { return file.chmod(mode_, fi_); }
    int operator()(Symlink&) const noexcept
    {
        // you cannot change mode of the symlink, but we must return 0 in this case as if it has been successful
        return 0;
    }

    template <typename T>
    int operator()(T&&) const noexcept
    {
        static_assert(dependent_false_v<T>, "unhandled type T to handle 'chmod'");
        return 0;
    }
};

} // namespace multifs::inode
