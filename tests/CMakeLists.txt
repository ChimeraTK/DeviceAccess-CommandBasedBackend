FIND_PACKAGE(Boost COMPONENTS system unit_test_framework)
include_directories(${Boost_INCLUDE_DIRS})

enable_testing()  

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src testExecutables)

foreach(testExecutableSrcFile ${testExecutables})
  #NAME_WE means the base name without path and (longest) extension
  get_filename_component(executableName ${testExecutableSrcFile} NAME_WE)
  add_executable(${executableName} ${testExecutableSrcFile})
  target_link_libraries(${executableName} ${ChimeraTK-DeviceAccess_LIBRARIES})
  add_test(${executableName} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${executableName})
endforeach(testExecutableSrcFile)

#FIXME: copy dmap-file template and test bash script
FILE(COPY ${CMAKE_SOURCE_DIR}/tests/devices.dmap.template
     ${CMAKE_SOURCE_DIR}/tests/configureDMap.sh
     DESTINATION ${PROJECT_BINARY_DIR}/tests)
