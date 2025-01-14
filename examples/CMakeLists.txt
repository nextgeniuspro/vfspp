cmake_minimum_required(VERSION 3.10)

project(vfsppexample)

#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/.. vfspp_build)

add_executable(vfsppexample example.cpp)

add_compile_definitions(VFSPP_ENABLE_MULTITHREADING)

target_link_libraries(vfsppexample PRIVATE vfspp::vfspp)
target_compile_features(vfsppexample PRIVATE cxx_std_17)

# Specify the output directory
set(COPY_DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/test-data)

# Add a post-build step to copy the whole directory
add_custom_command(TARGET vfsppexample POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/test-data
    ${COPY_DESTINATION}
)