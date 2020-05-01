cmake_minimum_required(VERSION 3.1...3.14)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

add_library(common
  tee_output.cpp
  tee_output.h
  )
target_link_libraries(common PRIVATE project_warnings project_options)

# add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/testreport.txt"
#   COMMAND ./fwdaemon --run-unit-tests --use-colour no --tee-output "${CMAKE_CURRENT_BINARY_DIR}/testreport.txt"
#   DEPENDS fwdaemon)

# add_custom_target(test ALL
#   DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/testreport.txt"
#   )
