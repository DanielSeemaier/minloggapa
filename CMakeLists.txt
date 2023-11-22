cmake_minimum_required(VERSION 3.7)
project(implementation CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/KaHIP/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/KaHIP/lib/tools
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/KaHIP/lib/partition
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/sdsl-lite-built/include
)

link_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/sdsl-lite-built/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/kahip-built/lib
)

set(CORE_LIB implementation-core)
set(LIBRARIES libkahip.a sqlite3)

add_subdirectory(lib)
add_subdirectory(tools)
add_subdirectory(experiments)
