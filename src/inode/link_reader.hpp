#pragma once

#include <cerrno>
#include <cstddef>

#include <fuse.h>

#include "file.hpp"
#include "symlink.hpp"
#include "utilities.hpp"

namespace multifs::inode
{

class LinkReader
{
private:
    char* buf_;
    size_t size_;

public:
    explicit LinkReader(char* buf, size_t size) noexcept
        : buf_(buf)
        , size_(size)
    {
    }

    int operator()(File const&) const noexcept
    {
        // reading generic files as symlinks is impossible
        return -EINVAL;
    }

    int operator()(Symlink const& lnk) const noexcept
    {
        std::strncpy(buf_, lnk.target().c_str(), size_);
        return 0;
    }

    template <typename T>
    int operator()(T const&) const noexcept
    {
        static_assert(dependent_false_v<T>, "unhandled type T to handle 'readlink'");
        return 0;
    }
};

} // namespace multifs::inode
