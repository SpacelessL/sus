#pragma once

#include <ranges>
#include <concepts>
#include <regex>
#include <type_traits>

namespace spaceless {

inline constexpr double eps = 1e-15;

template <typename R, typename V>
concept range_of = std::ranges::range<R> && std::convertible_to<std::ranges::range_value_t<R>, V>;

template<size_t N>
struct string_literal {
	std::array<char, N> value;

	constexpr string_literal(const char (&str)[N + 1]) noexcept { std::ranges::copy_n(str, N, value.begin()); }
	constexpr operator std::string_view() const noexcept { return { value.data(), value.size() }; }
	constexpr size_t length() const noexcept { return N; }
	template<size_t I> requires(I < N)
	constexpr char get() const noexcept { return value[I]; }
	constexpr char operator [] (size_t idx) const { return value[idx]; }
	constexpr auto operator <=> (const string_literal &) const noexcept = default;
	constexpr bool operator == (const string_literal &) const noexcept = default;
};

template<size_t N>
string_literal(const char(&)[N]) -> string_literal<N - 1>;

template<typename... Args>
[[nodiscard]] auto make_reference_range(Args&... args) {
	return std::views::iota(0, int(sizeof...(args))) | std::views::transform([arr = std::vector{ &args... }](int i) -> decltype(auto) { return *arr[i]; });
}

template<typename Head, typename ...>
struct head_of { using type = Head; };

template<typename ...Args>
using head_of_t = typename head_of<Args...>::type;

template<typename ...Args>
struct tail_of { using type = std::tuple_element_t<sizeof...(Args) - 1, std::tuple<Args...>>; };

template<typename ...Args>
using tail_of_t = typename tail_of<Args...>::type;

}
