cmake_minimum_required(VERSION 3.1...3.14)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

find_package(Python3 REQUIRED)

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/viewer_report.txt"
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/run_tests.py" "$<TARGET_FILE:fwdaemon>" "$<TARGET_FILE:filewatcher>" --tee-output "${CMAKE_CURRENT_BINARY_DIR}/viewer_report.txt" "${CMAKE_CURRENT_SOURCE_DIR}/test_*.py"
  DEPENDS
  fwdaemon
  filewatcher
  npm-build-target
  ${CMAKE_CURRENT_BINARY_DIR}/../daemon/daemon_report.txt # to enforce order
  ${CMAKE_CURRENT_SOURCE_DIR}/myhttp.py
  ${CMAKE_CURRENT_SOURCE_DIR}/run_tests.py
  ${CMAKE_CURRENT_SOURCE_DIR}/systest_import.py
  ${CMAKE_CURRENT_SOURCE_DIR}/test_100_serves_index.py
  ${CMAKE_CURRENT_SOURCE_DIR}/test_200_files.py
  ${CMAKE_CURRENT_SOURCE_DIR}/../systest/common_tests.py
  ${CMAKE_CURRENT_SOURCE_DIR}/../systest/filesys.py
  ${CMAKE_CURRENT_SOURCE_DIR}/../systest/__init__.py
  ${CMAKE_CURRENT_SOURCE_DIR}/../systest/runningprocess.py
  ${CMAKE_CURRENT_SOURCE_DIR}/../systest/runtests.py
  ${CMAKE_CURRENT_SOURCE_DIR}/../systest/teeoutput.py
  ${CMAKE_CURRENT_SOURCE_DIR}/../systest/temporarydir.py
  )

add_custom_target(systest_filewatcher ALL
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/viewer_report.txt"
  )
