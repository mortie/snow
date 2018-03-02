cmake_minimum_required(VERSION 3.5)
project(euler-function)

include_directories("..")

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -std=c11")

set(SOURCES src/main.c src/vector.c src/vector.h)
set(VGFLAGS --quiet --leak-check=full --show-leak-kinds=all --track-origins=yes)

add_executable(exampleproject ${SOURCES})
set_target_properties(exampleproject PROPERTIES COMPILE_FLAGS "-DSNOW_DISABLED")

add_executable(exampleproject-test ${SOURCES})
set_target_properties(exampleproject-test PROPERTIES COMPILE_FLAGS "-g")

add_custom_target(run-tests
	COMMAND valgrind ${VGFLAGS} ./exampleproject-test
	DEPENDS exampleproject-test)
