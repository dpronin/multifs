#pragma once

#include <ctime>

#include <type_traits>

namespace multifs
{

// clang-format off
template <typename T>
struct dependent_true : std::true_type {};
// clang-format on

template <typename T>
inline constexpr bool dependent_true_v = dependent_true<T>::value;

// clang-format off
template <typename T>
struct dependent_false : std::false_type {};
// clang-format on

template <typename T>
inline constexpr bool dependent_false_v = dependent_false<T>::value;

inline auto current_time() noexcept
{
    // clang-format off
    struct timespec current_time{};
    // clang-format on
    timespec_get(&current_time, TIME_UTC);
    return current_time;
}

} // namespace multifs
