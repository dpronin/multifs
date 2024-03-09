#pragma once

#include <filesystem>
#include <list>

namespace multifs
{

struct app_params {
    bool show_help;
    std::list<std::filesystem::path> mpts; ///< Mount points
#ifndef NDEBUG
    std::filesystem::path logp; ///< Log path
#endif
};

} // namespace multifs
