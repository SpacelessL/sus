#pragma once

#include <ctime>

namespace spaceless {

[[nodiscard]] std::tm *gmtime_safe(const time_t *timer);

}
