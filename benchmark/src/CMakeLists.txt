cmake_minimum_required(VERSION 3.1.3)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)
include(config)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CXX_STANDARD 14)

project(poly_vector_benchmark_exe)

file(GLOB poly_vector_benchmark_source_files  *.cpp)
file(GLOB poly_vector_benchmark_header_files  ../include/*.h)
file(GLOB poly_vector_benchmark_header_files  ../../include/*.h)

set(HEADERS
        ${poly_vector_benchmark_header_files}
        ${poly_vector_benchmark_files}
)

add_executable(poly_vector_benchmark  ${poly_vector_benchmark_source_files} ${HEADERS})

target_include_directories(poly_vector_benchmark PUBLIC ../../include/)
target_include_directories(poly_vector_benchmark PUBLIC ../include/)

set_property(TARGET poly_vector_benchmark PROPERTY CXX_STANDARD 14)

