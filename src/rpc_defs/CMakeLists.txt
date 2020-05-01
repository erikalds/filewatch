cmake_minimum_required(VERSION 3.1...3.14)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

find_package(Protobuf REQUIRED)
find_package(GRPC REQUIRED)

set(PROTOS
  ${CMAKE_CURRENT_SOURCE_DIR}/filewatch.proto
  )

set(PROTO_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/rpcgen)
file(MAKE_DIRECTORY ${PROTO_SRC_DIR})
include_directories(${PROTO_SRC_DIR})

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_SRC_DIR} ${PROTOS})
grpc_generate_cpp(GRPC_SRCS GRPC_HDRS ${PROTO_SRC_DIR} ${PROTOS})

add_library(filewatch_rpc ${PROTO_SRCS} ${GRPC_SRCS})
target_link_libraries(filewatch_rpc PRIVATE project_warnings project_options
  gRPC::grpc++_reflection
  protobuf::libprotobuf
  )
