#pragma once

#include <cerrno>

#include <stdexcept>
#include <system_error>
#include <type_traits>

namespace multifs
{

template <typename Callable, typename... Args>
auto wrap(Callable&& callable, Args&&... args)
{
    using return_t = std::invoke_result_t<Callable, Args...>;
    try {
        if constexpr (std::is_same_v<return_t, void>)
            return callable(std::forward<Args>(args)...), static_cast<return_t>(0);
        else
            return static_cast<return_t>(callable(std::forward<Args>(args)...));
    } catch (std::system_error const& ex) {
        return static_cast<return_t>(-ex.code().value());
    } catch (std::invalid_argument const& ex) {
        return static_cast<return_t>(-EINVAL);
    } catch (std::bad_alloc const& ex) {
        return static_cast<return_t>(-ENOMEM);
    } catch (...) {
        return static_cast<return_t>(-EINVAL);
    }
}

} // namespace multifs
