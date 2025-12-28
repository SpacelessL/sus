file(GLOB_RECURSE SRCS *.cpp)

add_executable(test ${SRCS})
target_link_libraries(test sus)
