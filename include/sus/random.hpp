#pragma once

#include <random>
#include <concepts>

namespace spaceless {

inline std::mt19937 &get_mt19937() {
	thread_local std::mt19937 gen(std::random_device{}());
	return gen;
}

template<std::integral T>
[[nodiscard]] T get_random(T min, T max) {
	return std::uniform_int_distribution<T>(min, max)(get_mt19937());
}

template<std::floating_point T>
[[nodiscard]] T get_random(T min, T max) {
	return std::uniform_real_distribution<T>(min, max)(get_mt19937());
}

}
