#find_package(Boost COMPONENTS unit_test_framework REQUIRED)

add_executable(dummy-server dummy-server.cpp)
target_link_libraries(dummy-server PUBLIC ${PROJECT_NAME} )
#add_test(testModbus testModbus)

#file(COPY dummy.map DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
#file(COPY devMapFile.dmap DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
