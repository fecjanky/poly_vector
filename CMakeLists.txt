cmake_minimum_required(VERSION 3.1.3)

enable_testing()

project("poly_vector" VERSION 0.1 LANGUAGES CXX)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)

include(config)
include(catch)

add_library(PolyVector INTERFACE)

set(POLY_VECTOR_CMAKE_LIB_DIR lib/cmake/PolyVector)
set(POLY_VECTOR_CMAKE_INSTALL_INCLUDE_DIR include/poly_vector)
set(POLY_VECTOR_CMAKE_INSTALL_SHARE_DIR share/poly_vector)

if(MSVC)
target_compile_definitions(PolyVector INTERFACE POLY_VECTOR_MSVC_WORKAROUND)
endif()

target_include_directories(PolyVector
  INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${POLY_VECTOR_CMAKE_INSTALL_INCLUDE_DIR}>
)

if(MSVC)
target_sources(PolyVector  
	INTERFACE 
		$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/visualizer/poly_vector.natvis> 
		$<INSTALL_INTERFACE:${POLY_VECTOR_CMAKE_INSTALL_SHARE_DIR}/poly_vector.natvis>
)
endif()


include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/PolyVectorConfigVersion.cmake"
    VERSION 0.1
    COMPATIBILITY AnyNewerVersion
)

install(TARGETS PolyVector
    EXPORT PolyVectorTargets
    LIBRARY DESTINATION lib COMPONENT Runtime
    ARCHIVE DESTINATION lib COMPONENT Development
    RUNTIME DESTINATION bin COMPONENT Runtime
    PUBLIC_HEADER DESTINATION include COMPONENT Development
    BUNDLE DESTINATION bin COMPONENT Runtime
)

install(EXPORT PolyVectorTargets DESTINATION ${POLY_VECTOR_CMAKE_LIB_DIR})

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/PolyVectorConfig.cmake.in"
    "${PROJECT_BINARY_DIR}/PolyVectorConfig.cmake"
    INSTALL_DESTINATION ${POLY_VECTOR_CMAKE_LIB_DIR}
)


install(FILES "${PROJECT_BINARY_DIR}/PolyVectorConfigVersion.cmake"
              "${PROJECT_BINARY_DIR}/PolyVectorConfig.cmake"
        DESTINATION ${POLY_VECTOR_CMAKE_LIB_DIR})
		
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ DESTINATION ${POLY_VECTOR_CMAKE_INSTALL_INCLUDE_DIR})
install(FILES  visualizer/poly_vector.natvis DESTINATION ${POLY_VECTOR_CMAKE_INSTALL_SHARE_DIR})


get_filename_component(POLY_VECTOR_NATVIS_FILE visualizer/poly_vector.natvis ABSOLUTE)
get_filename_component(POLY_VECTOR_HEADER_FILE include/poly_vector.h ABSOLUTE)

add_subdirectory(test)
add_subdirectory(benchmark)
