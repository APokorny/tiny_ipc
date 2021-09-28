cmake_policy(SET CMP0057 NEW)
cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0077 NEW)
cmake_minimum_required(VERSION 3.14)
project(tiny_ipc VERSION 0.1.0 LANGUAGES CXX)

include(cmake/CPM.cmake)
CPMAddPackage(NAME kvasir-mpl
  GITHUB_REPOSITORY "kvasir-io/mpl"
  GIT_TAG development)
CPMAddPackage("gh:TheLartians/PackageProject.cmake@1.6.0")
CPMAddPackage(NAME tiny_tuple
  GITHUB_REPOSITORY "APokorny/tiny_tuple"
  GIT_TAG development)
find_package(Boost REQUIRED) # Boost Asio i needed
find_package(Threads)

add_library(tiny_ipc INTERFACE)
add_library(tiny_ipc::tiny_ipc ALIAS tiny_ipc)
target_compile_features(tiny_ipc INTERFACE cxx_std_20)
target_link_libraries(tiny_ipc INTERFACE kvasir_mpl tiny_tuple::tiny_tuple Boost::boost)
target_include_directories(tiny_ipc INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  )

option(TINY_IPC_BUILD_EXAMPLE "enable examples" OFF)

if(TINY_IPC_BUILD_EXAMPLE)
  add_executable(client example/client.cpp)
  add_executable(tiny_ipc::client ALIAS client)
  target_link_libraries(client PUBLIC tiny_ipc Threads::Threads)
  add_executable(server example/server.cpp)
  add_executable(tiny_ipc::server ALIAS server)
  target_link_libraries(server PUBLIC tiny_ipc Threads::Threads)
endif(TINY_IPC_BUILD_EXAMPLE)

packageProject(
  NAME ${PROJECT_NAME}
  VERSION ${PROJECT_VERSION}
  BINARY_DIR ${PROJECT_BINARY_DIR}
  INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
  INCLUDE_DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}
  DEPENDENCIES "kvasir-mpl 1.0.0;tiny_tuple 0.1.0;Boost 1.65"
  VERSION_HEADER "${PROJECT_NAME}/version.h"
  EXPORT_HEADER "${PROJECT_NAME}/export.h"
  NAMESPACE tiny_tuple
  COMPATIBILITY AnyNewerVersion
  DISABLE_VERSION_SUFFIX YES
  ARCH_INDEPENDENT YES
  )
