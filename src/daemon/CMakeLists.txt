cmake_minimum_required(VERSION 3.1...3.14)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

add_executable(fwdaemon
  defaultfilesystem.cpp
  details/inotify.cpp
  directoryeventlistener.cpp
  directoryview.cpp
  directorywatcher.cpp
  filesystem.cpp
  filesystemfactory.cpp
  filesystemwatcherfactory.cpp
  fileview.cpp
  filewatcher.cpp
  linuxfilesystem.cpp
  main.cpp
  server.cpp
  windowsfilesystem.cpp
  unittest/test_directorywatcher.cpp
  unittest/test_filesystem.cpp
  unittest/test_filesystemwatcherfactory.cpp
  unittest/test_filewatcher.cpp
  unittest/test_linux_filesystem.cpp
  defaultfilesystem.h
  details/inotify.h
  directoryeventlistener.h
  directoryview.h
  directorywatcher.h
  filesystem.h
  filesystemfactory.h
  filesystemwatcherfactory.h
  fileview.h
  filewatcher.h
  linuxfilesystem.h
  server.h
  windowsfilesystem.h
  unittest/dummyfilesystem.h
  )
target_include_directories(fwdaemon PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}/../rpc_defs/rpcgen
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  )
target_link_libraries(fwdaemon PRIVATE project_warnings project_options
  filewatch_rpc
  common
  pthread
  stdc++fs
  CONAN_PKG::catch2
  CONAN_PKG::docopt.cpp
  CONAN_PKG::spdlog
  CONAN_PKG::fmt
  )

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/testreport.txt"
  COMMAND "$<TARGET_FILE:fwdaemon>" --run-unit-tests --use-colour no --tee-output "${CMAKE_CURRENT_BINARY_DIR}/testreport.txt"
  DEPENDS fwdaemon)

add_custom_target(test_fwdaemon ALL
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/testreport.txt"
  )
