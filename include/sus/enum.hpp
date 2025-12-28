#pragma once

#include <utility>
#include <type_traits>

namespace spaceless {

template<typename>
struct is_bit_flags : std::false_type {};
template<typename T>
inline constexpr bool is_bit_flags_v = is_bit_flags<T>::value;

template<typename T>
concept BitFlags = is_bit_flags_v<T> && std::is_enum_v<T>;

template<BitFlags T>
constexpr T operator ~ (T lhs) noexcept { return T(~std::to_underlying(lhs)); }

template<BitFlags T>
constexpr T operator & (T lhs, T rhs) noexcept { return T(std::to_underlying(lhs) & std::to_underlying(rhs)); }
template<BitFlags T>
constexpr T operator | (T lhs, T rhs) noexcept { return T(std::to_underlying(lhs) | std::to_underlying(rhs)); }
template<BitFlags T>
constexpr T operator ^ (T lhs, T rhs) noexcept { return T(std::to_underlying(lhs) ^ std::to_underlying(rhs)); }

template<BitFlags T>
constexpr T &operator &= (T &lhs, T rhs) noexcept { return lhs = lhs & rhs; }
template<BitFlags T>
constexpr T &operator |= (T &lhs, T rhs) noexcept { return lhs = lhs | rhs; }
template<BitFlags T>
constexpr T &operator ^= (T &lhs, T rhs) noexcept { return lhs = lhs ^ rhs; }

template<BitFlags T>
constexpr bool contains_all(T t, T option) noexcept { return (t & option) == option; }
template<BitFlags T>
constexpr bool contains_any(T t, T options) noexcept { return (t & options) != T(0); }
template<BitFlags T>
constexpr bool contains_none(T t, T options) noexcept { return !contains_any(t, options); }

template<BitFlags auto Option>
constexpr bool contains_all(decltype(Option) t) noexcept { return (t & Option) == Option; }
template<BitFlags auto Option>
constexpr bool contains_any(decltype(Option) t) noexcept { return (t & Option) != decltype(Option)(0); }
template<BitFlags auto Option>
constexpr bool contains_none(decltype(Option) t) noexcept { return !contains_any<Option>(t); }

}
