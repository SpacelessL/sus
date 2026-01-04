file(GLOB_RECURSE SRCS *.cpp)

add_executable(sus_test ${SRCS})
target_link_libraries(sus_test sus)
