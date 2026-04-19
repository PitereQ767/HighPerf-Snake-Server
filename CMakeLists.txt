cmake_minimum_required(VERSION 3.20)

project(HighPerfSnakeServer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wshadow)

add_subdirectory(shared)
add_subdirectory(server)

option(BUILD_CLIENT "Build the graphical client" ON)
if (BUILD_CLIENT)
    add_subdirectory(client)
endif ()