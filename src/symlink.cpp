#include "symlink.hpp"

#include <cassert>

#include <utility>

#include <fuse.h>

#include "utilities.hpp"

using namespace multifs;

Symlink::Symlink(std::filesystem::path target) noexcept
    : target_(std::move(target))
{
    auto const* ctx = fuse_get_context();
    assert(ctx);
    desc_.owner_uid = ctx->uid;
    desc_.owner_gid = ctx->gid;
    desc_.atime     = current_time();
    desc_.mtime     = desc_.atime;
    desc_.ctime     = desc_.atime;
}

int Symlink::chown(uid_t uid, gid_t gid) noexcept
{
    desc_.owner_uid = uid;
    desc_.owner_gid = gid;
    desc_.ctime     = current_time();
    return 0;
}

#ifdef HAVE_UTIMENSAT
int Symlink::utimens(const struct timespec ts[2]) noexcept
{
    auto const cur_time = current_time();

    if (UTIME_NOW == ts[0].tv_nsec)
        desc_.atime = cur_time;
    else if (UTIME_OMIT != ts[0].tv_nsec)
        desc_.atime = ts[0];

    if (UTIME_NOW == ts[1].tv_nsec)
        desc_.mtime = cur_time;
    else if (UTIME_OMIT != ts[1].tv_nsec)
        desc_.mtime = ts[1];

    if (UTIME_OMIT != ts[0].tv_nsec || UTIME_OMIT != ts[1].tv_nsec)
        desc_.ctime = cur_time;

    return 0;
}
#endif
