#include "multifs.hpp"

#include <cstdlib>

#include <memory>
#include <utility>

#include <unistd.h>

#include <fuse.h>

#include "file_system_interface.hpp"
#include "file_system_noexcept.hpp"
#include "file_system_noexcept_interface.hpp"
#include "file_system_reflector.hpp"
#include "fs_factory_interface.hpp"
#include "fs_reflector_factory.hpp"
#include "logged_file_system.hpp"
#include "multi_file_system.hpp"
#include "multi_fs_factory.hpp"

namespace multifs
{

namespace
{

std::unique_ptr<IFSFactory> make_fs_factory()
{
    std::unique_ptr<IFSFactory> fsf;
    if (auto const& mpts = multifs::instance().mpts(); 1 == mpts.size())
        fsf = std::make_unique<FSReflectorFactory>(mpts.front());
    else
        fsf = std::make_unique<MultiFSFactory>(getuid(), getgid(), mpts.begin(), mpts.end());
    return fsf;
}

auto* make_fs(std::unique_ptr<IFileSystem> fs) noexcept { return new FileSystemNoexcept(std::move(fs)); }

auto& to_fs(void* private_data) noexcept { return *static_cast<FileSystemNoexcept*>(private_data); }

auto& get_fs() noexcept { return to_fs(fuse_get_context()->private_data); }

int getattr(char const* path, struct stat* stbuf, struct fuse_file_info* fi) noexcept { return get_fs().getattr(path, stbuf, fi); }

int readlink(char const* path, char* buf, size_t size) noexcept { return get_fs().readlink(path, buf, size); }

int mknod(char const* path, mode_t mode, dev_t rdev) noexcept { return get_fs().mknod(path, mode, rdev); }

int mkdir(char const* path, mode_t mode) noexcept { return get_fs().mkdir(path, mode); }

int unlink(char const* path) noexcept { return get_fs().unlink(path); }

int rmdir(char const* path) noexcept { return get_fs().rmdir(path); }

int symlink(char const* from, char const* to) noexcept { return get_fs().symlink(from, to); }

int rename(char const* from, char const* to, unsigned int flags) noexcept { return get_fs().rename(from, to, flags); }

int link(char const* from, char const* to) noexcept { return get_fs().link(from, to); }

int chmod(char const* path, mode_t mode, struct fuse_file_info* fi) noexcept { return get_fs().chmod(path, mode, fi); }

int chown(char const* path, uid_t uid, gid_t gid, struct fuse_file_info* fi) noexcept { return get_fs().chown(path, uid, gid, fi); }

int truncate(char const* path, off_t size, struct fuse_file_info* fi) noexcept { return get_fs().truncate(path, size, fi); }

int open(char const* path, struct fuse_file_info* fi) noexcept { return get_fs().open(path, fi); }

int read(char const* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi) noexcept
{
    return static_cast<int>(get_fs().read(path, buf, size, offset, fi));
}

int write(char const* path, char const* buf, size_t size, off_t offset, struct fuse_file_info* fi) noexcept
{
    return static_cast<int>(get_fs().write(path, buf, size, offset, fi));
}

int statfs(char const* path, struct statvfs* stbuf) noexcept { return get_fs().statfs(path, stbuf); }

int release(char const* path, struct fuse_file_info* fi) noexcept { return get_fs().release(path, fi); }

int fsync(char const* path, int isdatasync, struct fuse_file_info* fi) noexcept { return get_fs().fsync(path, isdatasync, fi); }

int readdir(char const* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, fuse_readdir_flags flags) noexcept
{
    return get_fs().readdir(path, buf, filler, offset, fi, flags);
}

void* init(struct fuse_conn_info* conn, struct fuse_config* cfg) noexcept
{
    // #ifndef NDEBUG
    //     out << "init: debug " << cfg->debug << std::endl;
    // #endif

    cfg->kernel_cache = 1;

    auto fs = make_fs_factory()->create_unique();
    if (auto const& logp = multifs::instance().logp(); !logp.empty())
        fs = std::make_unique<LoggedFileSystem>(std::move(fs), logp);

    return make_fs(std::move(fs));
}

void destroy(void* private_data)
{
    // #ifndef NDEBUG
    //     out << "destroy: private_data " << private_data << std::endl;
    // #endif

    delete &to_fs(private_data);
}

int access(char const* path, int mask) noexcept { return get_fs().access(path, mask); }

int create(char const* path, mode_t mode, struct fuse_file_info* fi) noexcept { return get_fs().create(path, mode, fi); }

#ifdef HAVE_UTIMENSAT
int utimens(char const* path, const struct timespec tv[2], struct fuse_file_info* fi) noexcept { return get_fs().utimens(path, tv, fi); }
#endif // HAVE_UTIMENSAT

#ifdef HAVE_POSIX_FALLOCATE
int fallocate(char const* path, int mode, off_t offset, off_t length, struct fuse_file_info* fi) noexcept
{
    return get_fs().fallocate(path, mode, offset, length, fi);
}
#endif // HAVE_POSIX_FALLOCATE

off_t lseek(char const* path, off_t off, int whence, struct fuse_file_info* fi) noexcept { return get_fs().lseek(path, off, whence, fi); }

} // namespace

fuse_operations const& getops() noexcept
{
    static fuse_operations const ops = {
        .getattr  = getattr,
        .readlink = readlink,
        .mknod    = mknod,
        .mkdir    = mkdir,
        .unlink   = unlink,
        .rmdir    = rmdir,
        .symlink  = symlink,
        .rename   = rename,
        .link     = link,
        .chmod    = chmod,
        .chown    = chown,
        .truncate = truncate,
        .open     = open,
        .read     = read,
        .write    = write,
        .statfs   = statfs,
        .release  = release,
        .fsync    = fsync,
        .readdir  = readdir,
        .init     = init,
        .destroy  = destroy,
        .access   = access,
        .create   = create,
#ifdef HAVE_UTIMENSAT
        .utimens = utimens,
#endif
#ifdef HAVE_POSIX_FALLOCATE
        .fallocate = fallocate,
#endif
        // #ifdef HAVE_SETXATTR
        //     .setxattr = setxattr,
        //     .getxattr = getxattr,
        //     .listxattr = listxattr,
        //     .removexattr = removexattr,
        // #endif
        // #ifdef HAVE_COPY_FILE_RANGE
        //     .copy_file_range = copy_file_range,
        // #endif
        .lseek = lseek,
    };
    return ops;
}

} // namespace multifs
