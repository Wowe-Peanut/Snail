# Taken from TAMU Dr. Shinjiro Sueda's assignments and altered by Gemini

cmake_minimum_required(VERSION 3.10)
set(CMAKE_BUILD_TYPE Release)

project(snail)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.h")
file(GLOB_RECURSE GLSL "resources/*.glsl")


list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")


add_library(snail_lib STATIC ${SOURCES} ${HEADERS} ${GLSL})


# GLM
set(GLM_INCLUDE_DIR "$ENV{GLM_INCLUDE_DIR}")

# GLFW
set(GLFW_DIR "$ENV{GLFW_DIR}")
option(GLFW_BUILD_EXAMPLES "OFF" OFF)
option(GLFW_BUILD_TESTS "OFF" OFF)
option(GLFW_BUILD_DOCS "OFF" OFF)

if(CMAKE_BUILD_TYPE MATCHES Release)
    add_subdirectory(${GLFW_DIR} ${GLFW_DIR}/release)
else()
    add_subdirectory(${GLFW_DIR} ${GLFW_DIR}/debug)
endif()

# GLEW
set(GLEW_DIR "$ENV{GLEW_DIR}")

# Eigen
set(EIGEN3_INCLUDE_DIR "$ENV{EIGEN3_INCLUDE_DIR}")

# Public library to inherit all dependencies
target_include_directories(snail_lib PUBLIC 
    src/utils
    src/rendering
    src/physics
    src/extern
    ${GLM_INCLUDE_DIR}
    ${EIGEN3_INCLUDE_DIR}
    ${GLEW_DIR}/include
    ${GLFW_DIR}/include
)

target_link_libraries(snail_lib PUBLIC 
    glfw 
    ${GLEW_DIR}/lib/libGLEW.a 
    GL
)

# main executable
add_executable(${CMAKE_PROJECT_NAME} src/main.cpp)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE snail_lib)

# use c++ 17
set_target_properties(snail_lib PROPERTIES CXX_STANDARD 17)
set_target_properties(snail_lib PROPERTIES LINKER_LANGUAGE CXX)

# compiler flags
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wno-maybe-uninitialized -Wall -pedantic)
endif()

# Enable Testing 
enable_testing()
include(FetchContent)
FetchContent_Declare(
  catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(catch2)

add_subdirectory(tests)