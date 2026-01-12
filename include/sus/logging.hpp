#pragma once

#include <chrono>
#include <thread>
#include <source_location>
#include <vector>
#include <string>
#include <format>
#include <queue>

#include "debug.hpp"
#include "rate_limit.hpp"
#include "scope_guard.hpp"
#include "enum.hpp"
#include "time.hpp"

namespace spaceless {

struct log_record;

class logger {
public:
	virtual void log(const log_record &record) noexcept = 0;
	virtual void flush() noexcept {}
	virtual ~logger() = default;
};

enum class log_level : uint8_t {
	debug = 1,
	info = 2,
	warning = 3,
	error = 4,
	critical = 5,
	DEBUG = debug,
	INFO = info,
	WARNING = warning,
	ERROR = error,
	CRITICAL = critical,
};

struct log_record {
	log_level level = log_level::info;
	std::chrono::time_point<std::chrono::system_clock> time;
	std::thread::id thread_id;
	std::source_location source_location;
	std::string message;
};

class logging {
public:
	struct init_option {
		enum class flag : uint8_t {
			none = 0,
			use_default_console_logger = 1 << 0,
			use_default_file_logger = 1 << 1,
			all = use_default_console_logger | use_default_file_logger,
		};
		using enum flag;
		flag flags = all;
		log_level level = log_level::info;
	};
	static void init(std::string_view program, init_option option);
	static void set_level(log_level l) noexcept;
	static log_level level() noexcept;
	static void register_logger(std::unique_ptr<logger> lg);
	static void register_wrapper_logger(std::unique_ptr<logger> lg);
	static void log(log_record record) noexcept;
	static void flush() noexcept;
};

template<>
struct is_bit_flags<logging::init_option::flag> : std::true_type {};

class log_dispatcher {
public:
	struct proxy {
		proxy(log_dispatcher *d, log_level l) noexcept : dispatcher(d), level(l) {}
		log_dispatcher *dispatcher;
		log_level level;

		template<typename ...Args>
		proxy &operator () (const std::format_string<Args...> &fmt, Args &&...vars) {
			dispatcher->log(level, fmt, std::forward<Args>(vars)...);
			return *this;
		}

		proxy &operator << (auto &&arg) {
			return operator()("{}", arg);
		}
	};

	log_dispatcher(std::source_location source_location) noexcept
		: source_location_(std::move(source_location)) {}

	proxy get_proxy(log_level level) noexcept { return { this, level }; }

