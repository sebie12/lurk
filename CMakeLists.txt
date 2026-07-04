cmake_minimum_required(VERSION 3.15)
project(lurk CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(raylib REQUIRED)

add_executable(game
    src/main.cpp
    src/Game.cpp
)
target_link_libraries(game raylib)
