cmake_minimum_required(VERSION 2.8)
PROJECT(generic_ccd CXX C)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake_modules/")

include(GNUInstallDirs)

set(UDEVRULES_INSTALL_DIR "/lib/udev/rules.d" CACHE STRING "Base directory for udev rules")

SET(BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall" )
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -DDEBUG_FITS" )
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall" )
SET(CMAKE_C_FLAGS_DEBUG "-O0 -g -DDEBUG_FITS" )
set(CMAKE_CXX_STANDARD_LIBRARIES -ljpeg)

set (GENERIC_VERSION_MAJOR 1)
set (GENERIC_VERSION_MINOR 0)

find_package(CFITSIO REQUIRED)
find_package(INDI REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)
find_package(JPEG REQUIRED)


configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/sony_a57.xml.cmake ${CMAKE_CURRENT_BINARY_DIR}/sony_a57.xml )

include_directories( ${CMAKE_CURRENT_BINARY_DIR})
include_directories( ${CMAKE_CURRENT_SOURCE_DIR})
include_directories( ${INDI_INCLUDE_DIR})
include_directories( ${CFITSIO_INCLUDE_DIR})


include(CMakeCommon)

############# GENERIC CCD ###############
set(genericccd_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/sony_a57.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/jpeg.cpp
)

add_executable(sony_a57 ${genericccd_SRCS})

target_link_libraries(sony_a57 ${INDI_LIBRARIES} ${CFITSIO_LIBRARIES} m ${ZLIB_LIBRARY} stdc++fs)

install(TARGETS sony_a57 RUNTIME DESTINATION bin)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/sony_a57.xml DESTINATION ${INDI_DATA_DIR})
