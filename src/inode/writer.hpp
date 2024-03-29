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

class Writer
{
private:
    std::span<std::byte const> buf_;
    off_t offset_;
    struct fuse_file_info* fi_;

public:
    explicit Writer(std::span<std::byte const> buf, off_t offset, struct fuse_file_info* fi) noexcept
        : buf_(buf)
        , offset_(offset)
        , fi_(fi)
    {
        assert(!buf.empty());
    }

    ssize_t operator()(File& file) const noexcept { return file.write(buf_, offset_, fi_); }
    ssize_t operator()(Symlink&) const noexcept
    {
        // writing to symlinks is impossible
        return -EINVAL;
    }

    template <typename T>
    ssize_t operator()(T&&) const noexcept
    {
        static_assert(dependent_false_v<T>, "unhandled type T to handle 'write'");
        return 0;
    }
};

} // namespace multifs::inode
