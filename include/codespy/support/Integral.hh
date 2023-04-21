#pragma once

#include <cstdint>

namespace codespy {

template <typename>
inline constexpr bool is_integral = false;
template <>
inline constexpr bool is_integral<bool> = true;
template <>
inline constexpr bool is_integral<char> = true;
template <>
inline constexpr bool is_integral<std::int8_t> = true;
template <>
inline constexpr bool is_integral<std::int16_t> = true;
template <>
inline constexpr bool is_integral<std::int32_t> = true;
template <>
inline constexpr bool is_integral<std::int64_t> = true;
template <>
inline constexpr bool is_integral<std::uint8_t> = true;
template <>
inline constexpr bool is_integral<std::uint16_t> = true;
template <>
inline constexpr bool is_integral<std::uint32_t> = true;
template <>
inline constexpr bool is_integral<std::uint64_t> = true;

template <typename T>
inline constexpr bool is_signed = T(-1) < T(0);

template <typename T>
concept Integral = is_integral<T>;

template <typename T>
concept SignedIntegral = is_integral<T> && is_signed<T>;

template <typename T>
concept UnsignedIntegral = is_integral<T> && !is_signed<T>;

} // namespace codespy
