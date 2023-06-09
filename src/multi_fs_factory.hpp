#pragma once

#include <sys/types.h>

#include <algorithm>
#include <iterator>
#include <list>
#include <memory>
#include <ranges>

#include "file_system_interface.hpp"
#include "file_system_reflector.hpp"
#include "fs_factory_interface.hpp"
#include "multi_file_system.hpp"
#include "thread_safe_access_file_system.hpp"

namespace multifs
{

class MultiFSFactory final : public IFSFactory
{
private:
    uid_t owner_uid_;
    gid_t owner_gid_;
    std::list<std::filesystem::path> fsmps_;

public:
    template <typename InputIt>
    explicit MultiFSFactory(uid_t owner_uid, gid_t owner_gid, InputIt begin, InputIt end)
        : owner_uid_(owner_uid)
        , owner_gid_(owner_gid)
        , fsmps_(begin, end)
    {
    }

    MultiFSFactory(MultiFSFactory const&)            = default;
    MultiFSFactory& operator=(MultiFSFactory const&) = default;

    MultiFSFactory(MultiFSFactory&&) noexcept            = default;
    MultiFSFactory& operator=(MultiFSFactory&&) noexcept = default;

    std::unique_ptr<IFileSystem> create_unique() override
    {
        std::list<std::unique_ptr<IFileSystem>> fss;
        std::ranges::transform(fsmps_, std::back_inserter(fss), [](auto const& mp) { return std::make_unique<FileSystemReflector>(mp); });
        return std::make_unique<ThreadSafeAccessFileSystem>(
            std::make_unique<MultiFileSystem>(owner_uid_, owner_gid_, std::make_move_iterator(fss.begin()), std::make_move_iterator(fss.end())));
    }
};

} // namespace multifs
