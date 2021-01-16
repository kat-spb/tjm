cmake_minimum_required(VERSION 3.11)

project(TJM
    DESCRIPTION "TIFF to JPEG2000 and JPEG2000 to MFX (IMF4kRM7S0) convertor"
    HOMEPAGE_URL "https://github.com/kat-spb/tjm"
    LANGUAGES C CXX)

################################################################################
# Include CMake dependencies
################################################################################

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

################################################################################
# Options
################################################################################

option(OPENMP_ENABLED "Whether to enable OpenMP" ON)

###############################################################################
# Find packages
################################################################################

if(OPENMP_ENABLED)
    find_package(OpenMP QUIET)
endif()

find_package(TIFF REQUIRED)

if(NOT TIFF_FOUND)
    message(SEND_ERROR "Couldn't found libtiff")
endif()

if(UNIX)
    include (FindPkgConfig)
    find_package(PkgConfig QUIET)
    pkg_check_modules(OPENJPEG libopenjp2)
endif()

#if(NOT OPENJPEG_FOUND)
    #find_file(OpenJPEG_Targets_PATH NAMES OpenJPEGTargets.cmake ENV CMAKE_HINT PATH_SUFFIXES "targets")
    #find_path(OpenJPEG_include_DIR NAMES openjpeg.h)
    #find_library(OpenJPEGLib_Path NAMES openjp2 PATHS ENV CMAKE_HINT PATH_SUFFIXES "lib")
    #find_library(OpenJPEGLib_Debug_Path NAMES openjp2 PATHS ENV CMAKE_HINT PATH_SUFFIXES "lib")
#endif()

if (NOT OPENJPEG_FOUND)
    add_library(openjp2 SHARED IMPORTED)
    set_property(TARGET openjp2 PROPERTY
             IMPORTED_LOCATION /usr/local/lib/libopenjp2.so)
    target_link_libraries(${TARGET} openjp2)
endif()

#find kumu and asdcp
find_package(XercesC QUIET)

add_library(kumu SHARED IMPORTED)
set_property(TARGET kumu PROPERTY
             IMPORTED_LOCATION /usr/local/lib/libkumu.so)
target_link_libraries(${TARGET} kumu)


add_library(asdcp SHARED IMPORTED)
set_property(TARGET asdcp PROPERTY
             IMPORTED_LOCATION /usr/local/lib/libasdcp.so)
target_link_libraries(${TARGET} asdcp)


add_library(as02 SHARED IMPORTED)
set_property(TARGET as02 PROPERTY
             IMPORTED_LOCATION /usr/local/lib/libas02.so)
target_link_libraries(${TARGET} as02)

###############################################################################
# Compiler specific configuration
################################################################################

if(CMAKE_BUILD_TYPE)
    message(STATUS "Build type specified as ${CMAKE_BUILD_TYPE}")
else()
    message(STATUS "Build type not specified, using Release")
    set(CMAKE_BUILD_TYPE Release)
    set(IS_DEBUG FALSE)
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

if(OPENMP_ENABLED AND OPENMP_FOUND)
    message(STATUS "Enabling OpenMP support")
    add_definitions("-DOPENMP_ENABLED")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
else()
    message(STATUS "Disabling OpenMP support")
endif()

###############################################################################
# Add sources
################################################################################

#include(GenerateVersion)

add_library(tjm SHARED
    # headers
    src/opj.h
    src/mxf.h
    # sources
    src/opj.c
    src/opj/convert.c
    src/opj/converttif.c
    src/mxf.c
)

add_executable(j2c src/opj/converttif.c src/opj/convert.c src/opj.c src/filelist.c src/toj2c.c)
target_link_libraries(j2c  ${TIFF_LIBRARIES} openjp2)

add_executable(mxf src/filelist.c src/mxf.c src/tomxf)
set_source_files_properties("src/mxf.c" PROPERTIES LANGUAGE CXX )
target_link_libraries(mxf kumu asdcp as02)


################################################################################
# Install/uninstall script
################################################################################

install(TARGETS j2c mxf
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION bin
)

#configure_file("${CMAKE_SOURCE_DIR}/cmake/CMakeUninstall.cmake"
#               "${CMAKE_CURRENT_BINARY_DIR}/CMakeUninstall.cmake"
#               IMMEDIATE @ONLY)
#add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR
#set_target_properties(uninstall PROPERTIES FOLDER ${CMAKE_TARGETS_ROOT_FOLDER})

###############################################################################
## packaging ##################################################################
###############################################################################

# now comes everything we need, to create a package
# there are a lot more variables you can set, and some
# you need to set for some package types, but we want to
# be minimal here
set(CPACK_PACKAGE_NAME "tjm")
set(CPACK_PACKAGE_VERSION "1.0.0")

# we don't want to split our program up into several things
set(CPACK_MONOLITHIC_INSTALL 1)

# This must be last
include(CPack)
