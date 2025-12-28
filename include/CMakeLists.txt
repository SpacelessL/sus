set(SOURCES
	sus/internal/mpsc_queue.hpp
	sus/arguments.hpp
	sus/arguments.cpp
	sus/misc.hpp
	sus/benchmark.hpp
	sus/logging.hpp
	sus/logging.cpp
	sus/debug.hpp
	sus/debug.cpp
	sus/enum.hpp
	sus/macro.hpp
	sus/math.hpp
	sus/stop_watch.hpp
	sus/time.hpp
	sus/time.cpp
	sus/progress_bar.hpp
	sus/progress_bar.cpp
	sus.hpp
	sus.cpp
)

add_library(sus ${SOURCES})
set_target_properties(sus PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(sus PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	target_link_libraries(sus PUBLIC stdc++exp)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_link_libraries(sus PUBLIC stdc++exp)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	target_compile_options(sus PUBLIC "/Zc:preprocessor")
endif()