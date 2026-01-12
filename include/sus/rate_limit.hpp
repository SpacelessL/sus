#pragma once

#include <chrono>
#include <atomic>
#include <limits>
#include <optional>
#include <cstdint>

#include "debug.hpp"

namespace spaceless {

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
				, .last = typename clock::time_point(typename clock::duration(res->last))
				, .cur = typename clock::time_point(typename clock::duration(res->cur)) };
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
