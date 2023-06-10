#pragma once

#include <fuse.h>

#include "app_params.hpp"

namespace multifs
{
int main(fuse_args args, app_params const& params);
} // namespace multifs
