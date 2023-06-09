#pragma once

#include <filesystem>
#include <memory>
#include <utility>

#include "factory_unique_interface.hpp"
#include "file_system_interface.hpp"
#include "file_system_reflector.hpp"

namespace multifs
{

class FSReflectorFactory final : public IFactoryUnique<rvwrap<IFileSystem>()>
{
private:
    std::filesystem::path mp_;

public:
    explicit FSReflectorFactory(std::filesystem::path mount_point)
        : mp_(std::move(mount_point))
    {
    }

    FSReflectorFactory(FSReflectorFactory const&)            = default;
    FSReflectorFactory& operator=(FSReflectorFactory const&) = default;

    FSReflectorFactory(FSReflectorFactory&&) noexcept            = default;
    FSReflectorFactory& operator=(FSReflectorFactory&&) noexcept = default;

    std::unique_ptr<IFileSystem> create_unique() override { return std::make_unique<FileSystemReflector>(mp_); }
};

} // namespace multifs