	template<typename ...Args>
	void log(log_level level, const std::format_string<Args...> &fmt, Args &&...vars) {
		if (logging::level() > level) return;
		log_record record;
		record.level = level;
		record.time = std::chrono::system_clock::now();
		record.thread_id = std::this_thread::get_id();
		record.source_location = source_location_;
		record.message = std::format(fmt, std::forward<Args>(vars)...);
		logging::log(std::move(record));
	}

private:
	std::source_location source_location_;
};

#define SUS_DETAIL_LOG_ADD_DOT(...) .__VA_ARGS__

#ifndef SUS_LOG_LEVEL
#define SUS_LOG_LEVEL 1
#endif

#define SUS_DETAIL_LOG_IF_IMPL(lvl, cond, ...) \
uint8_t(::spaceless::log_level::lvl) < SUS_LOG_LEVEL || ::spaceless::log_level::lvl < ::spaceless::logging::level() || !(cond) \
	|| ![] noexcept { \
		static ::spaceless::time_rate_limited_counter<> _log_guard_ ## __LINE__; \
		return ::spaceless::rate_limit_guard(&_log_guard_ ## __LINE__); \
	}() FOR_EACH(SUS_DETAIL_LOG_ADD_DOT, __VA_ARGS__).tick() ? ::spaceless::log_dispatcher::proxy(nullptr, ::spaceless::log_level::critical) \
	: ::spaceless::log_dispatcher(std::source_location::current()).get_proxy(::spaceless::log_level::lvl)

#define LOG_IF(lvl, cond, ...) SUS_DETAIL_LOG_IF_IMPL(lvl, cond, __VA_ARGS__)
#define LOG(lvl, ...) LOG_IF(lvl, true, __VA_ARGS__)

#ifdef SUS_DISABLE_DEBUG_MACROS
#define DLOG_IF(lvl, ...) LOG_IF(lvl, false)
#define DLOG(lvl, ...) LOG_IF(lvl, false)
#else
#define DLOG_IF(lvl, cond, ...) LOG_IF(lvl, cond, __VA_ARGS__)
#define DLOG(lvl, ...) LOG_IF(lvl, true, __VA_ARGS__)
#endif

}

namespace std {
template<>
struct formatter<spaceless::log_record> {
	bool color = false, line_break = true, source = true, message = true;
	template<class ParseContext>
	[[nodiscard]] constexpr auto parse(ParseContext &ctx) {
		auto it = ctx.begin();
		for (; it != ctx.end() && *it != '}'; ++it) {
			switch (*it) {
			case 'c': color = true; break;
			case 's': source = true; message = false; break;
			case 'm': source = false; message = true; break;
			case 'f': line_break = false; break;
			default: throw std::format_error("Invalid format specifier for log_record");
			}
		}
		return it;
	}
	template<class FmtContext>
	typename FmtContext::iterator format(const spaceless::log_record &record, FmtContext &ctx) const {
		std::string_view level;
		std::string_view color_begin, color_end = "\033[0m", loc_color_begin = "\033[90m";
		switch (record.level) {
			case spaceless::log_level::debug: level = "D"; color_begin = "\033[90m"; break;
			case spaceless::log_level::info: level = "I"; color_begin = "\033[97m" ; break;
			case spaceless::log_level::warning: level = "W"; color_begin = "\033[93m"; break;
			case spaceless::log_level::error: level = "E"; color_begin = "\033[31;100m"; break;
			case spaceless::log_level::critical: level = "C"; color_begin = "\033[97;41m"; break;
			default: level = "U";
		}
		if (!color) color_begin = loc_color_begin = color_end = "";
		const auto &loc = record.source_location;

		auto out = ctx.out();

		auto write_sv = [&](std::string_view s) { for (char c : s) *out++ = c; };
		auto write_ch = [&](char c) { *out++ = c; };
		auto write_int = [&](auto v) {
			char buf[32];
			auto [p, ec] = std::to_chars(std::begin(buf), std::end(buf), v);
			if (ec == std::errc{}) write_sv({buf, static_cast<size_t>(p - buf)});
		};
		auto get_filename = [](std::string_view path) {
			size_t i = path.find_last_of("/\\");
			return i == std::string_view::npos ? path : path.substr(i + 1);
		};

		auto get_scope = [&](std::string_view begin, std::string_view end) {
			write_sv(begin);
			return spaceless::scope_guard(write_sv, end);
		};

		if (source) {
			std::string_view file = get_filename(std::string_view(loc.file_name()));

			{
				auto bracket_scope = get_scope("[", "]");
				auto scope = get_scope(loc_color_begin, color_end);
				{
					auto scope = get_scope("`", "`");
					write_sv(loc.function_name());
				}

				if (line_break) write_ch('\n'); else write_ch(' ');

				{
					auto scope = get_scope(color_begin, color_end);
					write_sv(level);
				}

				{
					auto scope = get_scope(loc_color_begin, color_end);
					out = std::format_to(out, "{}", record.thread_id);
					write_ch(' ');

					{
						char buffer[256];

						using namespace std::chrono;
						auto tp_sec = floor<seconds>(record.time);
						auto in_time_t = system_clock::to_time_t(tp_sec);
						auto ms = static_cast<int>(duration_cast<milliseconds>(record.time - tp_sec).count());

						if (std::strftime(buffer, sizeof(buffer), ":%F %T", ::spaceless::gmtime_safe(&in_time_t)))
							write_sv(buffer);

						write_ch('.');
						out = std::format_to(out, "{:03}", ms);

						if (std::strftime(buffer, sizeof(buffer), " %Ez", ::spaceless::gmtime_safe(&in_time_t)))
							write_sv(buffer);
					}
					write_ch(' ');

					write_sv(file);

					{
						auto scope = get_scope("(", ")");
						write_int(loc.line()); write_ch(':'); write_int(loc.column());
					}
				}
			}

			if (message) write_sv(": ");
		}

		if (message) {
			auto scope = get_scope(color_begin, color_end);
			write_sv(record.message);
		}

		return out;
	}
};
}
