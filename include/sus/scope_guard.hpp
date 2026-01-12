#pragma once

#include <tuple>
#include <exception>
#include <type_traits>

#include "macro.hpp"

namespace spaceless {

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

#define SCOPE_EXIT(...) auto ANON(scope_guard) = ::spaceless::scope_guard(__VA_ARGS__)
#define SCOPE_SUCC(...) auto ANON(scope_guard) = ::spaceless::scope_guard_without_exception(__VA_ARGS__)
#define SCOPE_FAIL(...) auto ANON(scope_guard) = ::spaceless::scope_guard_with_exception(__VA_ARGS__)

}
