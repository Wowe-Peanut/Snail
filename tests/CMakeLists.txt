cmake_minimum_required(VERSION 3.10)

add_executable(distances_test
	${CMAKE_SOURCE_DIR}/tests/distances_test.cpp
	${CMAKE_SOURCE_DIR}/src/physics/distances.cpp
)

target_include_directories(distances_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(distances_test PRIVATE Catch2::Catch2WithMain)

add_test(NAME distances_test COMMAND distances_test)