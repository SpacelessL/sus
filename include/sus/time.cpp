#include "time.hpp"

namespace spaceless {

std::tm *gmtime_safe(const time_t *timer) {
	thread_local std::tm ret;
#if defined(_WIN32)
	return ::gmtime_s(&ret, timer) ? nullptr : &ret;
#elif defined(__STDC_LIB_EXT1__) && !defined(__APPLE__)
	return ::gmtime_s(timer, &ret) ? nullptr : &ret;
#elif defined(__unix__) || defined(__APPLE__)
	return ::gmtime_r(timer, &ret) ? nullptr : &ret;
#else
	static std::mutex mutex;
	std::lock_guard guard(mutex);
	if (auto p = ::gmtime(timer)) {
		ret = *p;
		return &ret;
	}
	return nullptr;
#endif
}

}
