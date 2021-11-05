#pragma once

#include <cerrno>
#include <ctime>

#include <new>
#include <stdexcept>
#include <system_error>
#include <type_traits>

namespace multifs
{

template <typename T>
struct dependent_true : std::true_type {
};
template <typename T>
inline constexpr bool dependent_true_v = dependent_true<T>::value;
template <typename T>
struct dependent_false : std::false_type {
};
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

template <typename Callable, typename... Args>
auto wrap(Callable&& callable, Args&&... args)
try {
    using return_t = std::invoke_result_t<Callable, Args...>;
    if constexpr (std::is_same_v<return_t, void>)
        return callable(std::forward<Args>(args)...), 0;
    else
        return callable(std::forward<Args>(args)...);
} catch (std::system_error const& ex) {
    return -ex.code().value();
} catch (std::invalid_argument const& ex) {
    return -EINVAL;
} catch (std::bad_alloc const& ex) {
    return -ENOMEM;
} catch (...) {
    return -EINVAL;
}

} // namespace multifs
