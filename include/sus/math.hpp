#pragma once

#include <concepts>
#include <ranges>

namespace spaceless {

template<std::floating_point T = double>
class accumulator final {
public:
	constexpr accumulator(T init = 0) noexcept : sum_(init), comp_(0) {}

	template<std::ranges::input_range Range>
	requires std::is_nothrow_convertible_v<std::ranges::range_value_t<Range>, T>
	constexpr explicit accumulator(Range &&range) noexcept : accumulator() { for (auto &&x : range) add(x); }

	constexpr accumulator(const accumulator &) noexcept = default;
	constexpr accumulator(accumulator &&) noexcept = default;
	constexpr accumulator &operator = (const accumulator &) noexcept = default;
	constexpr accumulator &operator = (accumulator &&) noexcept = default;

	constexpr accumulator &operator += (T x) noexcept { add(x); return *this; }
	constexpr accumulator &operator -= (T x) noexcept { return *this += -x; }

	[[nodiscard]] constexpr operator T() const noexcept { return sum_ + comp_; }

private:
	constexpr void add(T x) noexcept {
		T tmp = sum_ + x;
		if (abs(sum_) >= abs(x))
			comp_ += (sum_ - tmp) + x;
		else
			comp_ += (x - tmp) + sum_;
		sum_ = tmp;
	}
	static constexpr T abs(T x) noexcept { return x < T(0) ? -x : x; }

	T sum_, comp_;
};

template<std::ranges::input_range Range>
accumulator(Range &&) -> accumulator<std::remove_cv_t<std::ranges::range_value_t<Range>>>;

[[nodiscard]] constexpr auto squared(const auto &t) noexcept(noexcept(t * t)) { return t * t; }
[[nodiscard]] constexpr auto cubed(const auto &t) noexcept(noexcept(t * t * t)) { return t * t * t; }

}
