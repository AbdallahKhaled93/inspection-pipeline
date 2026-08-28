cmake_minimum_required(VERSION 3.20)

project(inspection_pipeline VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(inspection_warnings INTERFACE)
target_compile_options(inspection_warnings INTERFACE
    -Wall -Wextra -Wpedantic -Wconversion -Wshadow)

add_library(inspection_core
    src/synthetic_source.cpp
    src/frame_queue.cpp
    src/producer.cpp
)

target_include_directories(inspection_core PUBLIC include)
target_compile_features(inspection_core PUBLIC cxx_std_20)
target_link_libraries(inspection_core PRIVATE inspection_warnings)

add_executable(inspection_demo apps/main.cpp)
target_link_libraries(inspection_demo PRIVATE inspection_core inspection_warnings)