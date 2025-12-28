#include <regex>

#include "debug.hpp"
#include "logging.hpp"

namespace spaceless {
namespace detail {

void report_error(std::string_view assertion, std::string_view reason, std::stacktrace trace, std::source_location loc) {
	log_dispatcher dispatcher(loc);
	dispatcher.get_proxy(log_level::critical)("\n{}{}{}\nStacktrace:\n\t{}", assertion, reason.empty() ? "." : ": ", reason, std::regex_replace(std::to_string(trace), std::regex("\n"), "\n\t"));
	logging::flush();
	throw assert_error(std::format("{}{}{}", assertion, reason.empty() ? "" : " : ", reason), std::move(trace), loc);
}

}
}
