project(test_poly_vec)

find_package(PolyVector CONFIG REQUIRED)

add_executable(example1 example1.cpp)

target_link_libraries(example1 PolyVector)