cmake_minimum_required(VERSION 2.6)
project(ALIEN_FX)

set(
  CMAKE_RUNTIME_OUTPUT_DIRECTORY
  ${CMAKE_BINARY_DIR}/bin
)

add_definitions(
  -W
  -pedantic
  -Wall
  -lusb-1.0
)

set(CMAKE_CXX_FLAGS "-g -Wall -lusb-1.0")

find_package(PkgConfig)

add_executable(
  controller
  controller.c

  vars.h
  functions.h
)

target_link_libraries(controller usb-1.0)
