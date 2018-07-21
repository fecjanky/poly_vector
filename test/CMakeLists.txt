cmake_minimum_required(VERSION 3.1.3)

enable_testing()

project(poly_vector_test CXX)

set(SRC
        src/main.cpp
        src/test_poly_vector.cpp
		src/test_poly_vector_meta.cpp
)

if (MSVC)
    set(SRC include/catch_ext.hpp include/test_poly_vector.h ${POLY_VECTOR_NATVIS_FILE} ${POLY_VECTOR_HEADER_FILE} ${SRC})
endif (MSVC)

add_executable(poly_vector_test  ${SRC})

target_link_libraries(poly_vector_test PolyVector Catch_lib)
set_target_properties(poly_vector_test PROPERTIES LINKER_LANGUAGE CXX)
set_property(TARGET poly_vector_test PROPERTY CXX_STANDARD 14)

target_include_directories(poly_vector_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_test(NAME test_poly_vector COMMAND poly_vector_test WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
