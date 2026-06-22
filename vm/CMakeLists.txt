cmake_minimum_required(VERSION 3.20)
project(onescript_vm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra -Wpedantic)

add_library(onescript_vm_core STATIC
    src/loader.cpp
    src/value.cpp
    src/interp.cpp
)
target_include_directories(onescript_vm_core PUBLIC include)

add_executable(osvm src/main.cpp)
target_link_libraries(osvm PRIVATE onescript_vm_core)

enable_testing()
add_subdirectory(tests)
