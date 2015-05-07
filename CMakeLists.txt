#
# Polygon-4 DB Tool
#

###############################################################################
#
# cmake settings
#
###############################################################################

cmake_minimum_required(VERSION 2.8.11)

# In-source builds are disabled.
if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR
        "CMake generation is not possible within the source directory!"
        "\n Remove the CMakeCache.txt file and try again from another folder, e.g.:"
        "\n "
        "\n rm CMakeCache.txt"
        "\n mkdir build"
        "\n cd build"
        "\n cmake .."
    )
endif()

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")
SET(LIBRARY_OUTPUT_PATH "${EXECUTABLE_OUTPUT_PATH}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${EXECUTABLE_OUTPUT_PATH}")

###############################################################################
#
# project settings
#
###############################################################################

project(db_tool C CXX)

# libraries and dependencies
if (WIN32)
    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_STATIC_RUNTIME ON)
else()
    ADD_DEFINITIONS(-DBOOST_ALL_DYN_LINK)
endif(WIN32)

find_package(Boost 1.54 REQUIRED filesystem system)
find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Multimedia REQUIRED)
find_package(Qt5LinguistTools REQUIRED)

if(WIN32)
    find_package(OpenGl REQUIRED)
    
    find_library(Qt5PlatformSupport_LIBRARY_DEBUG NAMES Qt5PlatformSupportd HINTS ${imported_implib})
    find_library(Qt5PlatformSupport_LIBRARY_RELEASE NAMES Qt5PlatformSupport HINTS ${imported_implib})
    set(Qt5PlatformSupport_LIBRARY debug ${Qt5PlatformSupport_LIBRARY_DEBUG} optimized ${Qt5PlatformSupport_LIBRARY_RELEASE})
    
    find_library(qtharfbuzzngd_LIBRARY_DEBUG NAMES qtharfbuzzngd HINTS ${imported_implib})
    find_library(qtharfbuzzng_LIBRARY_RELEASE NAMES qtharfbuzzng HINTS ${imported_implib})
    set(qtharfbuzzng_LIBRARY debug ${qtharfbuzzngd_LIBRARY_DEBUG} optimized ${qtharfbuzzng_LIBRARY_RELEASE})
endif()

###############################################################################
#
# compiler and linker
#
###############################################################################

add_definitions(-DUSE_QT)

if (WIN32)
    if (MSVC)
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")

        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT /MP")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd /MP")
    endif()
elseif (UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")
endif()

###############################################################################
#
# build
#
###############################################################################

include_directories(${Boost_INCLUDE_DIRS})

include_directories(${Qt5Core_INCLUDE_DIRS})
include_directories(${Qt5Widgets_INCLUDE_DIRS})
include_directories(${Qt5Multimedia_INCLUDE_DIRS})

link_directories(${Boost_LIBRARY_DIRS})

include_directories(dep/dbmgr/include)

add_subdirectory(dep/dbmgr)
add_subdirectory(src)

###############################################################################