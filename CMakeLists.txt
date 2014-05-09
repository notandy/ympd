cmake_minimum_required(VERSION 2.6)

project (ympd)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "2")
set(CPACK_PACKAGE_VERSION_PATCH "2")

option(WITH_MPD_HOST_CHANGE "Let users of the web frontend change the MPD Host" ON)

find_package(LibMPDClient REQUIRED)
find_package(Threads REQUIRED)

configure_file(src/config.h.in ${PROJECT_BINARY_DIR}/config.h)
include_directories(${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR})

include(CheckCSourceCompiles)

set(CMAKE_C_FLAGS "-std=gnu99 -Wall") 
set(CMAKE_C_FLAGS_DEBUG "-ggdb -pedantic")

file(GLOB RESOURCES 
	RELATIVE ${PROJECT_SOURCE_DIR}
	htdocs/js/*
	htdocs/assets/*
	htdocs/css/*.min.css
	htdocs/fonts/*
	htdocs/index.html
)

add_executable(mkdata htdocs/mkdata.c)
get_target_property(MKDATA_EXE mkdata LOCATION)

add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/assets.c
    COMMAND ${MKDATA_EXE} ${RESOURCES} > ${PROJECT_BINARY_DIR}/assets.c
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    DEPENDS ${RESOURCES} mkdata
)

set(SOURCES
    src/ympd.c
    src/http_server.c
    src/mpd_client.c
    src/mongoose.c
    src/json_encode.c
    assets.c
)

add_executable(ympd ${SOURCES})
target_link_libraries(ympd ${LIBMPDCLIENT_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS ympd DESTINATION bin)
install(FILES ympd.1 DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man1)
