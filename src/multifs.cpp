#include <algorithm>
#include <cassert>
#include <cstdlib>

#include <filesystem>
#include <iostream>
#include <iterator>
#include <list>
#include <memory>
#include <ranges>
#include <string_view>
#include <utility>

#include <unistd.h>

#include "file_system_interface.hpp"
#include "file_system_reflector.hpp"
#include "fs_factory_interface.hpp"
#include "fs_reflector_factory.hpp"
#include "log.hpp"
#include "multi_file_system.hpp"
#include "multi_fs_factory.hpp"

extern "C" {
#include "fuse.h"
} // extern "C"

using namespace multifs;

namespace
{

std::unique_ptr<IFSFactory> __FSF__;
std::unique_ptr<IFileSystem> __FS__;

struct options {
    int show_help;
} opts;

std::list<std::filesystem::path> __mpts__;

enum { KEY_FS };
constexpr std::string_view kKeyFSPrefix = "--fs=";

#define OPTION(t, p)                                                                                                                                           \
    {                                                                                                                                                          \
        t, offsetof(struct options, p), 1                                                                                                                      \
    }
static const struct fuse_opt option_spec[] = {
    OPTION("-h", show_help),
    OPTION("--help", show_help),
    FUSE_OPT_KEY(kKeyFSPrefix.data(), KEY_FS),
    FUSE_OPT_END,
};

int multifs_getattr(char const* path, struct stat* stbuf, struct fuse_file_info* fi) noexcept
{
#ifndef NDEBUG
    out << "getattr, path " << path << std::endl;
#endif
    return __FS__->getattr(path, stbuf, fi);
}

int multifs_readlink(char const* path, char* buf, size_t size) noexcept
{
#ifndef NDEBUG
    out << "readlink, path " << path << ", buf " << buf << ", size " << size << std::endl;
#endif
    return __FS__->readlink(path, buf, size);
}

int multifs_mknod(char const* path, mode_t mode, dev_t rdev) noexcept
{
#ifndef NDEBUG
    out << "mknod, path " << path << ", mode 0" << std::oct << mode << std::dec << std::endl;
#endif
    return wrap([](auto... args) { return __FS__->mknod(args...); }, path, mode, rdev);
}

int multifs_mkdir(char const* path, mode_t mode) noexcept
{
#ifndef NDEBUG
    out << "mkdir, path " << path << ", mode 0" << std::oct << mode << std::dec << std::endl;
#endif
    return wrap([](auto... args) { return __FS__->mkdir(args...); }, path, mode);
}

int multifs_unlink(char const* path) noexcept
{
#ifndef NDEBUG
    out << "unlink, path " << path << std::endl;
#endif
    return wrap([](auto... args) { return __FS__->unlink(args...); }, path);
}

int multifs_rmdir(char const* path) noexcept
{
#ifndef NDEBUG
    out << "rmdir, path " << path << std::endl;
#endif
    return wrap([](auto... args) { return __FS__->rmdir(args...); }, path);
}

int multifs_symlink(char const* from, char const* to) noexcept
{
#ifndef NDEBUG
    out << "symlink, from " << from << ", to " << to << std::endl;
#endif
    return wrap([](auto... args) { return __FS__->symlink(args...); }, from, to);
}

int multifs_rename(char const* from, char const* to, unsigned int flags) noexcept
{
#ifndef NDEBUG
    out << "rename, from " << from << ", to " << to << ", flags 0x" << std::hex << flags << std::dec << std::endl;
#endif
    return wrap([](auto... args) { return __FS__->rename(args...); }, from, to, flags);
}

int multifs_link(char const* from, char const* to) noexcept
{
#ifndef NDEBUG
    out << "link, from " << from << ", to " << to << std::endl;
#endif
    return wrap([](auto... args) { return __FS__->link(args...); }, from, to);
}

int multifs_chmod(char const* path, mode_t mode, struct fuse_file_info* fi) noexcept
{
#ifndef NDEBUG
    out << "chmod, path " << path << ", mode 0" << std::oct << mode << std::dec << ", fi " << fi;
    if (fi)
        out << ", fi->flags 0" << std::oct << fi->flags << std::dec;
    out << std::endl;
#endif
    return __FS__->chmod(path, mode, fi);
}

int multifs_chown(char const* path, uid_t uid, gid_t gid, struct fuse_file_info* fi) noexcept
{
#ifndef NDEBUG
    out << "chown, path " << path << ", uid " << uid << ", gid " << gid << ", fi " << fi;
    if (fi)
        out << ", fi->flags 0" << std::oct << fi->flags << std::dec;
    out << std::endl;
#endif
    return __FS__->chown(path, uid, gid, fi);
}

int multifs_truncate(char const* path, off_t size, struct fuse_file_info* fi) noexcept
{
#ifndef NDEBUG
    out << "truncate, path " << path << ", size " << size << ", fi " << fi;
    if (fi)
        out << ", fi->flags 0" << std::oct << fi->flags << std::dec;
    out << std::endl;
#endif
    return wrap([](auto... args) { return __FS__->truncate(args...); }, path, size, fi);
}

int multifs_open(char const* path, struct fuse_file_info* fi) noexcept
{
#ifndef NDEBUG
    out << "open, path " << path << ", fi " << fi;
    if (fi)
        out << ", fi->flags 0" << std::oct << fi->flags << std::dec;
    out << std::endl;
#endif
    return wrap([](auto... args) { return __FS__->open(args...); }, path, fi);
}

int multifs_read(char const* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi) noexcept
{
#ifndef NDEBUG
    out << "read, path " << path << ", buf " << static_cast<void const*>(buf) << ", size " << size << ", off " << offset << ", fi " << fi;
    if (fi)
        out << ", fi->flags 0" << std::oct << fi->flags << std::dec;
    out << std::endl;
#endif
    return static_cast<int>(__FS__->read(path, buf, size, offset, fi));
}

int multifs_write(char const* path, char const* buf, size_t size, off_t offset, struct fuse_file_info* fi) noexcept
{
#ifndef NDEBUG
    out << "write, path " << path << ", buf " << static_cast<void const*>(buf) << ", size " << size << ", off " << offset << ", fi " << fi;
    if (fi)
        out << ", fi->flags 0" << std::oct << fi->flags << std::dec;
    out << std::endl;
#endif
    return wrap([](auto... args) { return static_cast<int>(__FS__->write(args...)); }, path, buf, size, offset, fi);
}

int multifs_statfs(char const* path, struct statvfs* stbuf) noexcept
{
#ifndef NDEBUG
    out << "statfs, path " << path << ", stbuf " << stbuf << std::endl;
#endif
    return __FS__->statfs(path, stbuf);
}

int multifs_release(char const* path, struct fuse_file_info* fi) noexcept
{
#ifndef NDEBUG
    out << "release, path " << path << ", fi " << fi;
    if (fi)
        out << ", fi->flags 0" << std::oct << fi->flags << std::dec;
    out << std::endl;
#endif
    return __FS__->release(path, fi);
}

int multifs_fsync(char const* path, int isdatasync, struct fuse_file_info* fi) noexcept
{
#ifndef NDEBUG
    out << "fsync, path " << path << ", fi " << fi;
    if (fi)
        out << ", fi->flags 0" << std::oct << fi->flags << std::dec;
    out << std::endl;
#endif
    return __FS__->fsync(path, isdatasync, fi);
}

int multifs_readdir(char const* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, fuse_readdir_flags flags) noexcept
{
#ifndef NDEBUG
    out << "readdir, path " << path << ", buf " << buf << ", off " << offset << ", fi " << fi;
    if (fi)
        out << ", fi->flags 0" << std::oct << fi->flags << std::dec;
    out << ", flags 0x" << std::hex << static_cast<int>(flags) << std::dec << std::endl;
#endif
    return wrap([](auto... args) { return __FS__->readdir(args...); }, path, buf, filler, offset, fi, flags);
}

void* multifs_init(struct fuse_conn_info* conn, struct fuse_config* cfg) noexcept
{
#ifndef NDEBUG
    out << "init " << std::endl;
#endif
    __FS__ = __FSF__->create_unique();
    return __FS__->init(conn, cfg);
}

int multifs_access(char const* path, int mask) noexcept
{
#ifndef NDEBUG
    out << "access, path " << path << ", mask 0" << std::oct << mask << std::dec << std::endl;
#endif
    return __FS__->access(path, mask);
}

int multifs_create(char const* path, mode_t mode, struct fuse_file_info* fi) noexcept
{
#ifndef NDEBUG
    out << "create, path " << path << ", mode 0" << std::oct << mode << std::dec << ", fi " << fi;
    if (fi)
        out << ", fi->flags 0" << std::oct << fi->flags << std::dec;
    out << std::endl;
#endif
    return wrap([](auto... args) { return __FS__->create(args...); }, path, mode, fi);
}

#ifdef HAVE_UTIMENSAT
int multifs_utimens(char const* path, const struct timespec tv[2], struct fuse_file_info* fi) noexcept
{
#ifndef NDEBUG
    out << "utimens, path " << path;
    if (fi)
        out << ", fi->flags 0" << std::oct << fi->flags << std::dec;
    out << std::endl;
#endif
    return __FS__->utimens(path, tv, fi);
}
#endif // HAVE_UTIMENSAT

#ifdef HAVE_POSIX_FALLOCATE
int multifs_fallocate(char const* path, int mode, off_t offset, off_t length, struct fuse_file_info* fi) noexcept
{
#ifndef NDEBUG
    out << "fallocate, path " << path << ", mode 0" << std::oct << mode << std::dec << ", fi " << fi;
    if (fi)
        out << ", fi->flags 0" << std::oct << fi->flags << std::dec;
    out << std::endl;
#endif
    return wrap([](auto... args) { return __FS__->fallocate(args...); }, path, mode, offset, length, fi);
}
#endif // HAVE_POSIX_FALLOCATE

off_t multifs_lseek(char const* path, off_t off, int whence, struct fuse_file_info* fi) noexcept
{
#ifndef NDEBUG
    out << "lseek, path " << path << std::endl;
#endif
    return __FS__->lseek(path, off, whence, fi);
}

void show_help(std::string_view progname)
{
    std::cout << "usage: " << progname << " [options] <mountpoint>\n\n";
    std::cout << "Multi File-system specific options:\n"
              << "    --fs=<path>            path to a single mount point to "
                 "combine it with others within the Multi File-system"
              << "\n\n";
    std::cout.flush();
}

int arg_processor(void* data, char const* arg, int key, struct fuse_args* outargs) noexcept
try {
    switch (key) {
        case KEY_FS: {
            auto svarg = std::string_view{arg};
            svarg.remove_prefix(kKeyFSPrefix.size());
            __mpts__.push_back(svarg);
            return 0;
        }
        default:
            return 1;
    }
} catch (std::exception const& ex) {
    std::cerr << ex.what() << '\n';
    exit(EXIT_FAILURE);
} catch (...) {
    std::cerr << "unknown exception occurred\n";
    exit(EXIT_FAILURE);
}

inline std::unique_ptr<IFSFactory> make_fs_factory()
{
    std::unique_ptr<IFSFactory> fsf;
    std::vector<std::filesystem::path> mps;

    std::ranges::transform(__mpts__, std::back_inserter(mps), [](auto const& mp) { return std::filesystem::absolute(mp).lexically_normal(); });

    if (__mpts__.empty())
        throw std::runtime_error("there are no single File-systems to combine to Multi File-system");

    if (1 == __mpts__.size())
        fsf = std::make_unique<FSReflectorFactory>(std::move(__mpts__.front()));
    else
        fsf = std::make_unique<MultiFSFactory>(getuid(), getgid(), std::make_move_iterator(__mpts__.begin()), std::make_move_iterator(__mpts__.end()));

    return fsf;
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
        return 1;

    if (opts.show_help) {
        show_help(argv[0]);
        assert(fuse_opt_add_arg(&args, "--help") == 0);
        args.argv[0][0] = '\0';
    } else {
        __FSF__ = make_fs_factory();
    }

    auto const ret = fuse_main(args.argc, args.argv, &multifs_oper, NULL);
    fuse_opt_free_args(&args);
    return ret;
}
