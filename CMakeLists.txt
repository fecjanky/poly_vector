cmake_minimum_required(VERSION 3.1.3)

enable_testing()

project("poly_vector")

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)

include(config)
include(catch)

add_library(poly_vector_lib INTERFACE)

target_include_directories(poly_vector_lib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(test)
add_subdirectory(benchmark)
