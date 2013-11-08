# This module tries to find libWebsockets library and include files
#
# LIBWEBSOCKETS_FOUND, If false, do not try to use libWebSockets
# LIBWEBSOCKETS_INCLUDE_DIR, path where to find libwebsockets.h
# LIBWEBSOCKETS_LIBRARY_DIR, path where to find libwebsockets.so
# LIBWEBSOCKETS_LIBRARIES, the library to link against
#
# This currently works probably only for Linux

find_package(PkgConfig)
pkg_check_modules(PC_LIBWEBSOCKETS QUIET libwebsockets)
set(LIBWEBSOCKETS_DEFINITIONS ${PC_LIBWEBSOCKETS_CFLAGS_OTHER})

find_path(LIBWEBSOCKETS_INCLUDE_DIR libwebsockets.h
    HINTS ${PC_LIBWEBSOCKETS_INCLUDEDIR} ${PC_LIBWEBSOCKETS_INCLUDE_DIRS}
)

find_library(LIBWEBSOCKETS_LIBRARY websockets
    HINTS ${PC_LIBWEBSOCKETS_LIBDIR} ${PC_LIBWEBSOCKETS_LIBRARY_DIRS}
)

set(LIBWEBSOCKETS_LIBRARIES ${LIBWEBSOCKETS_LIBRARY})
set(LIBWEBSOCKETS_INCLUDE_DIRS ${LIBWEBSOCKETS_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBWEBSOCKETS_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(LibWebSockets DEFAULT_MSG
    LIBWEBSOCKETS_LIBRARY LIBWEBSOCKETS_INCLUDE_DIR
)

mark_as_advanced(
    LIBWEBSOCKETS_LIBRARY
    LIBWEBSOCKETS_INCLUDE_DIR
)
