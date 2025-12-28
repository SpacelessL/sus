#include "sus.hpp"

namespace spaceless {

void init(int argc, char** argv, init_option option) {
	logging::init(argv[0], option.logging_option);
}

}
