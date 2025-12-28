#pragma once

#include <utility>

#include "logging.hpp"

namespace spaceless {

template<typename StopWatch>
class pause_guard {
public:
	explicit pause_guard(StopWatch *p) noexcept : p_(p), was_running_(p->running()) { p_->pause(); }
	~pause_guard() { if (was_running_) p_->resume(); }

	pause_guard(const pause_guard &) = delete;
	pause_guard(pause_guard &&) = delete;
	pause_guard &operator=(const pause_guard &) = delete;
	pause_guard &operator=(pause_guard &&) = delete;
private:
	StopWatch *p_;
	bool was_running_;
};

template<typename StopWatch>
class run_guard {
public:
	explicit run_guard(StopWatch *p) noexcept : p_(p), was_paused_(p->paused()) { p_->resume(); }
	~run_guard() { if (was_paused_) p_->pause(); }

	run_guard(const run_guard &) = delete;
	run_guard(run_guard &&) = delete;
	run_guard &operator=(const run_guard &) = delete;
	run_guard &operator=(run_guard &&) = delete;
private:
	StopWatch *p_;
	bool was_paused_;
};

struct stop_watch_no_logging {
	template<typename ...Args>
	void on_action(Args &&...) const noexcept {}
};

struct stop_watch_logging {
	std::string name;
	log_level level;

	stop_watch_logging(std::string name, log_level level) : name(std::move(name)), level(level) {}

	template<typename StopWatch>
	void on_action(std::string_view action, StopWatch *stop_watch, std::source_location source_location) const noexcept {
		log_dispatcher(std::move(source_location)).get_proxy(level)("[{}] {}, total : {}, lap : {}", name, action, stop_watch->elapsed(), stop_watch->lap_elapsed());
	}

	void on_action(std::string_view action, std::source_location source_location) const noexcept {
		log_dispatcher(std::move(source_location)).get_proxy(level)("[{}] {}", name, action);
	}
};

template<typename LoggingPolicy = stop_watch_no_logging, typename Clock = std::chrono::steady_clock>
class stop_watch {
public:
	using duration = std::chrono::duration<double>;
	using clock = Clock;

	stop_watch(bool pause_on_start = false) noexcept requires std::is_same_v<LoggingPolicy, stop_watch_no_logging>
		: past_(duration::zero()) { if (!pause_on_start) resume(); }
	stop_watch(std::string name, log_level level = log_level::info, bool pause_on_start = false) noexcept requires std::is_same_v<LoggingPolicy, stop_watch_logging>
		: policy_(std::move(name), level), past_(duration::zero()) { if (!pause_on_start) resume(); }

	stop_watch(const stop_watch &) = delete;
	stop_watch(stop_watch &&) = delete;
	stop_watch &operator = (const stop_watch &) = delete;
	stop_watch &operator = (stop_watch &&) = delete;
	
	[[nodiscard]] duration elapsed() const noexcept { return past_ + lap_elapsed(); }
	[[nodiscard]] duration lap_elapsed() const noexcept { auto now = clock::now(); return now - start_.value_or(now); }

	[[nodiscard]] bool paused() const noexcept { return !start_.has_value(); }
	[[nodiscard]] bool running() const noexcept { return start_.has_value(); }

	void pause() noexcept {
		if (paused()) {
			policy_.on_action("is already paused", std::source_location::current());
			return;
		}
		past_ += lap_elapsed();
		start_.reset();
		policy_.on_action("paused", std::source_location::current());
	}
	void resume() noexcept {
		if (running()) {
			policy_.on_action("is already running", std::source_location::current());
			return;
		}
		start_ = clock::now();
		policy_.on_action(past_ != duration::zero() ? "resumed" : "started", std::source_location::current());
	}
	void reset(bool pause_on_start = false) noexcept {
		policy_.on_action("reset", this, std::source_location::current());
		start_.reset();
		past_ = duration::zero();
		if (!pause_on_start) resume();
	}
	duration lap() noexcept {
		if (!running()) {
			policy_.on_action("isn\'t running", std::source_location::current());
			return duration::zero();
		}
		auto now = clock::now();
		auto d = now - *start_;
		past_ += d;
		start_ = now;
		policy_.on_action("ended", this, std::source_location::current());
		return d;
	}

	[[nodiscard]] auto pause_scope() noexcept { return pause_guard{ this }; }
	[[nodiscard]] auto run_scope() noexcept { return pause_guard{ this }; }

	~stop_watch() {
		policy_.on_action("ended", this, std::source_location::current());
	}

private:

	[[no_unique_address]] LoggingPolicy policy_;
	std::optional<std::chrono::time_point<clock>> start_;
	duration past_;
};

using timer = stop_watch<stop_watch_logging>;

}
