#pragma once

#include <sys/types.h>

#include <fuse.h>

#include "file.hpp"
#include "symlink.hpp"
#include "utilities.hpp"

namespace multifs::inode
{

class Chowner
{
private:
    uid_t uid_;
    gid_t gid_;
    struct fuse_file_info* fi_;

public:
    explicit Chowner(uid_t uid, gid_t gid, struct fuse_file_info* fi) noexcept
        : uid_(uid)
        , gid_(gid)
        , fi_(fi)
    {
    }

    int operator()(File& file) const noexcept { return file.chown(uid_, gid_, fi_); }
    int operator()(Symlink& lnk) const noexcept { return lnk.chown(uid_, gid_); }

    template <typename T>
    int operator()(T&&) const noexcept
    {
        static_assert(dependent_false_v<T>, "unhandled type T to handle 'chown'");
        return 0;
    }
};

} // namespace multifs::inode
