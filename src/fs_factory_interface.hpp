#pragma once

#include "factory_unique_interface.hpp"
#include "file_system_interface.hpp"

namespace multifs
{
using IFSFactory = IFactoryUnique<rvwrap<IFileSystem>()>;
} // namespace multifs
