cmake_minimum_required(VERSION 3.1.3)


project(poly_vector_benchmark_exe)

set(poly_vector_benchmark_source_files  src/main.cpp)


add_executable(poly_vector_benchmark  ${poly_vector_benchmark_source_files} )

target_link_libraries(poly_vector_benchmark PolyVector)


