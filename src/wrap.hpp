#pragma once

#include <cerrno>

#include <stdexcept>
#include <system_error>
#include <type_traits>

namespace multifs
{

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
