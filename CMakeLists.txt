cmake_minimum_required(VERSION 3.10)
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

# Embed the UI resources (CSS) in the binary
set(RESOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/resources)
set(GRESOURCE_XML ${RESOURCES_DIR}/alienfx.gresource.xml)
set(GRESOURCE_C ${CMAKE_CURRENT_BINARY_DIR}/alienfx-resources.c)
add_custom_command(
    OUTPUT ${GRESOURCE_C}
    COMMAND ${GLIB_COMPILE_RESOURCES}
        --target=${GRESOURCE_C}
        --sourcedir=${RESOURCES_DIR}
        --generate-source
        ${GRESOURCE_XML}
    DEPENDS ${GRESOURCE_XML} ${RESOURCES_DIR}/css/gtk.css
)

add_executable(
    controller
    src/controller.c

    src/lib/utils/vars.h
    src/lib/utils/functions.h
    src/lib/utils/functions.c
)

add_executable(
    ${PROJECT_NAME}
    src/controller-ui.c
    ${GRESOURCE_C}

    src/lib/utils/vars.h
    src/lib/utils/functions.h
    src/lib/utils/functions.c

    src/lib/zones.h
    src/lib/zones.c
    src/lib/laptop-view.h
    src/lib/laptop-view.c
)

target_link_libraries(controller usb-1.0)
target_link_libraries(${PROJECT_NAME} PRIVATE usb-1.0 m ${GTK4_LIBRARIES})
target_include_directories(${PROJECT_NAME} PRIVATE ${GTK4_INCLUDE_DIRS})
# The generated resource file embeds the CSS as one long string literal
set_source_files_properties(${GRESOURCE_C} PROPERTIES COMPILE_OPTIONS -Wno-overlength-strings)
