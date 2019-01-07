cmake_minimum_required(VERSION 3.1.3)

enable_testing()

project("poly_vector" VERSION 0.1 LANGUAGES CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/CMakeModules)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)

include(config)
include(catch)
include(ConfigureFeatures)


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
	$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/gen>
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
		

install(FILES 
 ${PROJECT_SOURCE_DIR}/include/poly/vector.h 
 DESTINATION ${POLY_VECTOR_CMAKE_INSTALL_INCLUDE_DIR})

install(FILES 
 ${PROJECT_SOURCE_DIR}/include/poly/detail/vector_impl.h 
 ${CMAKE_BINARY_DIR}/gen/poly/detail/vector_config.h  
 DESTINATION ${POLY_VECTOR_CMAKE_INSTALL_INCLUDE_DIR}/detail)

install(FILES  visualizer/poly_vector.natvis DESTINATION ${POLY_VECTOR_CMAKE_INSTALL_SHARE_DIR})


get_filename_component(POLY_VECTOR_NATVIS_FILE visualizer/poly_vector.natvis ABSOLUTE)
get_filename_component(POLY_VECTOR_HEADER_FILE include/poly/vector.h ABSOLUTE)
get_filename_component(POLY_VECTOR_IMPL_HEADER_FILE include/poly/detail/vector_impl.h ABSOLUTE)
set(POLY_VECTOR_HEADER_FILES ${POLY_VECTOR_HEADER_FILE}  ${POLY_VECTOR_IMPL_HEADER_FILE})

add_subdirectory(test)
add_subdirectory(benchmark)
