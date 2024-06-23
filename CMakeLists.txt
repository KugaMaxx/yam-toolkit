cmake_minimum_required(VERSION 3.22)

# Project name and version.
project(
    dv-toolkit
    VERSION 0.1.1
    LANGUAGES C CXX)

# List of available options
option(TOOLKIT_ENABLE_SAMPLES "Build samples" OFF)
option(TOOLKIT_ENABLE_PYTHON "Build python bindings" OFF)

# Print basic options info
message(STATUS "TOOLKIT_ENABLE_SAMPLES ${TOOLKIT_ENABLE_SAMPLES}")
message(STATUS "TOOLKIT_ENABLE_PYTHON ${TOOLKIT_ENABLE_PYTHON}")

# C++ standard settings.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find compression support
INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(liblz4 IMPORTED_TARGET liblz4)
PKG_CHECK_MODULES(libzstd IMPORTED_TARGET libzstd)

# Find dv-processing support.
find_package(dv-processing REQUIRED)

# Install header files.
add_subdirectory(include)

# Install samples.
if(TOOLKIT_ENABLE_SAMPLES)
    add_subdirectory(samples)
endif()

# Install python package.
if(TOOLKIT_ENABLE_PYTHON)
    add_subdirectory(python)
endif()
