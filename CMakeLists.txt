cmake_minimum_required(VERSION 2.6)

project (ympd)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")

option(WITH_MPD_HOST_CHANGE "Let users of the web frontend change the MPD Host" ON)

find_package(LibMPDClient REQUIRED)
find_package(Threads REQUIRED)

configure_file(${PROJECT_SOURCE_DIR}/src/config.h.in
    ${PROJECT_BINARY_DIR}/config.h)
include_directories(${PROJECT_BINARY_DIR})

include(CheckCSourceCompiles)
include(CPack)

set(CMAKE_C_FLAGS "-std=gnu99 -Wall") 
set(CMAKE_C_FLAGS_DEBUG "-ggdb -pedantic")

add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/http_files.h
    COMMAND perl mkdata.pl index.html js/* assets/* css/* fonts/* > ${PROJECT_BINARY_DIR}/http_files.h
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/htdocs
)

set(SOURCES
    src/ympd.c
    src/http_server.c
    src/mpd_client.c
    src/mongoose.c
)

add_executable(ympd ${SOURCES} ${PROJECT_BINARY_DIR}/http_files.h)
target_link_libraries(ympd ${LIBMPDCLIENT_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS ympd DESTINATION bin)
install(FILES ympd.1 DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man1)
