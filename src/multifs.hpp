#pragma once

#include <fuse.h>

namespace multifs
{
fuse_operations const& getops() noexcept;
} // namespace multifs
