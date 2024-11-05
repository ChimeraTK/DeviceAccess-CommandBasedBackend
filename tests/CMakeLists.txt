find_package(Boost COMPONENTS unit_test_framework REQUIRED)
find_program(SOCAT socat REQUIRED)
if(NOT SOCAT)
        message(FATAL_ERROR "socat not found!")
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/DummyServerLib)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/DummyServerLib dummy_lib_sources)
add_library(DummyServerLib ${dummy_lib_sources})
target_link_libraries(DummyServerLib PUBLIC ${PROJECT_NAME} )

add_executable(dummy-server manual_tests/DummyServer-stand-alone.cpp)
target_link_libraries(dummy-server PUBLIC DummyServerLib )

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} testExecutables)
foreach( testExecutableSrcFile ${testExecutables})
  #NAME_WE means the base name without path and (longest) extension
  get_filename_component(executableName ${testExecutableSrcFile} NAME_WE)
  add_executable(${executableName} ${testExecutableSrcFile})
  target_link_libraries(${executableName} PRIVATE ${PROJECT_NAME} ${Boost_LIBRARIES} DummyServerLib)
  add_test(${executableName} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${executableName})
endforeach(testExecutableSrcFile)

file(COPY manual_tests/devices.dmap test.json DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

