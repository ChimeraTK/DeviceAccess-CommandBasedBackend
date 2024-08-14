find_package(Boost COMPONENTS unit_test_framework REQUIRED)

add_executable(dummy-server dummy-server.cpp)
target_link_libraries(dummy-server PUBLIC ${PROJECT_NAME} )

add_executable(test_serialPort test_serialPort.cpp)
target_link_libraries(test_serialPort PUBLIC ${PROJECT_NAME} )

add_executable(testCommandBasedBackendUnified testCommandBasedBackendUnified.cpp)
target_link_libraries(testCommandBasedBackendUnified  PRIVATE ${Boost_LIBRARIES} ${PROJECT_NAME} )
add_test(testCommandBasedBackendUnified testCommandBasedBackendUnified)

file(COPY mock.map mock.dmap DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
#file(COPY devMapFile.dmap DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
