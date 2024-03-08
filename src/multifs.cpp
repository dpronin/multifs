#include "multifs.hpp"

#include <cassert>

#include <algorithm>
#include <array>
#include <iostream>
#include <memory>
#include <ranges>
#include <utility>

#include <unistd.h>

#include <fuse.h>

#include "boost/lockfree/detail/prefix.hpp"

#include "file_system_interface.hpp"
#include "file_system_noexcept.hpp"
#include "file_system_reflector.hpp"
#include "logged_file_system.hpp"
#include "multi_file_system.hpp"
#include "thread_safe_access_file_system.hpp"

namespace multifs
{

namespace
{

std::array<std::byte, sizeof(FileSystemNoexcept)> __fsmem_layout__ alignas(BOOST_LOCKFREE_CACHELINE_BYTES);

void show_help(std::string_view progname)
{
    std::cout << "usage: " << progname << " [options] <mountpoint>\n\n";
    std::cout << "Multi File-system specific options:\n"
              << "    --fss=<path1>:<path2>:<path3>:...    paths to mount points to "
                 "combine them within the multifs\n"
              << "    --log=<path>                         path to a file where multifs "
                 "will log operations\n"
              << "\n";
    std::cout.flush();
}

auto make_absolute_normal(std::filesystem::path const& path) { return std::filesystem::absolute(path).lexically_normal(); }

std::unique_ptr<IFileSystem> make_bfs(app_params const& params)
{
    std::unique_ptr<IFileSystem> fs;
    std::list<std::unique_ptr<IFileSystem>> fss;
    std::ranges::transform(params.mpts, std::back_inserter(fss), [](auto const& mp) {
        return std::make_unique<FileSystemReflector>(make_absolute_normal(mp));
    });

    bool need_thread_safety = false;

    if (1 == params.mpts.size()) {
        fs = std::move(fss.front());
    } else {
        fs = std::make_unique<MultiFileSystem>(getuid(), getgid(), std::make_move_iterator(fss.begin()), std::make_move_iterator(fss.end()));
        need_thread_safety = true;
    }

    if (auto const& logp = params.logp; !logp.empty()) {
        fs = std::make_unique<LoggedFileSystem>(std::move(fs), logp);
        need_thread_safety = true;
    }

    if (need_thread_safety)
        fs = std::make_unique<ThreadSafeAccessFileSystem>(std::move(fs));

    return fs;
}

void destroy_fs_noexcept(FileSystemNoexcept* fs) noexcept
{
    static_assert(std::is_nothrow_destructible_v<FileSystemNoexcept>, "FileSystemNoexcept must be nothrow-destructible");
    fs->~FileSystemNoexcept();
}

auto make_fs_noexcept(std::unique_ptr<IFileSystem> fs)
{
    return std::unique_ptr<FileSystemNoexcept, decltype(&destroy_fs_noexcept)>(
        new (__fsmem_layout__.data()) FileSystemNoexcept(std::move(fs)),
        destroy_fs_noexcept);
}

auto* fuse_private_data_ptr() noexcept { return fuse_get_context()->private_data; }

auto* to_fs_noexcept_ptr(void* private_data) noexcept { return static_cast<FileSystemNoexcept*>(private_data); }

auto* fs_noexcept_ptr() noexcept { return to_fs_noexcept_ptr(fuse_private_data_ptr()); }

auto& fs_noexcept_ref() noexcept { return *fs_noexcept_ptr(); }

int getattr(char const* path, struct stat* stbuf, struct fuse_file_info* fi) noexcept { return fs_noexcept_ref().getattr(path, stbuf, fi); }

int readlink(char const* path, char* buf, size_t size) noexcept { return fs_noexcept_ref().readlink(path, buf, size); }

int mknod(char const* path, mode_t mode, dev_t rdev) noexcept { return fs_noexcept_ref().mknod(path, mode, rdev); }

int mkdir(char const* path, mode_t mode) noexcept { return fs_noexcept_ref().mkdir(path, mode); }

int unlink(char const* path) noexcept { return fs_noexcept_ref().unlink(path); }

int rmdir(char const* path) noexcept { return fs_noexcept_ref().rmdir(path); }

int symlink(char const* from, char const* to) noexcept { return fs_noexcept_ref().symlink(from, to); }

int rename(char const* from, char const* to, unsigned int flags) noexcept { return fs_noexcept_ref().rename(from, to, flags); }

int link(char const* from, char const* to) noexcept { return fs_noexcept_ref().link(from, to); }

int chmod(char const* path, mode_t mode, struct fuse_file_info* fi) noexcept { return fs_noexcept_ref().chmod(path, mode, fi); }

int chown(char const* path, uid_t uid, gid_t gid, struct fuse_file_info* fi) noexcept { return fs_noexcept_ref().chown(path, uid, gid, fi); }

int truncate(char const* path, off_t size, struct fuse_file_info* fi) noexcept { return fs_noexcept_ref().truncate(path, size, fi); }

int open(char const* path, struct fuse_file_info* fi) noexcept { return fs_noexcept_ref().open(path, fi); }

int read(char const* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi) noexcept
{
    return static_cast<int>(fs_noexcept_ref().read(path, buf, size, offset, fi));
}

int write(char const* path, char const* buf, size_t size, off_t offset, struct fuse_file_info* fi) noexcept
{
    return static_cast<int>(fs_noexcept_ref().write(path, buf, size, offset, fi));
}

int statfs(char const* path, struct statvfs* stbuf) noexcept { return fs_noexcept_ref().statfs(path, stbuf); }

int release(char const* path, struct fuse_file_info* fi) noexcept { return fs_noexcept_ref().release(path, fi); }

int fsync(char const* path, int isdatasync, struct fuse_file_info* fi) noexcept { return fs_noexcept_ref().fsync(path, isdatasync, fi); }

int readdir(char const* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, fuse_readdir_flags flags) noexcept
{
    return fs_noexcept_ref().readdir(path, buf, filler, offset, fi, flags);
}

void* init(struct fuse_conn_info* conn, struct fuse_config* cfg) noexcept
{
    // #ifndef NDEBUG
    //     out << "init: debug " << cfg->debug << std::endl;
    // #endif

    cfg->kernel_cache = 1;

    return fs_noexcept_ptr();
}

void destroy(void* private_data) noexcept
{
    // #ifndef NDEBUG
    //     out << "destroy: private_data " << private_data << std::endl;
    // #endif

    destroy_fs_noexcept(to_fs_noexcept_ptr(private_data));
}

int access(char const* path, int mask) noexcept { return fs_noexcept_ref().access(path, mask); }

int create(char const* path, mode_t mode, struct fuse_file_info* fi) noexcept { return fs_noexcept_ref().create(path, mode, fi); }

#ifdef HAVE_UTIMENSAT
int utimens(char const* path, const struct timespec tv[2], struct fuse_file_info* fi) noexcept { return get_fs().utimens(path, tv, fi); }
#endif // HAVE_UTIMENSAT

#ifdef HAVE_POSIX_FALLOCATE
int fallocate(char const* path, int mode, off_t offset, off_t length, struct fuse_file_info* fi) noexcept
{
    return get_fs().fallocate(path, mode, offset, length, fi);
}
#endif // HAVE_POSIX_FALLOCATE

off_t lseek(char const* path, off_t off, int whence, struct fuse_file_info* fi) noexcept { return fs_noexcept_ref().lseek(path, off, whence, fi); }

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

} // namespace

int main(fuse_args args, app_params const& params)
{
    std::unique_ptr<FileSystemNoexcept, decltype(&destroy_fs_noexcept)> fs{nullptr, destroy_fs_noexcept};

    if (params.show_help || params.mpts.empty()) {
        if (params.mpts.empty())
            std::cerr << "there are no a single FS to combine within multifs\n";
        show_help(args.argv[0]);
        assert(fuse_opt_add_arg(&args, "--help") == 0);
        args.argv[0][0] = '\0';
    } else {
        fs = make_fs_noexcept(make_bfs(params));
    }

    return fuse_main(args.argc, args.argv, &getops(), fs.release());
}

} // namespace multifs
