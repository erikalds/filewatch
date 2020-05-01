cmake_minimum_required(VERSION 3.1...3.14)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

add_executable(filewatcher
  main.cpp
  unittest/testsomething.cpp
  )
target_include_directories(filewatcher PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}/../rpc_defs/rpcgen
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  )
target_link_libraries(filewatcher PRIVATE project_warnings project_options
  filewatch_rpc
  common
  CONAN_PKG::catch2
  )

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/testreport.txt"
  COMMAND "$<TARGET_FILE:filewatcher>" --run-unit-tests --use-colour no --tee-output "${CMAKE_CURRENT_BINARY_DIR}/testreport.txt"
  DEPENDS filewatcher)

add_custom_target(test_filewatcher ALL
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/testreport.txt"
  )
