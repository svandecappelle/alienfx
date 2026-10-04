cmake_minimum_required(VERSION 3.10)
project(alienFx VERSION 1.0.0 LANGUAGES C)

include(GNUInstallDirs)

# Application id, shared by the GtkApplication, the desktop entry and the icon
set(APP_ID org.alienfx.Controller)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

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
    src/lib/utils/color.h
    src/lib/utils/color.c

    src/lib/profile.h
    src/lib/profile.c
)

add_executable(
    ${PROJECT_NAME}
    src/controller-ui.c
    ${GRESOURCE_C}

    src/lib/utils/vars.h
    src/lib/utils/functions.h
    src/lib/utils/functions.c

    src/lib/utils/color.h
    src/lib/utils/color.c

    src/lib/profile.h
    src/lib/profile.c
    src/lib/power.h
    src/lib/power.c
    src/lib/zones.h
    src/lib/zones.c
    src/lib/laptop-view.h
    src/lib/laptop-view.c
)

target_link_libraries(controller usb-1.0)
target_link_libraries(${PROJECT_NAME} PRIVATE usb-1.0 m ${GTK4_LIBRARIES})
# Names of the installed programs
set_target_properties(controller PROPERTIES OUTPUT_NAME alienfx-cli)
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME alienfx)

# SYSTEM: warnings inside the GTK headers are not ours (they trip -pedantic)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${GTK4_INCLUDE_DIRS})
# The generated resource file embeds the CSS as one long string literal
set_source_files_properties(${GRESOURCE_C} PROPERTIES COMPILE_OPTIONS -Wno-overlength-strings)

# ---------------------------------------------------------------------------
# Install: programs, desktop entry, icon, AppStream metadata and udev rule
# ---------------------------------------------------------------------------

# udev only reads rules from its own directories, whatever the install prefix
pkg_get_variable(UDEV_DIR udev udevdir)
if(NOT UDEV_DIR)
    set(UDEV_DIR /usr/lib/udev)
endif()
set(UDEV_RULES_DIR "${UDEV_DIR}/rules.d" CACHE PATH "Directory of the installed udev rule")

install(TARGETS controller ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES data/${APP_ID}.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES data/${APP_ID}.metainfo.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
install(FILES data/icons/${APP_ID}.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
install(FILES udev/70-alienfx.rules DESTINATION ${UDEV_RULES_DIR})

# alienfx-desktop-icon: puts a launcher on the user's desktop (run as the user)
set(DESKTOP_FILE ${CMAKE_INSTALL_FULL_DATADIR}/applications/${APP_ID}.desktop)
configure_file(data/alienfx-desktop-icon.in ${CMAKE_CURRENT_BINARY_DIR}/alienfx-desktop-icon @ONLY)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/alienfx-desktop-icon DESTINATION ${CMAKE_INSTALL_BINDIR})

# Refresh the system caches, unless staging into a DESTDIR (packaging)
install(CODE "
if(\"\$ENV{DESTDIR}\" STREQUAL \"\")
    message(STATUS \"Refreshing the icon cache, the desktop database and the udev rules\")
    execute_process(COMMAND gtk-update-icon-cache -q -t -f \"${CMAKE_INSTALL_FULL_DATADIR}/icons/hicolor\" ERROR_QUIET)
    execute_process(COMMAND update-desktop-database -q \"${CMAKE_INSTALL_FULL_DATADIR}/applications\" ERROR_QUIET)
    execute_process(COMMAND udevadm control --reload-rules ERROR_QUIET)
    execute_process(COMMAND udevadm trigger --subsystem-match=usb --attr-match=idVendor=187c ERROR_QUIET)
endif()
")

add_custom_target(
    uninstall
    COMMAND ${CMAKE_COMMAND} -DMANIFEST=${CMAKE_BINARY_DIR}/install_manifest.txt -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/uninstall.cmake
    COMMENT "Uninstalling"
)

add_custom_target(
    desktop-icon
    COMMAND sh ${CMAKE_CURRENT_BINARY_DIR}/alienfx-desktop-icon
    COMMENT "Creating the AlienFX icon on the desktop"
)
