#pragma once

#include "sus/logging.hpp"
#include "sus/benchmark.hpp"
#include "sus/debug.hpp"
#include "sus/macro.hpp"
#include "sus/math.hpp"
#include "sus/misc.hpp"
#include "sus/progress_bar.hpp"
#include "sus/stop_watch.hpp"
#include "sus/enum.hpp"

namespace spaceless {

struct init_option {
	logging::init_option logging_option;
};

void init(int argc, char **argv, init_option option = {});

}