CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
set(CMAKE_BUILD_TYPE Release)

# Name of the project
PROJECT(sim)


# Use glob to get the list of all source files.
# We don't really need to include header and resource files to build, but it's
# nice to have them also show up in IDEs.
FILE(GLOB_RECURSE SOURCES "src/*.cpp")
FILE(GLOB_RECURSE HEADERS "src/*.h")
FILE(GLOB_RECURSE GLSL "resources/*.glsl")


# Set the executable.
ADD_EXECUTABLE(${CMAKE_PROJECT_NAME} ${SOURCES} ${HEADERS} ${GLSL})

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC 
    src/utils
    src/rendering
    src/physics
	src/extern
)

# GLM
SET(GLM_INCLUDE_DIR "$ENV{GLM_INCLUDE_DIR}")
INCLUDE_DIRECTORIES(${GLM_INCLUDE_DIR})


# GLFW
SET(GLFW_DIR "$ENV{GLFW_DIR}")
OPTION(GLFW_BUILD_EXAMPLES "GLFW_BUILD_EXAMPLES" OFF)
OPTION(GLFW_BUILD_TESTS "GLFW_BUILD_TESTS" OFF)
OPTION(GLFW_BUILD_DOCS "GLFW_BUILD_DOCS" OFF)
IF(CMAKE_BUILD_TYPE MATCHES Release)
	ADD_SUBDIRECTORY(${GLFW_DIR} ${GLFW_DIR}/release)
ELSE()
	ADD_SUBDIRECTORY(${GLFW_DIR} ${GLFW_DIR}/debug)
ENDIF()
INCLUDE_DIRECTORIES(${GLFW_DIR}/include)
TARGET_LINK_LIBRARIES(${CMAKE_PROJECT_NAME} glfw ${GLFW_LIBRARIES})

# GLEW
SET(GLEW_DIR "$ENV{GLEW_DIR}")
INCLUDE_DIRECTORIES(${GLEW_DIR}/include)
TARGET_LINK_LIBRARIES(${CMAKE_PROJECT_NAME} ${GLEW_DIR}/lib/libGLEW.a)


# Eigen
SET(EIGEN3_INCLUDE_DIR "$ENV{EIGEN3_INCLUDE_DIR}")
INCLUDE_DIRECTORIES(${EIGEN3_INCLUDE_DIR})

# Ignore maybe-uninitialized warnings from Eigen
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wno-maybe-uninitialized)
endif()

# Enable CTest
enable_testing()

# Fetch Catch2
include(FetchContent)
FetchContent_Declare(
  catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.4.0  # pick a stable v3 tag
)
FetchContent_MakeAvailable(catch2)

# Add tests subdirectory
add_subdirectory(tests)



# Use c++17
SET_TARGET_PROPERTIES(${CMAKE_PROJECT_NAME} PROPERTIES CXX_STANDARD 17)
SET_TARGET_PROPERTIES(${CMAKE_PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)


# Set flags (-I includes )
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic")

TARGET_LINK_LIBRARIES(${CMAKE_PROJECT_NAME} "GL")