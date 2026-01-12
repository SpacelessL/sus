#include "progress_bar.hpp"

#include <cmath>

namespace spaceless {

class sub_progress_bar : public progress_bar {
public:
	sub_progress_bar(progress_bar *p, double weight) : p_(p), weight_(weight) {}
	virtual bool cancelled() const override { return p_->cancelled(); }
	virtual void cancel() override { p_->cancel(); }
	virtual void set_range(double min, double max) override { min_ = min; max_ = max; }
	virtual double min() const override { return min_; }
	virtual double max() const override { return max_; }
	virtual void add(double value) override { value_.fetch_add(value, std::memory_order_release); p_->add(value / (max_ - min_) * weight_); }
	virtual double value() const override { return value_.load(std::memory_order_acquire); }
	virtual void set_text(std::string text) override { p_->set_text(std::move(text)); }
	virtual std::string_view text() const override { return p_->text(); }

private:
	progress_bar *p_;
	double min_ = 0, max_ = 1, weight_ = 1;
	std::atomic<double> value_;
};

void progress_bar::set_range(double min, double max) {
	min_ = min; max_ = max;
	value_.store(min, std::memory_order_acquire);
	last_value_.store(min, std::memory_order_acquire);
	speed_.store(0, std::memory_order_acquire);
	counter_.force_update();
	timer_.reset(false);
}

void progress_bar::add(double x) {
	double v = value_.fetch_add(x, std::memory_order_release) + x;
	try_update(v);
}

void progress_bar::set(double x) {
	value_.store(x, std::memory_order_release);
	try_update(x);
}

void progress_bar::try_update(double v) {
	if (auto res = counter_.update(interval_); res) on_update(v, *res);
}

void progress_bar::on_update(double v, const time_rate_limited_counter<>::result &res) {
	auto t = std::chrono::duration_cast<std::chrono::duration<double>>(res.cur - res.last);
	double d = last_value_.load(std::memory_order_acquire);
	while (!last_value_.compare_exchange_weak(d, value(), std::memory_order_acq_rel, std::memory_order_acquire)) {}

	double last_speed = speed(), cur_speed = (v - d) / t.count(), alpha = last_speed == 0.0 ? 0.0 : std::pow(0.5, t.count());
	double new_speed;
	if (std::isnan(last_speed)) new_speed = cur_speed;
	else if (std::isnan(cur_speed)) new_speed = last_speed;
	else if (std::isinf(last_speed)) new_speed = cur_speed;
	else if (std::isinf(cur_speed)) new_speed = last_speed;
	else new_speed = std::lerp(cur_speed, last_speed, alpha);

	speed_.store(new_speed, std::memory_order_release);
}

progress_ptr progress_bar::sub(double weight) {
	return { std::make_shared<sub_progress_bar>(this, weight) };
}

progress_ptr logging_progress_bar::create(std::string text, double interval, log_level level, std::source_location loc) {
	return { std::make_shared<logging_progress_bar>(std::move(text), interval, level, std::move(loc)) };
}

static std::string duration_to_text(std::chrono::duration<double> d, int num_units = 2) {
	using namespace std::chrono;
	if (d.count() < 0) return "-" + duration_to_text(-d, num_units);

	if (num_units <= 0 || num_units > 11) num_units = 11;

	std::string ret;
	ret.reserve(num_units * 15);
	int used = 0;
	auto process = [&]<typename D>(const char *unit) {
		auto n = floor<D>(d);
		d -= n;
		constexpr bool is_second = std::is_same_v<D, seconds>;
		if (used < num_units && (n.count() || !ret.empty() || is_second)) {
			++used;
			if (!ret.empty()) ret += " ";
			if (is_second && used < num_units) {
				d += n;
				ret += std::format("{:.3f} {}{}", d.count(), unit, d.count() == 1.0 ? "" : "s");
			}
			else
				ret += std::format("{} {}{}", n.count(), unit, n.count() == 1 ? "" : "s");
		}
	};

	process.operator()<duration<int64_t, std::ratio_multiply<years::period, std::giga>>>("gigayear");
	process.operator()<duration<int16_t, std::ratio_multiply<years::period, std::mega>>>("megayear");
	process.operator()<duration<int16_t, std::ratio_multiply<years::period, std::kilo>>>("kiloyear");
	process.operator()<years>("year");
	process.operator()<months>("month");
	process.operator()<weeks>("week");
	process.operator()<days>("day");
	process.operator()<hours>("hour");
	process.operator()<minutes>("minute");
	process.operator()<seconds>("second");

	return ret;
}

void logging_progress_bar::on_update(double v, const time_rate_limited_counter<>::result &res) {
	progress_bar::on_update(v, res);

	log_dispatcher ld(loc_);
	double eta = (max() - v) / speed();
	double r = max() - min();
	ld.log(level_, "{} :\n\t{:g} / {:g} ({:.2f}%), {:.2e}/s\n"
		"\tElapsed:   {}\n\tRemaining: {}"
		, text(), v, r, v / r * 100, speed()
		, duration_to_text(elapsed()), duration_to_text(std::chrono::duration<double>(eta)));
}

}
