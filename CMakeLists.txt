CMAKE_MINIMUM_REQUIRED(VERSION 3.5.1)

PROJECT(ChimeraTK-CommandBasedHardwareBackend)

## Setting up library version.
SET(${PROJECT_NAME}_MAJOR_VERSION 00)
SET(${PROJECT_NAME}_MINOR_VERSION 00)
SET(${PROJECT_NAME}_PATCH_VERSION 00)
include(${CMAKE_SOURCE_DIR}/cmake/set_version_numbers.cmake)

## Loading custom defined CMake modules, like FindDOOCS.cmake.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)

## If "-DCMAKE_BUILD_TYPE=Debug" is not defined, 'Release' will be forced.
if(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "None")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
      "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
endif(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "None")

## Additional flags set when in Debug mode, to remove code optimization and add coverage report.
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 --coverage")

## Loading `add_dependency` function for setup convenience.
include(${CMAKE_SOURCE_DIR}/cmake/add_dependency.cmake)

## Dependencies loaded.
add_dependency(Boost COMPONENTS system REQUIRED)
add_dependency(ChimeraTK-DeviceAccess 01.08 REQUIRED)

## Don't know if this is really important, but specify detailed errors for the compiler
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpointer-arith -Wcast-align -Wwrite-strings -Wswitch-default")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wunreachable-code -Winit-self -Wmissing-field-initializers -Wno-unknown-pragmas")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wundef")

include_directories(${CMAKE_SOURCE_DIR}/include)
aux_source_directory(${CMAKE_SOURCE_DIR}/src lib_sources)
add_library(${PROJECT_NAME} SHARED ${lib_sources})

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES VERSION ${${PROJECT_NAME}_FULL_LIBRARY_VERSION} SOVERSION ${${PROJECT_NAME}_SOVERSION})
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} ${ChimeraTK-DeviceAccess_LIBRARIES})

## We are enabling testing. Full setup is done in CMakeTests.txt in 'tests/' directory.
enable_testing()  
add_subdirectory ("${PROJECT_SOURCE_DIR}/tests")
# Copy server dummys for testing
FILE(COPY ${CMAKE_SOURCE_DIR}/DummyServer/echo-server.py
     DESTINATION ${PROJECT_BINARY_DIR}/tests)

#include(${CMAKE_SOURCE_DIR}/cmake/enable_code_coverage_report.cmake)

SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

#include(${CMAKE_SOURCE_DIR}/cmake/enable_doxygen_documentation.cmake)
