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

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/constants.hpp>
#include <boost/algorithm/string/split.hpp>

#include <fuse.h>

#include <unistd.h>

#include "file_system_interface.hpp"
#include "file_system_reflector.hpp"
#include "logged_file_system.hpp"
#include "multi_file_system.hpp"
#include "thread_safe_access_file_system.hpp"

#include "multifs.hpp"
#include "scope_exit.hpp"

using namespace multifs;

namespace
{

struct app_params {
    int show_help;
    std::list<std::filesystem::path> mpts; ///< Mount points
    std::filesystem::path logp;            ///< Log path
};

enum {
    /* Valueless keys */
    KEY_HELP,
    KEY_VALUELESS_QTY,
    /* Valueful keys */
    KEY_FSS = KEY_VALUELESS_QTY,
    KEY_LOG,
    KEY_VALUEFUL_QTY,
};

const struct fuse_opt multifs_option_desc[] = {
    FUSE_OPT_KEY("--help", KEY_HELP),
    FUSE_OPT_KEY("-h", KEY_HELP),
    FUSE_OPT_KEY("--fss=", KEY_FSS),
    FUSE_OPT_KEY("--log=", KEY_LOG),
    FUSE_OPT_END,
};

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

int arg_processor(void* data, char const* arg, int key, struct fuse_args* outargs) noexcept
try {
    if (auto it = std::ranges::find_if(multifs_option_desc, [=](auto const& opt) { return key == opt.value; }); it != std::end(multifs_option_desc)) {
        auto& params = *static_cast<app_params*>(data);
        /* Process valueless keys */
        if (it->value < KEY_VALUELESS_QTY) {
            switch (it->value) {
                case KEY_HELP:
                    params.show_help = 1;
                    break;
                default:
                    break;
            }
        } else {
            std::string_view svarg{arg};
            svarg.remove_prefix(std::string_view{it->templ}.length());
            switch (it->value) {
                case KEY_FSS: {
                    std::list<std::filesystem::path> mpts;
                    boost::split(mpts, svarg, boost::is_any_of(":"), boost::token_compress_on);
                    std::ranges::move(mpts, std::back_inserter(params.mpts));
                    return 0;
                }
                case KEY_LOG:
                    params.logp = svarg;
                    return 0;
                default:
                    break;
            }
        }
    }
    return 1;
} catch (std::exception const& ex) {
    std::cerr << ex.what() << '\n';
    return -1;
} catch (...) {
    std::cerr << "unknown exception occurred\n";
    return -1;
}

auto make_absolute_normal(std::filesystem::path const& path) { return std::filesystem::absolute(path).lexically_normal(); }

template <typename PathsRange>
auto make_absolute_normal(PathsRange const& paths)
{
    std::vector<std::filesystem::path> new_paths;
    new_paths.reserve(paths.size());
    std::ranges::transform(paths, std::back_inserter(new_paths), [](auto const& path) { return make_absolute_normal(path); });
    return new_paths;
}

std::unique_ptr<IFileSystem> make_fs(app_params const& params)
{
    std::unique_ptr<IFileSystem> fs;
    if (auto const& mpts = params.mpts; 1 == mpts.size()) {
        fs = std::make_unique<FileSystemReflector>(make_absolute_normal(mpts.front()));
    } else {
        std::list<std::unique_ptr<IFileSystem>> fss;
        std::ranges::transform(make_absolute_normal(mpts), std::back_inserter(fss), [](auto const& mp) { return std::make_unique<FileSystemReflector>(mp); });
        fs = std::make_unique<ThreadSafeAccessFileSystem>(
            std::make_unique<MultiFileSystem>(getuid(), getgid(), std::make_move_iterator(fss.begin()), std::make_move_iterator(fss.end())));
    }
    return fs;
}

} // namespace

int main(int argc, char* argv[])
{
    fuse_args args = FUSE_ARGS_INIT(argc, argv);

    app_params params;

    /* Parse options */
    if (fuse_opt_parse(&args, &params, multifs_option_desc, arg_processor) == -1)
        return EINVAL;

    multifs::scope_exit const free_args_sce{[&args] { fuse_opt_free_args(&args); }};

    std::unique_ptr<IFileSystem> fs;

    if (params.show_help) {
        show_help(argv[0]);
        assert(fuse_opt_add_arg(&args, "--help") == 0);
        args.argv[0][0] = '\0';
    } else if (!params.mpts.empty()) {
        fs = make_fs(params);
        if (auto const& logp = params.logp; !logp.empty())
            fs = std::make_unique<LoggedFileSystem>(std::move(fs), logp);
    } else {
        std::cerr << "there are no a single FS to combine within multifs\n";
        return EINVAL;
    }

    return fuse_main(args.argc, args.argv, &multifs::getops(), fs.release());
}
