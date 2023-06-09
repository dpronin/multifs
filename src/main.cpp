#include <cassert>
#include <cstdlib>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <memory>
#include <ranges>
#include <string_view>
#include <utility>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/constants.hpp>
#include <boost/algorithm/string/split.hpp>

#include <fuse.h>

#include "multifs.hpp"
#include "scope_exit.hpp"

namespace
{

struct options {
    int show_help;
} opts;

enum {
    KEY_FSS,
    KEY_LOG,
};

struct multifs_option_desc {
    int index;
    std::string_view key;
} multifs_option_desc[] = {
    {.index = KEY_FSS, .key = "--fss="},
    {.index = KEY_LOG, .key = "--log="},
};

#define OPTION(t, p)                                                                                                                                           \
    {                                                                                                                                                          \
        t, offsetof(struct options, p), 1                                                                                                                      \
    }

const struct fuse_opt option_spec[] = {
    OPTION("-h", show_help),
    OPTION("--help", show_help),
    FUSE_OPT_KEY(multifs_option_desc[KEY_FSS].key.data(), multifs_option_desc[KEY_FSS].index),
    FUSE_OPT_KEY(multifs_option_desc[KEY_LOG].key.data(), multifs_option_desc[KEY_LOG].index),
    FUSE_OPT_END,
};

#undef OPTION

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
    if (auto it = std::ranges::find_if(multifs_option_desc, [=](auto const& opt) { return key == opt.index; }); it != std::end(multifs_option_desc)) {
        std::string_view svarg{arg};
        svarg.remove_prefix(it->key.size());
        switch (it->index) {
            case KEY_FSS: {
                auto& mpts = multifs::multifs::instance().mpts();
                std::remove_reference_t<decltype(mpts)> tmp_mpts;
                boost::split(tmp_mpts, svarg, boost::is_any_of(":"), boost::token_compress_on);
                std::ranges::transform(tmp_mpts, std::back_inserter(mpts), [](auto const& mp) { return std::filesystem::absolute(mp).lexically_normal(); });
                return 0;
            }
            case KEY_LOG:
                multifs::multifs::instance().logp() = std::filesystem::absolute(svarg).lexically_normal();
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

} // namespace

int main(int argc, char* argv[])
{
    fuse_args args = FUSE_ARGS_INIT(argc, argv);

    /* Parse options */
    if (fuse_opt_parse(&args, &opts, option_spec, arg_processor) == -1)
        return EINVAL;

    multifs::scope_exit const free_args_sce{[&args] { fuse_opt_free_args(&args); }};

    if (opts.show_help) {
        show_help(argv[0]);
        assert(fuse_opt_add_arg(&args, "--help") == 0);
        args.argv[0][0] = '\0';
    } else if (multifs::multifs::instance().mpts().empty()) {
        std::cerr << "there are no a single FS to combine to multifs\n";
        return EINVAL;
    }

    return fuse_main(args.argc, args.argv, &multifs::getops(), NULL);
}
