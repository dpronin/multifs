#pragma once

#include <filesystem>
#include <list>
#include <utility>

#include <fuse.h>

namespace multifs
{

namespace detail
{

inline std::list<std::filesystem::path> __mpts__;
inline std::filesystem::path __logp__;

} // namespace detail

class multifs
{
public:
    static auto& instance() noexcept
    {
        static multifs obj;
        return obj;
    }

    void append_mpt(std::filesystem::path path) { detail::__mpts__.push_back(std::filesystem::absolute(path).lexically_normal()); }
    auto const& mpts() const noexcept { return detail::__mpts__; }

    void set_logp(std::filesystem::path path) { detail::__logp__ = std::filesystem::absolute(path).lexically_normal(); }
    auto const& logp() const noexcept { return detail::__logp__; }

private:
    multifs() noexcept  = default;
    ~multifs() noexcept = default;

    multifs(multifs const&)            = delete;
    multifs& operator=(multifs const&) = delete;

    multifs(multifs&&)            = delete;
    multifs& operator=(multifs&&) = delete;
};

fuse_operations const& getops() noexcept;

} // namespace multifs
