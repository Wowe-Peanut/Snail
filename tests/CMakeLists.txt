cmake_minimum_required(VERSION 3.10)

# distance tests
add_executable(distances_test distances_test.cpp)
target_link_libraries(distances_test 
    PRIVATE snail_lib 
    PRIVATE Catch2::Catch2WithMain
)
add_test(NAME distances_test COMMAND distances_test)

# energy derivative tests
add_executable(finite_difference_test finite_difference_test.cpp)
target_link_libraries(finite_difference_test 
    PRIVATE snail_lib 
    PRIVATE Catch2::Catch2WithMain
)
add_test(NAME finite_difference_test COMMAND finite_difference_test)