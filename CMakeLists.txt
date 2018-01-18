########################################################################
IF(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
MESSAGE(FATAL_ERROR "Prevented in-tree built. This is bad practice.")
ENDIF(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})

########################################################################
# Project setup
########################################################################
CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
PROJECT(MasterServer CXX)
ENABLE_TESTING()

# include(FindPkgConfig)

set (CMAKE_CXX_STANDARD 11)
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${PROJECT_SOURCE_DIR})

# C++ Include directories
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

FILE(GLOB MASTERSERVER_SOURCES
"src/*.cc"
"src/*.h"
)

# SET(RTMATH_USE_DOUBLE false CACHE BOOL "Use doubles instead of floats internally")
SET(USE_AVAHI false CACHE BOOL "Use Avahi")

ADD_EXECUTABLE(MasterServer ${MASTERSERVER_SOURCES})

if (USE_AVAHI)
    find_package(Avahi REQUIRED)
    target_link_libraries(MasterServer ${AVAHI_LIBRARIES})
    add_definitions(-DRC_USE_AVAHI=1)
endif()

# find_library(MPSSE MPSSE ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/libMPSSE)
# if(${MPSSE} STREQUAL "MPSSE-NOTFOUND")
# message(FATAL_ERROR "Couldn't find the 'MPSSE' library")
# endif()

# target_link_libraries(RTIMULib ${MPSSE})
# SET_PROPERTY(TARGET RTIMULib PROPERTY LINKER_LANGUAGE CXX)
