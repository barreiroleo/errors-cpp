cmake_minimum_required(VERSION 3.23)
project(errors LANGUAGES CXX)

if(PROJECT_IS_TOP_LEVEL)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

add_library(errors INTERFACE)
add_library(errors::errors ALIAS errors)

target_compile_features(errors INTERFACE cxx_std_23)
target_include_directories(errors INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)

option(ERRORS_BUILD_EXAMPLES "Build the error-handling examples" ${PROJECT_IS_TOP_LEVEL})
option(ERRORS_BUILD_TESTS "Build the error-handling tests" ${PROJECT_IS_TOP_LEVEL})

if(ERRORS_BUILD_EXAMPLES)
    # add_subdirectory(examples)
endif()

if(ERRORS_BUILD_TESTS)
    enable_testing()
    # add_executable(errors_test)
    # target_link_libraries(errors_test PRIVATE errors::errors)
    # add_test(NAME errors_test COMMAND errors_test)
endif()
