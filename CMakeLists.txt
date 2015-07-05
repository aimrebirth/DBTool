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
endif()

###############################################################################
#
# compiler and linker
#
###############################################################################

add_definitions(-DUSE_QT)

if (WIN32)
    if (MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
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

include_directories(../DatabaseManager/include)

add_subdirectory(../DatabaseManager DatabaseManager)
add_subdirectory(src)

###############################################################################