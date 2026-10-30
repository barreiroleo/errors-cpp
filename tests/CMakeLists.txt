include(FetchContent)
FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.8.1
    GIT_SHALLOW TRUE
    SYSTEM
)
FetchContent_MakeAvailable(Catch2)

add_executable(errors_test domain_test.cpp)
target_link_libraries(errors_test PRIVATE errors::errors Catch2::Catch2WithMain)
target_compile_options(errors_test PRIVATE -Wall -Wextra -Wpedantic -Werror)

include(Catch)
catch_discover_tests(errors_test)

# Builds compile_fail/<name>.cpp and passes if the compiler output matches expected_error.
# Matching the diagnostic ensures the build fails for the intended reason.
function(add_compile_fail_test name expected_error)
    set(target compile_fail_${name})
    add_library(${target} OBJECT EXCLUDE_FROM_ALL compile_fail/${name}.cpp)
    target_link_libraries(${target} PRIVATE errors::errors)
    add_test(NAME ${target}
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target ${target} --config $<CONFIG>
    )
    set_tests_properties(${target} PROPERTIES PASS_REGULAR_EXPRESSION "${expected_error}")
endfunction()

add_compile_fail_test(duplicate_code error_code_must_be_unique)
add_compile_fail_test(zero_code error_code_must_not_be_zero)
