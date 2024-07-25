cmake_minimum_required(VERSION 3.10)

# Project name and version
project(vfspp VERSION 1.0 LANGUAGES CXX)

# Add the miniz-cpp library
add_subdirectory(vendor/miniz-cpp EXCLUDE_FROM_ALL)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add the include directory as a target
add_library(vfspp INTERFACE)

target_include_directories(vfspp
    INTERFACE 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vendor/miniz-cpp>
)

target_compile_features(vfspp INTERFACE cxx_std_17)