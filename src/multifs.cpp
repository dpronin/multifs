#include <cassert>
#include <cstdlib>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <list>
#include <memory>
#include <ranges>
#include <string_view>
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
#include "scope_exit.hpp"

using namespace multifs;

namespace
{

struct options {
    int show_help;
} opts;

std::list<std::filesystem::path> __mpts__;
std::filesystem::path __logp__;

enum {
    KEY_FS,
    KEY_LOG,
};

struct multifs_option_desc {
    int index;
    std::string_view key;
} multifs_option_desc[] = {
    {.index = KEY_FS, .key = "--fs="},
    {.index = KEY_LOG, .key = "--log="},
};

#define OPTION(t, p)                                                                                                                                           \
    {                                                                                                                                                          \
        t, offsetof(struct options, p), 1                                                                                                                      \
    }

const struct fuse_opt option_spec[] = {
    OPTION("-h", show_help),
    OPTION("--help", show_help),
    FUSE_OPT_KEY(multifs_option_desc[KEY_FS].key.data(), multifs_option_desc[KEY_FS].index),
    FUSE_OPT_KEY(multifs_option_desc[KEY_LOG].key.data(), multifs_option_desc[KEY_LOG].index),
    FUSE_OPT_END,
};

#undef OPTION

std::unique_ptr<IFSFactory> make_fs_factory()
{
    std::unique_ptr<IFSFactory> fsf;

    if (1 == __mpts__.size())
        fsf = std::make_unique<FSReflectorFactory>(std::move(__mpts__.front()));
    else
        fsf = std::make_unique<MultiFSFactory>(getuid(), getgid(), std::make_move_iterator(__mpts__.begin()), std::make_move_iterator(__mpts__.end()));

    return fsf;
}

auto& multifs_get_fs() noexcept { return *static_cast<IFileSystem*>(fuse_get_context()->private_data); }

int multifs_getattr(char const* path, struct stat* stbuf, struct fuse_file_info* fi) noexcept { return multifs_get_fs().getattr(path, stbuf, fi); }

int multifs_readlink(char const* path, char* buf, size_t size) noexcept { return multifs_get_fs().readlink(path, buf, size); }

int multifs_mknod(char const* path, mode_t mode, dev_t rdev) noexcept { return multifs_get_fs().mknod(path, mode, rdev); }

int multifs_mkdir(char const* path, mode_t mode) noexcept { return multifs_get_fs().mkdir(path, mode); }

int multifs_unlink(char const* path) noexcept { return multifs_get_fs().unlink(path); }

int multifs_rmdir(char const* path) noexcept { return multifs_get_fs().rmdir(path); }

int multifs_symlink(char const* from, char const* to) noexcept { return multifs_get_fs().symlink(from, to); }

int multifs_rename(char const* from, char const* to, unsigned int flags) noexcept { return multifs_get_fs().rename(from, to, flags); }

int multifs_link(char const* from, char const* to) noexcept { return multifs_get_fs().link(from, to); }

int multifs_chmod(char const* path, mode_t mode, struct fuse_file_info* fi) noexcept { return multifs_get_fs().chmod(path, mode, fi); }

int multifs_chown(char const* path, uid_t uid, gid_t gid, struct fuse_file_info* fi) noexcept { return multifs_get_fs().chown(path, uid, gid, fi); }

int multifs_truncate(char const* path, off_t size, struct fuse_file_info* fi) noexcept { return multifs_get_fs().truncate(path, size, fi); }

int multifs_open(char const* path, struct fuse_file_info* fi) noexcept { return multifs_get_fs().open(path, fi); }

int multifs_read(char const* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi) noexcept
{
    return static_cast<int>(multifs_get_fs().read(path, buf, size, offset, fi));
}

int multifs_write(char const* path, char const* buf, size_t size, off_t offset, struct fuse_file_info* fi) noexcept
{
    return static_cast<int>(multifs_get_fs().write(path, buf, size, offset, fi));
}

int multifs_statfs(char const* path, struct statvfs* stbuf) noexcept { return multifs_get_fs().statfs(path, stbuf); }

int multifs_release(char const* path, struct fuse_file_info* fi) noexcept { return multifs_get_fs().release(path, fi); }

int multifs_fsync(char const* path, int isdatasync, struct fuse_file_info* fi) noexcept { return multifs_get_fs().fsync(path, isdatasync, fi); }

int multifs_readdir(char const* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, fuse_readdir_flags flags) noexcept
{
    return multifs_get_fs().readdir(path, buf, filler, offset, fi, flags);
}

void* multifs_init(struct fuse_conn_info* conn, struct fuse_config* cfg) noexcept
{
    // #ifndef NDEBUG
    //     out << "init: debug " << cfg->debug << std::endl;
    // #endif

    cfg->kernel_cache = 1;

    auto fs = make_fs_factory()->create_unique();
    if (!__logp__.empty())
        fs = std::make_unique<LoggedFileSystem>(std::move(fs), std::move(__logp__));

    return new FileSystemNoexcept(std::move(fs));
}

void multifs_destroy(void* private_data)
{
    // #ifndef NDEBUG
    //     out << "destroy: private_data " << private_data << std::endl;
    // #endif

    delete static_cast<FileSystemNoexcept*>(private_data);
}

int multifs_access(char const* path, int mask) noexcept { return multifs_get_fs().access(path, mask); }

int multifs_create(char const* path, mode_t mode, struct fuse_file_info* fi) noexcept { return multifs_get_fs().create(path, mode, fi); }

#ifdef HAVE_UTIMENSAT
int multifs_utimens(char const* path, const struct timespec tv[2], struct fuse_file_info* fi) noexcept { return multifs_get_fs().utimens(path, tv, fi); }
#endif // HAVE_UTIMENSAT

#ifdef HAVE_POSIX_FALLOCATE
int multifs_fallocate(char const* path, int mode, off_t offset, off_t length, struct fuse_file_info* fi) noexcept
{
    return multifs_get_fs().fallocate(path, mode, offset, length, fi);
}
#endif // HAVE_POSIX_FALLOCATE

off_t multifs_lseek(char const* path, off_t off, int whence, struct fuse_file_info* fi) noexcept { return multifs_get_fs().lseek(path, off, whence, fi); }

void show_help(std::string_view progname)
{
    std::cout << "usage: " << progname << " [options] <mountpoint>\n\n";
    std::cout << "Multi File-system specific options:\n"
              << "    --fs=<path>            path to a single mount point to "
                 "combine it with others within the Multi File-system\n"
              << "    --log=<path>           path to a file where multifs will log operations\n"
              << "\n";
    std::cout.flush();
}

int arg_processor(void* data, char const* arg, int key, struct fuse_args* outargs) noexcept
try {
    if (auto it = std::ranges::find_if(multifs_option_desc, [=](auto const& opt) { return key == opt.index; }); it != std::end(multifs_option_desc)) {
        std::string_view svarg{arg};
        svarg.remove_prefix(it->key.size());
        switch (it->index) {
            case KEY_FS:
                __mpts__.push_back(std::filesystem::absolute(svarg).lexically_normal());
                return 0;
            case KEY_LOG:
                __logp__ = std::filesystem::absolute(svarg).lexically_normal();
                return 0;
            default:
                return 1;
        }
    } else {
        return 1;
    }
} catch (std::exception const& ex) {
    std::cerr << ex.what() << '\n';
    return -1;
} catch (...) {
    std::cerr << "unknown exception occurred\n";
    return -1;
}

fuse_operations const multifs_oper = {
    .getattr  = multifs_getattr,
    .readlink = multifs_readlink,
    .mknod    = multifs_mknod,
    .mkdir    = multifs_mkdir,
    .unlink   = multifs_unlink,
    .rmdir    = multifs_rmdir,
    .symlink  = multifs_symlink,
    .rename   = multifs_rename,
    .link     = multifs_link,
    .chmod    = multifs_chmod,
    .chown    = multifs_chown,
    .truncate = multifs_truncate,
    .open     = multifs_open,
    .read     = multifs_read,
    .write    = multifs_write,
    .statfs   = multifs_statfs,
    .release  = multifs_release,
    .fsync    = multifs_fsync,
    .readdir  = multifs_readdir,
    .init     = multifs_init,
    .destroy  = multifs_destroy,
    .access   = multifs_access,
    .create   = multifs_create,
#ifdef HAVE_UTIMENSAT
    .utimens = multifs_utimens,
#endif
#ifdef HAVE_POSIX_FALLOCATE
    .fallocate = multifs_fallocate,
#endif
    // #ifdef HAVE_SETXATTR
    //     .setxattr = multifs_setxattr,
    //     .getxattr = multifs_getxattr,
    //     .listxattr = multifs_listxattr,
    //     .removexattr = multifs_removexattr,
    // #endif
    // #ifdef HAVE_COPY_FILE_RANGE
    //     .copy_file_range = multifs_copy_file_range,
    // #endif
    .lseek = multifs_lseek,
};

} // namespace

int main(int argc, char* argv[])
{
    fuse_args args = FUSE_ARGS_INIT(argc, argv);

    /* Parse options */
    if (fuse_opt_parse(&args, &opts, option_spec, arg_processor) == -1)
        return EINVAL;

    scope_exit const free_args_sce{[&args] { fuse_opt_free_args(&args); }};

    if (opts.show_help) {
        show_help(argv[0]);
        assert(fuse_opt_add_arg(&args, "--help") == 0);
        args.argv[0][0] = '\0';
    } else if (__mpts__.empty()) {
        std::cerr << "there are no a single FS to combine to multifs\n";
        return EINVAL;
    }

    return fuse_main(args.argc, args.argv, &multifs_oper, NULL);
}
