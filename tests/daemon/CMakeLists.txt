cmake_minimum_required(VERSION 3.1...3.14)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

find_package(Python3 REQUIRED)

add_custom_command(OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/filewatch_pb2.py" "${CMAKE_CURRENT_SOURCE_DIR}/filewatch_pb2_grpc.py"
  COMMAND "${Python3_EXECUTABLE}" -m grpc_tools.protoc "-I${CMAKE_CURRENT_SOURCE_DIR}/../../src/rpc_defs" "--python_out=${CMAKE_CURRENT_SOURCE_DIR}" "--grpc_python_out=${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/../../src/rpc_defs/filewatch.proto"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../../src/rpc_defs/filewatch.proto"
  )

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/daemon_report.txt"
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/run_tests.py" "$<TARGET_FILE:fwdaemon>" --tee-output "${CMAKE_CURRENT_BINARY_DIR}/daemon_report.txt" "${CMAKE_CURRENT_SOURCE_DIR}/test_*.py"
  DEPENDS fwdaemon "${CMAKE_CURRENT_SOURCE_DIR}/filewatch_pb2.py" "${CMAKE_CURRENT_SOURCE_DIR}/filewatch_pb2_grpc.py")

add_custom_target(systest_fwdaemon ALL
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/daemon_report.txt"
  )
