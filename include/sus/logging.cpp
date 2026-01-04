#include <print>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <condition_variable>
#include <mutex>
#include <ranges>

#include "logging.hpp"

#include "stop_watch.hpp"
#include "internal/mpsc_queue.hpp"

namespace spaceless {

static void exception_fallback(const char *source, const char *exception, const char *reason, const log_record &record) {
	(void)std::fputs(source, stderr);
	(void)std::fputs(" has exception because of ", stderr);
	(void)std::fputs(exception, stderr);
	(void)std::fputs("(", stderr);
	(void)std::fputs(reason, stderr);
	(void)std::fputs("), original message: ", stderr);
	(void)std::fputs(record.message.c_str(), stderr);
}

static const char *to_string(std::ios_base::iostate state) {
	switch (state) {
	case std::ios_base::goodbit: return "goodbit";
	case std::ios_base::badbit: return "badbit";
	case std::ios_base::failbit: return "failbit";
	case std::ios_base::eofbit: return "eofbit";
	default: return "unknown";
	}
}

#define SUS_LOGGER_CATCH_EXCEPTIONS(source) \
	catch (const std::bad_alloc &e) { exception_fallback(source, "bad_alloc", e.what(), record); } \
	catch (const std::format_error &e) { exception_fallback(source, "format_error", e.what(), record); } \
	catch (const std::exception &e) { exception_fallback(source, "exception", e.what(), record); } \
	catch (...) { exception_fallback(source, "unknown_exception", "none", record); }

class logger_manager {
public:
	logger_manager() : mpsc_queue_(128) {
		to_be_logged_.reserve(128);
		thread_ = std::jthread([&](std::stop_token stop) {

			stop_watch<> timer;
			auto sleep = [&] {
				using namespace std::chrono_literals;
				if (timer.elapsed() < 50ms)
					std::this_thread::yield();
				else {
					std::unique_lock lock(flush_cv_mutex_);
					flush_cv_.wait_for(lock, 1ms, [&] {
						return need_flush_.load(std::memory_order_acquire) || stop.stop_requested();
					});
				}
			};
			while (!stop.stop_requested()) {
				if (send_all())
					timer.reset();
				if (need_flush_.load(std::memory_order_acquire) || flush_counter_.update(5)) {
					send_all();
					flush();
					flush_counter_.force_update();
				}

				sleep();
			}
			send_all();
			flush();
		});
	}

	void await_flush() {
		static std::mutex mutex;
		std::lock_guard guard(mutex);

		need_flush_.store(true, std::memory_order_release);
		flush_cv_.notify_one();
		std::unique_lock lock(flush_cv_mutex_);
		flush_cv_.wait(lock, [&] {
			return !need_flush_.load(std::memory_order_acquire);
		});
	}

	void register_logger(std::unique_ptr<logger> lg) {
		std::lock_guard guard(log_mutex_);
		loggers_.emplace_back(std::move(lg));
	}

	void register_wrapper_logger(std::unique_ptr<logger> lg) {
		wrapper_loggers_.emplace_back(std::move(lg));
	}

	void append(log_record record) noexcept {
		for (auto &&lg : wrapper_loggers_)
			lg->log(record);
		mpsc_queue_.enqueue(std::move(record));
	}

	~logger_manager() {
		thread_.request_stop();
		flush_cv_.notify_one();
		if (thread_.joinable())
			thread_.join();
		flush();
	}

private:
	bool send_all() noexcept {
		while (true) {
			auto record = mpsc_queue_.try_dequeue();
			if (!record) break;
			to_be_logged_.emplace_back(std::move(*record));
		}
		if (!to_be_logged_.empty()) {
			std::lock_guard log_guard(log_mutex_);
			for (auto &&record : to_be_logged_)
				for (auto &&logger : loggers_)
					logger->log(record);
			to_be_logged_.clear();
			return true;
		}
		return false;
	}

	void flush() noexcept {
		{
			std::lock_guard guard(log_mutex_);
			for (auto &&logger : loggers_)
				logger->flush();
		}
		need_flush_.store(false, std::memory_order_release);
		flush_cv_.notify_one();
	}

	std::vector<std::unique_ptr<logger>> loggers_, wrapper_loggers_;
	std::mutex log_mutex_, flush_cv_mutex_;
	std::condition_variable flush_cv_;
	std::atomic_bool need_flush_ = false;
	std::jthread thread_;

	std::vector<log_record> to_be_logged_;
	time_rate_limited_counter<> flush_counter_;
	mpsc_queue<log_record> mpsc_queue_;
};

namespace {
static logger_manager &get_logger_manager() {
	static logger_manager g_manager;
	return g_manager;
}
static log_level g_level = log_level::info;
}

class terminal_logger : public logger {
public:
	void log(const log_record &record) noexcept override {
		try {
			auto x = std::format("{:c}", record);
			std::printf("%s\n", x.c_str());
		}
		SUS_LOGGER_CATCH_EXCEPTIONS("terminal_logger")
	}
	void flush() noexcept override {
		(void)std::fflush(stdout);
	}
};

class file_logger : public logger {
public:
	static constexpr size_t kMaxFileSize = 100'000'000;
	file_logger(std::filesystem::path filename) : filename_(std::move(filename)) { advance(); }
	bool valid() const noexcept { return file_.is_open(); }

	void log(const log_record &record) noexcept override {
		try {
			if (!file_) {
				exception_fallback("file_logger", "io_error before output", to_string(file_.rdstate()), record);
				return;
			}
			auto s = std::format("{:f}\n", record);
			file_ << s;
			if (!file_)
				exception_fallback("file_logger", "io_error after output", to_string(file_.rdstate()), record);
			if ((p_ += s.length()) >= kMaxFileSize) advance();
		}
		SUS_LOGGER_CATCH_EXCEPTIONS("file_logger")
	}
	void flush() noexcept override { file_.flush(); }
private:
	void advance() {
		auto f = filename_;
		f += std::format(".{}.log", index_++);
		if (file_.is_open()) file_.close();
		file_.open(f);
		p_ = 0;
	}

	int index_ = 0;
	size_t p_ = 0;
	std::filesystem::path filename_;
	std::ofstream file_;
};

void logging::init(std::string_view program, init_option option) {
	if (contains_all(option.flags, init_option::flag::use_default_console_logger))
		register_logger(std::make_unique<terminal_logger>());
	if (contains_all(option.flags, init_option::flag::use_default_file_logger)) {
		auto filename = std::format("{}_{:%C%y%m%d-%H%M%S}", program, std::chrono::system_clock::now());
		auto fl = std::make_unique<file_logger>(filename);
		if (!fl->valid())
			LOG(error)("Create file logger {} failed.", filename);
		else
			register_logger(std::move(fl));
	}
	set_level(option.level);
}

void logging::set_level(log_level l) noexcept {
	g_level = l;
}

log_level logging::level() noexcept {
	return g_level;
}

void logging::register_logger(std::unique_ptr<logger> lg) {
	get_logger_manager().register_logger(std::move(lg));
}

void logging::register_wrapper_logger(std::unique_ptr<logger> lg) {
	get_logger_manager().register_wrapper_logger(std::move(lg));
}

void logging::log(log_record record) noexcept {
	get_logger_manager().append(std::move(record));
}

void logging::flush() noexcept {
	get_logger_manager().await_flush();
}

}
