cmake_minimum_required(VERSION 3.5)
project(euler-function)

include_directories("..")

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -I.. -Wall -Wextra")

set(SOURCES src/main.c src/vector.c src/vector.h)
set(VGFLAGS
	--quiet --leak-check=full --show-leak-kinds=all
	--track-origins=yes --error-exitcode=1)

add_executable(exampleproject ${SOURCES})

add_executable(test-exampleproject ${SOURCES})
set_target_properties(test-exampleproject
	PROPERTIES COMPILE_FLAGS "-g -DSNOW_ENABLED")

add_custom_target(run-test
	COMMAND valgrind ${VGFLAGS} ./test-exampleproject
	DEPENDS test-exampleproject)
