cmake_minimum_required(VERSION 3.10)

project(vfsppexample)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/.. vfspp_build)

add_executable(vfsppexample example.cpp)

add_compile_definitions(VFSPP_ENABLE_MULTITHREADING)

target_link_libraries(vfsppexample PRIVATE vfspp)
target_compile_features(vfsppexample PRIVATE cxx_std_17)