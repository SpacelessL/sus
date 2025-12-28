#pragma once

#include <format>
#include <stacktrace>
#include <source_location>

#include "macro.hpp"

namespace spaceless {

struct assert_error : std::runtime_error {
	std::stacktrace trace;
	std::source_location loc;
	assert_error(const std::string &err_msg, std::stacktrace trace, std::source_location loc) : std::runtime_error(err_msg), trace(std::move(trace)), loc(std::move(loc)) {}
};

namespace detail {
std::string get_error_reason(std::string_view reason, auto &&...args) { return (std::format("{}{}", reason, sizeof...(args) ? "\n\twherein:" : "") + ... + args); }
[[noreturn]] void report_error(std::string_view assertion, std::string_view reason = "", std::stacktrace trace = std::stacktrace::current(), std::source_location loc = std::source_location::current());
}

#define SHOW(x) std::format("{}:\t{}", #x, x)

#define SUS_DETAIL_SHOW_ARG(x) , "\n\t\t" + SHOW(x)
#define SUS_DETAIL_SHOW_ARGS(...) FOR_EACH(SUS_DETAIL_SHOW_ARG, __VA_ARGS__)

#define SUS_DETAIL_ASSERT_IMPL(assertion, value, ...) \
do { \
	if (!(value)) [[unlikely]] \
		detail::report_error(assertion \
			__VA_OPT__(, detail::get_error_reason(FIRST_ONLY(__VA_ARGS__) SUS_DETAIL_SHOW_ARGS(FIRST_IGNORED(__VA_ARGS__))))); \
} while (false)

#ifdef SUS_DISABLE_DEBUG_MESSAGES
#define ASSERT(value, ...) SUS_DETAIL_ASSERT_IMPL("ASSERT() failed", value)
#else
#define ASSERT(value, ...) SUS_DETAIL_ASSERT_IMPL("ASSERT(" #value ") failed", value, __VA_ARGS__)
#endif

#ifdef SUS_DISABLE_DEBUG_MACROS
#define DASSERT(...) ((void)0)
#else
#define DASSERT(...) ASSERT(__VA_ARGS__)
#endif

#define UNREACHABLE(...) SUS_DETAIL_ASSERT_IMPL("UNREACHABLE", false, __VA_ARGS__)
#define UNIMPLEMENTED(...) SUS_DETAIL_ASSERT_IMPL("UNIMPLEMENTED", false, __VA_ARGS__)

}
 