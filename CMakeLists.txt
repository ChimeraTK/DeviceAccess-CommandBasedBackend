cmake_minimum_required(VERSION 3.16.3)
project(ChimeraTK-DeviceAccess-CommandBasedBackend)

option(BUILD_TESTS "Build tests." ON)

set(${PROJECT_NAME}_MAJOR_VERSION 00)
set(${PROJECT_NAME}_MINOR_VERSION 01)
set(${PROJECT_NAME}_PATCH_VERSION 00)
include(cmake/set_version_numbers.cmake)

include(cmake/set_default_build_to_release.cmake)
include(cmake/set_default_flags.cmake)
include(cmake/enable_code_coverage_report.cmake)
include(cmake/enable_code_style_check.cmake)

find_package(ChimeraTK-DeviceAccess 03.15 REQUIRED)
find_package(nlohmann_json 03.07.0 REQUIRED) 
find_package(inja 03.04.0 REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/include)
file(GLOB_RECURSE headers "${CMAKE_SOURCE_DIR}/include/*")

aux_source_directory(${CMAKE_SOURCE_DIR}/src library_sources)

add_library(${PROJECT_NAME} SHARED ${library_sources} ${headers})
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${${PROJECT_NAME}_FULL_LIBRARY_VERSION} SOVERSION ${${PROJECT_NAME}_SOVERSION})
target_link_libraries(${PROJECT_NAME} PUBLIC ChimeraTK::ChimeraTK-DeviceAccess pantor::inja)

# Specify include directories for the target -- Trial
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)

if(BUILD_TESTS)
  add_subdirectory(tests)
endif(BUILD_TESTS)

# Note: Backends do not need to install their header files, so just install the library.
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}Targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

# we support our cmake EXPORTS as imported targets
set(PROVIDES_EXPORTED_TARGETS 1)
include(${CMAKE_SOURCE_DIR}/cmake/create_cmake_config_files.cmake)
