#pragma once

#include <codespy/support/Utility.hh>

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace codespy {

template <typename T>
class Span {
    T *m_data{nullptr};
    std::size_t m_size{0};

    template <typename U>
    using const_t = codespy::copy_const<T, U>;

    static constexpr bool is_void = std::is_same_v<std::remove_cv_t<T>, void>;
    using no_void_t = std::conditional_t<is_void, char, T>;

public:
    constexpr Span() = default;
    constexpr Span(T *data, std::size_t size) : m_data(data), m_size(size) {}
    constexpr Span(no_void_t &object) requires(!is_void) : m_data(&object), m_size(1) {}

    template <typename U>
    constexpr Span<U> as() const;
    constexpr const_t<std::uint8_t> *byte_offset(std::size_t offset) const;
    constexpr Span<T> subspan(std::size_t offset) const;
    constexpr Span<T> subspan(std::size_t offset, std::size_t size) const;

    // Allow implicit conversion from `Span<T>` to `Span<void>`.
    constexpr operator Span<void>() const requires(!std::is_const_v<T>) { return {data(), size_bytes()}; }
    constexpr operator Span<const void>() const requires(!is_void) { return {data(), size_bytes()}; }
    constexpr operator Span<const T>() const { return {data(), size_bytes()}; }

    constexpr T *begin() const { return m_data; }
    constexpr T *end() const { return m_data + m_size; }

    template <typename U = no_void_t>
    constexpr U &operator[](std::size_t index) const requires(!is_void);
    constexpr bool empty() const { return m_size == 0; }
    constexpr T *data() const { return m_data; }
    constexpr std::size_t size() const { return m_size; }
    constexpr std::size_t size_bytes() const { return m_size * sizeof(T); }
};

template <typename T>
template <typename U>
constexpr Span<U> Span<T>::as() const {
    return {reinterpret_cast<U *>(m_data), m_size};
}

template <typename T>
constexpr typename Span<T>::template const_t<std::uint8_t> *Span<T>::byte_offset(std::size_t offset) const {
    return as<const_t<std::uint8_t>>().data() + offset;
}

template <typename T>
constexpr Span<T> Span<T>::subspan(std::size_t offset) const {
    return {m_data + offset, m_size - offset};
}

template <typename T>
constexpr Span<T> Span<T>::subspan(std::size_t offset, std::size_t size) const {
    return {m_data + offset, size};
}

template <typename T>
template <typename U>
constexpr U &Span<T>::operator[](std::size_t index) const requires(!is_void) {
    return begin()[index];
}

template <typename T>
constexpr Span<T> make_span(T *data, std::size_t size) {
    return {data, size};
}

} // namespace codespy
