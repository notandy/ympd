cmake_minimum_required(VERSION 2.6)

project (ympd)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "4")
set(CPACK_GENERATOR "DEB;RPM;TGZ")
set(CPACK_SOURCE_GENERATOR "TBZ2")
set(DEBIAN_PACKAGE_SECTION "web")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "MPD web client based on Websockets and Bootstrap")
set(CPACK_PACKAGE_CONTACT "Andrew Karpow <andy@ympd.org>")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "andy@ndyk.de")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libssl1.0.0,libmpdclient2")
set(CPACK_SOURCE_PACKAGE_FILE_NAME
    "${PROJECT_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}")

option(WITH_STATIC_WEBSOCKETS "Build with static libwebsockets library" OFF)

find_package(LibMPDClient REQUIRED)
find_package(LibWebSockets REQUIRED)
if(WITH_STATIC_WEBSOCKETS)
    find_package(OpenSSL REQUIRED)
    find_package(ZLIB REQUIRED)
endif()

configure_file(${PROJECT_SOURCE_DIR}/src/config.h.in
    ${PROJECT_BINARY_DIR}/config.h)
include_directories(${PROJECT_BINARY_DIR})

include(CheckCSourceCompiles)
include(CPack)

set(CMAKE_C_FLAGS "-std=gnu99 -Wall") 
set(CMAKE_C_FLAGS_DEBUG "-ggdb -pedantic")

set(SOURCES
    src/ympd.c
    src/http_server.c
    src/mpd_client.c
)

add_executable(ympd ${SOURCES})

# TODO: use generator expressions introduced to CMake 2.8.12, too fresh yet
if(WITH_STATIC_WEBSOCKETS)
    find_library(LIBWEBSOCKETS_LIBRARY_STATIC libwebsockets.a)
    target_link_libraries(ympd ${LIBMPDCLIENT_LIBRARY}
        ${LIBWEBSOCKETS_LIBRARY_STATIC} ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES})
else()
    target_link_libraries(ympd ${LIBMPDCLIENT_LIBRARY}
        ${LIBWEBSOCKETS_LIBRARIES})
endif()


install(TARGETS ympd DESTINATION bin)
install(DIRECTORY htdocs DESTINATION share/${PROJECT_NAME})
install(FILES contrib/init.debian DESTINATION /etc/init.d
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE CONFIGURATIONS debrelease RENAME ${PROJECT_NAME})
install(FILES contrib/default.debian DESTINATION /etc/default/${PROJECT_NAME}
    CONFIGURATIONS debrelease RENAME ${PROJECT_NAME})

