#include <iostream>
#include <mutex>
#include <ctime>
#include <complex>

#include "sus.hpp"

using namespace spaceless;

namespace {

static void multi_thread(auto &&func) {
	std::vector<std::jthread> threads;
	for (int i = 0; i < 8; i++)
		threads.emplace_back([&] { func(); });
	for (auto &&t : threads)
		t.join();
}

static void use_logging() {
	multi_thread([] {
		for (int i = 0; i < 1000000; i++)
			LOG(debug, every(100000)) << i;
	});
	multi_thread([] {
		for (int i = 0; i < 50; i++) {
			LOG(info, interval(0.3)) << i;
			std::this_thread::sleep_for(std::chrono::milliseconds(30));
		}
	});
	multi_thread([] {
		for (int i = 0; i < 10; i++) {
			std::this_thread::sleep_for(std::chrono::milliseconds(30));
			LOG(warning, first(10)) << i;
		}
	});
	multi_thread([] {
		for (int i = 0; i < 1000000; i++)
			LOG(error, every(100), first(10)) << i;
	});
	multi_thread([] {
		for (int i = 0; i < 100; i++) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			LOG(critical, interval(0.3), every(3), first(5)) << i;
		}
	});
}

static void use_assert() {
	int x = -1, y = 2;
	try {
		ASSERT(x > y);
	} catch (const assert_error &) {}
	try {
		ASSERT(x > y, "x should be bigger than y");
	} catch (const assert_error &) {}
	try {
		ASSERT(x > y, "x should be bigger than y! What are they?", x, y, x > y);
	} catch (const assert_error &) {}
	try {
		UNREACHABLE("WTF");
	} catch (const assert_error &) {}
	try {
		UNIMPLEMENTED("Hello", x);
	} catch (const assert_error &) {}
	auto time_consuming = [] { LOG(error)("I should never be called!"); return 1; };
	ASSERT(x < y, "My bad, x should be smaller than y", x, y, time_consuming());
}

static void use_progress_bar() {
	{
		auto ptr = logging_progress_bar::create("test", 0);
		int r = 1000000;
		ptr->set_range(r);
		for (int i = 0; i < r; i++) {
			ptr->add(1);
		}
		LOG(info)("done!");
	}
	//{
	//	auto ptr = logging_progress_bar::create("test", 0.1);
	//	int r = 1'000'000'000;
	//	ptr->set_range(r);
	//	for (int i = 0; i < r; i++) {
	//		ptr->add(1);
	//	}
	//	LOG(info)("done!");
	//}
}

static void test_empty_logging() {
	static constexpr int kT = 8;
	static constexpr int kNumLogs = 1'000'000;
	std::vector<std::jthread> threads;
	for (int i = 0; i < kT; i++)
		threads.emplace_back([id = i] {
			for (int i = 0; i < kNumLogs / kT; i++)
				LOG(info)("T{} {}", id, i);
		});
	for (auto &&t : threads)
		t.join();
}

}

int main(int argc, char **argv) {
	init(argc, argv, { .logging_option = logging::init_option::all });
	logging::set_level(log_level::debug);

	timer t("program");

	//test_empty_logging();

	//use_progress_bar();

	//use_logging();

	//use_assert();

	logging::flush();
	return 0;
}
