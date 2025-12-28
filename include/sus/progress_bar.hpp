#pragma once

#include "logging.hpp"
#include "stop_watch.hpp"

namespace spaceless {

class progress_ptr;

class progress_bar {
public:
	static constexpr double kDefaultUpdateInterval = 3.0;
	template<typename Duration>
	progress_bar(const Duration &update_interval) requires(!std::convertible_to<Duration, double>) : progress_bar(std::chrono::duration_cast<std::chrono::duration<double>>(update_interval).count()) {}
	progress_bar(double update_interval = kDefaultUpdateInterval) : timer_(true), interval_(update_interval) { set_range(1); }

	[[nodiscard]] virtual bool cancelled() const { return cancelled_.load(std::memory_order_acquire); }
	virtual void cancel() { cancelled_.store(true, std::memory_order_release); }

	void set_range(double max) { set_range(0, max); }
	virtual void set_range(double min, double max);
	[[nodiscard]] virtual std::pair<double, double> range() const { return { min(), max() }; }
	[[nodiscard]] virtual double min() const { return min_; }
	[[nodiscard]] virtual double max() const { return max_; }

	virtual void add(double x);
	virtual void set(double x);
	[[nodiscard]] virtual double value() const { return value_.load(std::memory_order_acquire); }

	virtual void on_update(double v, const time_rate_limited_counter<>::result &res);

	[[nodiscard]] double speed() const noexcept { return speed_.load(std::memory_order_acquire); }

	[[nodiscard]] stop_watch<>::duration elapsed() const noexcept { return timer_.elapsed(); }

	virtual void set_text(std::string text) { name_ = std::move(text); }
	[[nodiscard]] virtual std::string_view text() const { return name_; }

	[[nodiscard]] progress_ptr sub(double weight);

	virtual ~progress_bar() = default;

private:
	void try_update(double v);

	std::atomic_bool cancelled_ = false;
	double min_ = 0, max_ = 1;
	std::atomic<double> value_ = 0;
	stop_watch<> timer_;
	std::string name_;

	std::atomic<double> last_value_ = 0, speed_ = 0;
	time_rate_limited_counter<> counter_;
	const double interval_;
};

class logging_progress_bar : public progress_bar {
public:
	[[nodiscard]] static progress_ptr create(std::string text = "progress", double interval = kDefaultUpdateInterval, log_level level = log_level::info
		, std::source_location loc = std::source_location::current());

	logging_progress_bar(std::string text, double interval, log_level level, std::source_location loc)
		: progress_bar(interval), loc_(std::move(loc)), level_(level) { progress_bar::set_text(std::move(text)); }

	virtual void on_update(double v, const time_rate_limited_counter<>::result &res) override;

private:
	std::source_location loc_;
	log_level level_;
};

class progress_ptr {
public:
	progress_ptr() : p_(std::make_shared<progress_bar>()) {}
	progress_ptr(std::nullopt_t) : progress_ptr() {}
	progress_ptr(std::shared_ptr<progress_bar> p) : p_(std::move(p)) {}
	progress_ptr(const progress_ptr &) = default;
	progress_ptr(progress_ptr &&) noexcept = default;
	progress_ptr &operator = (const progress_ptr &) = default;
	progress_ptr &operator = (progress_ptr &&) noexcept = default;

	const progress_bar *operator -> () const noexcept { return p_.get(); }
	progress_bar *operator -> () noexcept { return p_.get(); }
	const progress_bar &operator * () const noexcept { return *p_; }
	progress_bar &operator * () noexcept { return *p_; }

private:
	std::shared_ptr<progress_bar> p_;
};

}
