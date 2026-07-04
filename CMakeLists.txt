cmake_minimum_required(VERSION 3.15)
project(lurk CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(raylib REQUIRED)
find_package(EnTT REQUIRED)

add_executable(game
    src/main.cpp
    src/core/Game.cpp
    src/sim/World.cpp
    src/sim/WorldGen.cpp
    src/sim/ChunkManager.cpp
)
target_include_directories(game PRIVATE src third_party/FastNoiseLite)
target_link_libraries(game raylib EnTT::EnTT)

# --- Tests ---
enable_testing()

add_executable(coords_test tests/coords_test.cpp)
target_include_directories(coords_test PRIVATE src)
add_test(NAME coords_test COMMAND coords_test)

add_executable(worldgen_test tests/worldgen_test.cpp src/sim/WorldGen.cpp)
target_include_directories(worldgen_test PRIVATE src third_party/FastNoiseLite)
add_test(NAME worldgen_test COMMAND worldgen_test)

add_executable(chunkmanager_test
    tests/chunkmanager_test.cpp
    src/sim/ChunkManager.cpp
    src/sim/WorldGen.cpp
)
target_include_directories(chunkmanager_test PRIVATE src third_party/FastNoiseLite)
add_test(NAME chunkmanager_test COMMAND chunkmanager_test)

add_executable(collision_test tests/collision_test.cpp)
target_include_directories(collision_test PRIVATE src)
add_test(NAME collision_test COMMAND collision_test)

add_executable(movement_test tests/movement_test.cpp)
target_include_directories(movement_test PRIVATE src)
target_link_libraries(movement_test EnTT::EnTT)
add_test(NAME movement_test COMMAND movement_test)
