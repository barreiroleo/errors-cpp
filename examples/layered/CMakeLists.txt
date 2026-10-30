add_executable(layered_example main.cpp inventory.cpp storage.cpp)
target_link_libraries(layered_example PRIVATE errors::errors)
target_compile_options(layered_example PRIVATE -Wall -Wextra -Wpedantic -Werror)
