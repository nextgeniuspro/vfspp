cmake_minimum_required(VERSION 3.16)

# Project name and version
project(vfspp VERSION 1.0 LANGUAGES CXX)
option(BUILD_EXAMPLES "Build examples" OFF)
# Add the miniz-cpp library
add_subdirectory(vendor/miniz-cpp EXCLUDE_FROM_ALL)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add the include directory as a target
add_library(vfspp INTERFACE)
add_library(vfspp::vfspp ALIAS vfspp)

# Suppress warnings originating from bundled miniz-cpp headers
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(vfspp INTERFACE -Wno-misleading-indentation -Wno-deprecated-declarations)
endif()

target_include_directories(vfspp
    INTERFACE 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vendor/miniz-cpp>
)

target_compile_features(vfspp INTERFACE cxx_std_20)

if (BUILD_EXAMPLES)
add_subdirectory(examples)
endif()
