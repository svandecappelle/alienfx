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

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)

add_executable(
  controller
  controller.c

  lib/utils/vars.h
  lib/utils/functions.h
)

add_executable(
  controller-ui
  controller-ui.c

  lib/utils/vars.h
  lib/utils/functions.h
  lib/keyboard.h
  lib/keyboard.c
  lib/key.h
  lib/key.c
)


target_link_libraries(controller usb-1.0)
target_link_libraries(controller-ui PRIVATE usb-1.0 ${GTK4_LIBRARIES})
target_include_directories(controller-ui PRIVATE ${GTK4_INCLUDE_DIRS})
