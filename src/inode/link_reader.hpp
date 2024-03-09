#pragma once

#include <cassert>
#include <cerrno>
#include <cstring>

#include <span>

#include <fuse.h>

#include "file.hpp"
#include "symlink.hpp"
#include "utilities.hpp"

namespace multifs::inode
{

class LinkReader
{
private:
    std::span<char> buf_;

public:
    explicit LinkReader(std::span<char> buf) noexcept
        : buf_(buf)
    {
        assert(!buf_.empty());
    }

    int operator()(File const&) const noexcept
    {
        // reading generic files as symlinks is impossible
        return -EINVAL;
    }

    int operator()(Symlink const& lnk) const noexcept
    {
        std::strncpy(buf_.data(), lnk.target().c_str(), buf_.size());
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
