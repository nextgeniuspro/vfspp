cmake_minimum_required(VERSION 3.16)

project(vfsppexample)

add_executable(vfsppexample example.cpp)

add_definitions(-DVFSPP_MT_SUPPORT_ENABLED) # Enable multi-threading support
# add_definitions(-DVFSPP_DISABLE_STD_FILESYSTEM) # Disable std::filesystem support

target_link_libraries(vfsppexample PRIVATE vfspp::vfspp)
target_compile_features(vfsppexample PRIVATE cxx_std_20)

# Copy test-data folder next to the produced binary
set(VFSPP_EXAMPLE_TESTDATA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test-data)
add_custom_command(TARGET vfsppexample POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Copying test-data to $<TARGET_FILE_DIR:vfsppexample>/test-data"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:vfsppexample>/test-data"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${VFSPP_EXAMPLE_TESTDATA_DIR}" "$<TARGET_FILE_DIR:vfsppexample>/test-data"
    VERBATIM
)

# Ensure Visual Studio debugger runs from the executable directory so relative 'test-data/...'
if (MSVC)
    set_property(TARGET vfsppexample PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:vfsppexample>")
endif()