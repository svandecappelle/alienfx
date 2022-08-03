cmake_minimum_required(VERSION 2.6)
project(alienFx)

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

find_program(GLIB_COMPILE_RESOURCES NAMES glib-compile-resources REQUIRED)
set(GRESOURCE_CSS src/resources/css/gtk.css)
add_custom_target(
    alienFxUI-resource
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${GRESOURCE_CSS}
)

add_executable(
    controller
    src/controller.c

    src/lib/utils/vars.h
    src/lib/utils/functions.h
    src/lib/utils/functions.c
)

configure_file(${GRESOURCE_CSS} ${GRESOURCE_CSS} @ONLY)

add_executable(
    ${PROJECT_NAME}
    src/controller-ui.c

    ${CMAKE_CURRENT_BINARY_DIR}/../${GRESOURCE_CSS}

    src/layout/color_selector.h
    src/layout/color_selector.c

    src/lib/utils/vars.h
    src/lib/utils/functions.h
    src/lib/utils/functions.c
    src/lib/keyboard.h
    src/lib/keyboard.c

    src/lib/key.h
    src/lib/key.c
    src/lib/touchpad.h
    src/lib/touchpad.c
    src/lib/mediabar.h
    src/lib/mediabar.c
    src/lib/speakers.h
    src/lib/speakers.c
)

# Step 5:
# add_dependencies(${PROJECT_NAME} alienFxUI-resource)

target_link_libraries(controller usb-1.0)
target_link_libraries(${PROJECT_NAME} PRIVATE usb-1.0 ${GTK4_LIBRARIES})
target_include_directories(${PROJECT_NAME} PRIVATE ${GTK4_INCLUDE_DIRS})
