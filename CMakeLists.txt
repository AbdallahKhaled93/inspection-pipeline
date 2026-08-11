cmake_minimum_required(VERSION 3.20)

project(inspection_pipeline VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_executable(inspection_demo apps/main.cpp)
target_include_directories(inspection_demo PRIVATE include)
target_compile_features(inspection_demo PRIVATE cxx_std_20)
target_compile_options(inspection_demo PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)