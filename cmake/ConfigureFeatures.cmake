#
cmake_minimum_required(VERSION 3.1.3)

try_compile(HAS_CXX_DISJUNCTION ${CMAKE_BINARY_DIR} ${PROJECT_SOURCE_DIR}/cmake/has_disjunction.cpp)
message("HAS_CXX_DISJUNCTION is ${HAS_CXX_DISJUNCTION}")
#
