cmake_minimum_required(VERSION 3.16.0)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake
  CACHE STRING "Vcpkg toolchain file")

project(rimworldlayoutoptimizer)

add_executable(rimworldlayoutoptimizer "")
set_property(TARGET rimworldlayoutoptimizer PROPERTY CXX_STANDARD 17)
target_link_libraries(rimworldlayoutoptimizer PRIVATE pthread)

target_compile_options(rimworldlayoutoptimizer PRIVATE 
    -Wall 
    -Wextra 
    -pedantic 
    -Wno-maybe-uninitialized
    -Wduplicated-cond
    -Wduplicated-branches
    -Wlogical-op
    -Wrestrict
    -Wnull-dereference
    -Wold-style-cast
    -Wuseless-cast
    -Wformat=2
    -Wconversion
)

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_property(TARGET rimworldlayoutoptimizer PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# argh
find_package(argh CONFIG REQUIRED)
target_link_libraries(rimworldlayoutoptimizer PRIVATE argh)

# doctest
find_package(doctest CONFIG REQUIRED)
target_link_libraries(rimworldlayoutoptimizer PRIVATE doctest::doctest)

# yaml-cpp
find_package(yaml-cpp CONFIG REQUIRED)
target_link_libraries(rimworldlayoutoptimizer PRIVATE yaml-cpp)

target_include_directories(rimworldlayoutoptimizer PRIVATE src)
target_include_directories(rimworldlayoutoptimizer SYSTEM PRIVATE third_party)

add_subdirectory(src)