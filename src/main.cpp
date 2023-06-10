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

#include "app_params.hpp"
#include "multifs.hpp"
#include "scope_exit.hpp"

using namespace multifs;

namespace
{

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

} // namespace

int main(int argc, char* argv[])
try {
    using namespace multifs;
    app_params params;
    fuse_args args = FUSE_ARGS_INIT(argc, argv);
    /* Parse options */
    if (fuse_opt_parse(&args, &params, multifs_option_desc, arg_processor) == -1)
        return EINVAL;
    scope_exit const free_args_sce{[&args] { fuse_opt_free_args(&args); }};
    return main(args, params);
} catch (std::exception const& ex) {
    std::cerr << ex.what() << '\n';
    return EXIT_FAILURE;
} catch (...) {
    std::cerr << "unknown exception occurred\n";
    return EXIT_FAILURE;
}
