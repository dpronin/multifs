#pragma once

#include <cassert>
#include <cerrno>
#include <cstddef>

#include <sys/types.h>

#include <fuse.h>

#include <span>

#include "file.hpp"
#include "symlink.hpp"
#include "utilities.hpp"

namespace multifs::inode
{

class Reader
{
private:
    std::span<std::byte> buf_;
    off_t offset_;
    struct fuse_file_info* fi_;

public:
    explicit Reader(std::span<std::byte> buf, off_t offset, struct fuse_file_info* fi) noexcept
        : buf_(buf)
        , offset_(offset)
        , fi_(fi)
    {
        assert(!buf_.empty());
    }

    ssize_t operator()(File const& file) const noexcept { return file.read(buf_, offset_, fi_); }
    ssize_t operator()(Symlink const&) const noexcept
    {
        // reading symlinks is impossible, their content must be read by readlink call
        return -EINVAL;
    }

    template <typename T>
    ssize_t operator()(T const&) const noexcept
    {
        static_assert(dependent_false_v<T>, "unhandled type T to handle 'read'");
        return 0;
    }
};

} // namespace multifs::inode
