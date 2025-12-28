#pragma once

#include <ranges>
#include <random>
#include <concepts>
#include <atomic>
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

// get random number in [0.0, 1.0]
template<std::floating_point T = double>
[[nodiscard]] T get_random() {
	thread_local std::mt19937 gen(std::random_device{}());
	thread_local std::uniform_real_distribution<T> dis(0, 1);
	return dis(gen);
}

template<typename... Args>
[[nodiscard]] auto make_reference_range(Args&... args) {
	return std::views::iota(0, int(sizeof...(args))) | std::views::transform([arr = std::vector{ &args... }](int i) -> decltype(auto) { return *arr[i]; });
}

namespace detail {
template<typename T>
struct save_type {
	using type = std::conditional_t<std::is_lvalue_reference_v<T>, T, std::remove_cvref_t<T>>;
};
template<typename T>
using save_type_t = typename save_type<T>::type;
}

template<typename Func, typename... Args>
class scope_guard {
public:
	using arg_tuple_t = std::tuple<detail::save_type_t<Args>...>;
	using func_t = detail::save_type_t<Func>;

	template<typename F, typename... A>
	scope_guard(F &&func, A&&... args) : active_(true), func_(std::forward<F>(func)), args_(std::forward<A>(args)...) {}
	~scope_guard() { if (active_) std::apply(func_, std::forward<arg_tuple_t>(args_)); }
	void dismiss() noexcept { active_ = false; }
	void rehire() noexcept { active_ = true; }

	scope_guard(const scope_guard &) = delete;
	scope_guard &operator = (const scope_guard &) = delete;

	scope_guard(scope_guard &&rhs) noexcept(std::is_nothrow_move_constructible_v<func_t> && std::is_nothrow_move_constructible_v<arg_tuple_t>)
		: active_(rhs.active_), func_(std::move(rhs.func_)), args_(std::move(rhs.args_)) { rhs.active_ = false; }

private:
	bool active_;
	func_t func_;
	arg_tuple_t args_;
};

template<bool Success, typename Func, typename... Args>
class scope_guard_for_exception {
public:
	template<typename F, typename... A>
	scope_guard_for_exception(F &&func, A&&... args) : guard_(std::forward<F>(func), std::forward<A>(args)...) {
		count_ = std::uncaught_exceptions();
	}

	void dismiss() noexcept { guard_.dismiss(); }
	void rehire() noexcept { guard_.rehire(); }

	~scope_guard_for_exception() {
		if (Success != (count_ == std::uncaught_exceptions()))
			guard_.dismiss();
	}

private:
	int count_;
	scope_guard<Func, Args...> guard_;
};

template<typename Func, typename... Args>
using scope_guard_without_exception = scope_guard_for_exception<true, Func, Args...>;
template<typename Func, typename... Args>
using scope_guard_with_exception = scope_guard_for_exception<false, Func, Args...>;

template<typename Func, typename... Args>
scope_guard(Func&&, Args&&...) -> scope_guard<Func, Args...>;
template<bool Success, typename Func, typename... Args>
scope_guard_for_exception(Func&&, Args&&...) -> scope_guard_for_exception<Success, Func, Args...>;

#define SCOPE_EXIT(...) auto ANON(scope_guard) = scope_guard(__VA_ARGS__)
#define SCOPE_SUCC(...) auto ANON(scope_guard) = scope_guard_without_exception(__VA_ARGS__)
#define SCOPE_FAIL(...) auto ANON(scope_guard) = scope_guard_with_exception(__VA_ARGS__)

template<typename T>
struct rate_limit_result {
	int64_t index;
	T last, cur;
};

class rate_limited_counter {
public:
	using result = rate_limit_result<int64_t>;
	[[nodiscard]] std::optional<result> update(int64_t cur, int64_t interval) noexcept {
		int64_t last = last_.load(std::memory_order_acquire);
		
		do { if (last > cur - interval) return std::nullopt; }
		while (!last_.compare_exchange_weak(last, cur, std::memory_order_acq_rel, std::memory_order_acquire));

		return result{ .index = n_.fetch_add(1, std::memory_order_release) + 1, .last = last, .cur = cur };
	}
	[[nodiscard]] int64_t current() const noexcept {
		return n_.load(std::memory_order_acquire);
	}
	void reset() noexcept { n_ = 0; last_ = std::numeric_limits<int64_t>::lowest(); }

private:
	std::atomic_int64_t n_ = 0, last_ = std::numeric_limits<int64_t>::lowest();
};

template<typename Clock = std::chrono::steady_clock>
class time_rate_limited_counter {
public:
	using clock = Clock;
	using result = rate_limit_result<typename clock::time_point>;

	template<typename Duration>
	[[nodiscard]] auto update(const Duration &d) noexcept requires(!std::convertible_to<Duration, double>) {
		return update(std::chrono::duration_cast<std::chrono::duration<double>>(d).count());
	}
	[[nodiscard]] std::optional<result> update(std::chrono::duration<double> interval) noexcept {
		if (auto res = counter_.update(current().time_since_epoch().count(), std::chrono::duration_cast<typename clock::duration>(interval).count()); res)
			return result{ .index = res->index
				, .last = clock::time_point(clock::duration(res->last))
				, .cur = clock::time_point(clock::duration(res->cur)) };
		return std::nullopt;
	}
	[[nodiscard]] std::optional<result> update(double interval_seconds) noexcept {
		return update(std::chrono::duration<double>{ interval_seconds });
	}
	std::optional<result> force_update() noexcept { return update(0); }
	[[nodiscard]] static typename clock::time_point current() { return std::chrono::steady_clock::now(); }
private:
	rate_limited_counter counter_;
};

template<typename Clock = std::chrono::steady_clock>
class rate_limit_guard {
public:
	using clock = Clock;

	rate_limit_guard(time_rate_limited_counter<Clock> *counter) noexcept : counter_(counter) { ASSERT(counter); }

	rate_limit_guard &first(int n) noexcept { first_ = n; return *this; }
	rate_limit_guard &every(int n) noexcept { ASSERT(n > 0, "Interval needs to be greater than zero", n); n_ = n; return *this; }
	rate_limit_guard &interval(std::chrono::duration<double> d) noexcept {
		ASSERT(d >= std::chrono::duration<double>::zero(), "Interval needs to be greater than or equal to zero", d);
		interval_ = d;
		return *this;
	}
	rate_limit_guard &interval(double seconds) noexcept { return interval(std::chrono::duration<double>{ seconds }); }

	[[nodiscard]] bool tick() const noexcept {
		auto n = counter_->update(interval_);
		if (!n || (n_ != 1 && n->index % n_ != 1)) return false;
		return first_ < 0 || n->index <= first_ * n_;
	}

private:
	time_rate_limited_counter<Clock> *counter_;
	int64_t first_ = -1, n_ = 1;
	std::chrono::duration<double> interval_{ 0 };
};

}
