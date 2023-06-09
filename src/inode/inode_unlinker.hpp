#pragma once

#include "file.hpp"
#include "symlink.hpp"
#include "utilities.hpp"

namespace multifs::inode
{

class INodeUnlinker
{
public:
    int operator()(File& file) const { return file.unlink(); }
    int operator()(Symlink&) const noexcept { return 0; }

    template <typename T>
    int operator()(T&&) const noexcept
    {
        static_assert(dependent_false_v<T>, "unhandled type T to handle 'unlink'");
        return 0;
    }
};

} // namespace multifs::inode
