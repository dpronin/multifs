#pragma once

#include <sys/types.h>

#include <list>
#include <memory>

#include "fs_factory_interface.hpp"
#include "multi_file_system.hpp"

namespace multifs
{

class MultiFSFactory final : public IFSFactory
{
private:
    uid_t owner_uid_;
    gid_t owner_gid_;
    std::list<std::shared_ptr<IFileSystem>> fss_;

public:
    template <typename InputIt>
    explicit MultiFSFactory(uid_t owner_uid, gid_t owner_gid, InputIt begin, InputIt end)
        : owner_uid_(owner_uid)
        , owner_gid_(owner_gid)
        , fss_(begin, end)
    {
    }

    MultiFSFactory(MultiFSFactory const&)            = default;
    MultiFSFactory& operator=(MultiFSFactory const&) = default;

    MultiFSFactory(MultiFSFactory&&) noexcept            = default;
    MultiFSFactory& operator=(MultiFSFactory&&) noexcept = default;

    std::unique_ptr<IFileSystem> create_unique() override { return std::make_unique<MultiFileSystem>(owner_uid_, owner_gid_, fss_.begin(), fss_.end()); }
};

} // namespace multifs
