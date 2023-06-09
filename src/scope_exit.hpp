#pragma once

#include <concepts>
#include <utility>

namespace multifs
{

template <typename F>
    requires std::invocable<F>
class scope_exit
{
public:
    using func_t = F;

    scope_exit(F&& f)
        : f_{std::move(f)}
    {
    }
    ~scope_exit() { f_(); }

    scope_exit(scope_exit const&)            = default;
    scope_exit& operator=(scope_exit const&) = default;

    scope_exit(scope_exit&&)            = default;
    scope_exit& operator=(scope_exit&&) = default;

private:
    F f_;
};

} // namespace multifs
