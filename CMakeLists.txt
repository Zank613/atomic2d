cmake_minimum_required(VERSION 3.24)

project(atomic2d VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

set(RAYLIB_VERSION 5.5)

FetchContent_Declare(
        raylib
        URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP OFF
)

FetchContent_MakeAvailable(raylib)

add_executable(atomic2d
        main.cpp
)

configure_file(elements.txt elements.txt COPYONLY)
configure_file(atomic_pic.png atomic_pic.png COPYONLY)

target_link_libraries(atomic2d PRIVATE raylib)

if (WIN32)
    target_link_libraries(atomic2d PRIVATE winmm)
endif()