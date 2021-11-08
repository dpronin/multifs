#pragma once

#include <cerrno>
#include <cstddef>

#include <sys/types.h>

#include "file.hpp"
#include "symlink.hpp"
#include "utilities.hpp"

extern "C" {
#include "fuse.h"
} // extern "C"

namespace multifs
{

class INodeReader
{
private:
    char* buf_;
    size_t size_;
    off_t offset_;
    struct fuse_file_info* fi_;

public:
    explicit INodeReader(char* buf, size_t size, off_t offset, struct fuse_file_info* fi) noexcept
        : buf_(buf)
        , size_(size)
        , offset_(offset)
        , fi_(fi)
    {
    }

    ssize_t operator()(File const& file) const noexcept { return file.read(buf_, size_, offset_, fi_); }
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

} // namespace multifs
