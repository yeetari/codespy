#pragma once

#include <cstddef>
#include <type_traits>

namespace codespy {
namespace detail {

template <typename, typename U>
struct CopyConst {
    using type = U;
};
template <typename T, typename U>
struct CopyConst<const T, U> {
    using type = const U;
};

} // namespace detail

template <typename T, typename U>
using copy_const = typename detail::CopyConst<T, U>::type;

template <typename T>
concept TriviallyCopyable = std::is_trivially_copyable_v<T>;

// NOLINTBEGIN
template <typename T>
struct AlignedStorage {
    alignas(T) unsigned char data[sizeof(T)];

    template <typename... Args>
    void emplace(Args &&...args) {
        new (data) T(forward<Args>(args)...);
    }

    void set(const T &value) { new (data) T(value); }
    void set(T &&value) { new (data) T(move(value)); }
    void release() { get().~T(); }

    T &get() { return *__builtin_launder(reinterpret_cast<T *>(data)); }
    const T &get() const { return const_cast<AlignedStorage<T> *>(this)->get(); }
};
// NOLINTEND

template <typename... Ts>
struct TypeList {
    template <unsigned, typename, typename...>
    struct Index;

    template <unsigned I, typename T>
    struct Index<I, T> {
        static constexpr auto index = unsigned(-1);
    };

    template <unsigned I, typename T, typename U, typename... Rest>
    struct Index<I, T, U, Rest...> {
        static constexpr auto index = std::is_same_v<T, U> ? I : Index<I + 1, T, Rest...>::index;
    };

    template <typename T>
    static consteval bool contains() {
        return (std::is_same_v<T, Ts> || ...);
    }

    template <typename T>
    static consteval auto index_of() {
        return Index<0, T, Ts...>::index;
    }
};

// clang-format off
template <typename T, typename... Ts>
concept ContainsType = TypeList<Ts...>::template contains<T>();
// clang-format on

template <typename T>
class RefWrapper {
    T *m_ptr;

public:
    constexpr RefWrapper(T &ref) : m_ptr(&ref) {}
    constexpr operator T &() const { return *m_ptr; }
};

template <typename T>
constexpr auto ref(T &ref) {
    return RefWrapper<T>(ref);
}
template <typename T>
constexpr auto cref(const T &ref) {
    return RefWrapper<const T>(ref);
}

constexpr auto maybe_unwrap(auto t) {
    return t;
}
template <typename T>
constexpr T &maybe_unwrap(T &t) {
    return t;
}
template <typename T>
constexpr T &maybe_unwrap(RefWrapper<T> t) {
    return static_cast<T &>(t);
}

template <typename T>
struct UnrapRefWrapper {
    using type = T;
};
template <typename T>
struct UnrapRefWrapper<RefWrapper<T>> {
    using type = T &;
};

template <typename T>
using unwrap_ref = typename UnrapRefWrapper<T>::type;

template <typename T>
using decay_unwrap = unwrap_ref<std::decay_t<T>>;

inline constexpr auto &operator&=(auto &lhs, auto rhs) {
    return lhs = (lhs & rhs);
}
inline constexpr auto &operator|=(auto &lhs, auto rhs) {
    return lhs = (lhs | rhs);
}
inline constexpr auto &operator^=(auto &lhs, auto rhs) {
    return lhs = (lhs ^ rhs);
}

inline std::size_t hash_combine(std::size_t lhs, std::size_t rhs) {
    lhs ^= rhs + 0x9e3779b9 + (lhs << 6u) + (lhs >> 2u);
    return lhs;
}

[[noreturn]] inline void unreachable() {
#ifdef __GNUC__
    __builtin_unreachable();
#else
    __assume(false);
#endif
}

} // namespace codespy
