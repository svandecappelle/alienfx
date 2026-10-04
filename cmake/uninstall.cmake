# Removes the files listed in the install manifest written by "make install"
if(NOT EXISTS "${MANIFEST}")
    message(FATAL_ERROR "Nothing to uninstall: ${MANIFEST} not found, run make install first")
endif()

file(STRINGS "${MANIFEST}" installed_files)
foreach(installed_file ${installed_files})
    set(path "$ENV{DESTDIR}${installed_file}")
    if(EXISTS "${path}" OR IS_SYMLINK "${path}")
        message(STATUS "Removing ${path}")
        file(REMOVE "${path}")
    endif()
endforeach()
