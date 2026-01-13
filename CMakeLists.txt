cmake_minimum_required(VERSION 3.16)

project(mem_alloc C)

# Export compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Use modern C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Be strict – warnings are your friend
add_compile_options(-Wall -Wextra -Wpedantic)

# Collect sources
file(GLOB SRC_FILES
    src/*.c
)

add_executable(mem_alloc ${SRC_FILES})

target_include_directories(mem_alloc
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

