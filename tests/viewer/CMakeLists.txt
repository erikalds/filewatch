cmake_minimum_required(VERSION 3.1...3.14)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

find_package(Python3 REQUIRED)

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/viewer_report.txt"
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/run_tests.py" "$<TARGET_FILE:fwdaemon>" "$<TARGET_FILE:filewatcher>" --tee-output "${CMAKE_CURRENT_BINARY_DIR}/viewer_report.txt" "${CMAKE_CURRENT_SOURCE_DIR}/test_*.py"
  DEPENDS fwdaemon filewatcher npm-build-target)

add_custom_target(systest_filewatcher ALL
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/viewer_report.txt"
  )
