cmake_minimum_required(VERSION 3.10)

project(vfsppexample)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/.. vfspp_build)

add_executable(vfsppexample example.cpp)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
endif(MSVC)

target_link_libraries(vfsppexample PRIVATE vfspp)
