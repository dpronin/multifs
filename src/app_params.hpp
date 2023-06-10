#pragma once

#include <filesystem>
#include <list>

namespace multifs
{

struct app_params {
    int show_help;
    std::list<std::filesystem::path> mpts; ///< Mount points
    std::filesystem::path logp;            ///< Log path
};

} // namespace multifs
