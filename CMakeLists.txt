cmake_minimum_required(VERSION 2.6)

project (ympd)

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "MPD web client based on Websockets and
Bootstrap")
set(CPACK_PACKAGE_VENDOR "Andrew Karpow <andy@ndyk.de>")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "andy@ndyk.de")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "1")
set(CPACK_PACKAGE_VERSION_PATCH "")
set(CPACK_BINARY_DEB "ON")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/")

find_package(LibWebSockets REQUIRED)
find_package(LibMPDClient REQUIRED)
include(CheckCSourceCompiles)
include(CPack)

SET(CMAKE_C_FLAGS_DEBRELEASE "-O2 -DNDEBUG -pipe")

set(SOURCES
    src/ympd.c
    src/http_server.c
    src/mpd_client.c
    )

add_executable(ympd ${SOURCES})
add_definitions(-DDATADIR="${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}")

target_link_libraries(ympd ${LIBMPDCLIENT_LIBRARY} ${LIBWEBSOCKETS_LIBRARIES})

install(TARGETS ympd DESTINATION bin)
install(DIRECTORY htdocs DESTINATION share/${PROJECT_NAME})
install(FILES contrib/init.debian DESTINATION /etc/init.d
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE CONFIGURATIONS debrelease RENAME ${PROJECT_NAME})
install(FILES contrib/default.debian DESTINATION /etc/default/${PROJECT_NAME}
    CONFIGURATIONS debrelease RENAME ${PROJECT_NAME})
