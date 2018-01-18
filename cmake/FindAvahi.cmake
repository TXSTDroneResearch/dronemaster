# - Try to find avahi
# Once done this will define
#
# AVAHI_FOUND - system has avahi
# AVAHI_INCLUDE_DIRS - All avahi include directories
# AVAHI_LIBRARIES - All avahi libraries
# AVAHI_LIBRARY_DIRS - All library directories
#
# Variables are defined for the specific modules as well.
# AVAHI_CLIENT_*
# AVAHI_COMMON_*
# AVAHI_CORE_*

if(PKG_CONFIG_FOUND)
    pkg_check_modules(AVAHI_CLIENT avahi-client)
    pkg_check_modules(AVAHI_COMMON avahi-common)
    pkg_check_modules(AVAHI_CORE avahi-core)
else()
    find_path(AVAHI_CLIENT_INCLUDE_DIRS avahi-client/client.h)
    find_path(AVAHI_COMMON_INCLUDE_DIRS avahi-common/defs.h)
    find_path(AVAHI_CORE_INCLUDE_DIRS avahi-common/defs.h)
    find_library(AVAHI_CLIENT_LIBRARIES avahi-client)
    find_library(AVAHI_COMMON_LIBRARIES avahi-common)
    find_library(AVAHI_CORE_LIBRARIES avahi-core)
endif()

set(AVAHI_INCLUDE_DIRS 
        ${AVAHI_CLIENT_INCLUDE_DIRS}
        ${AVAHI_COMMON_INCLUDE_DIRS}
        ${AVAHI_CORE_INCLUDE_DIRS}
)
set(AVAHI_LIBRARIES
        ${AVAHI_CLIENT_LIBRARIES}
        ${AVAHI_COMMON_LIBRARIES}
        ${AVAHI_CORE_LIBRARIES}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Avahi DEFAULT_MSG AVAHI_INCLUDE_DIRS AVAHI_LIBRARIES)

mark_as_advanced(AVAHI_INCLUDE_DIRS AVAHI_LIBRARIES)
list(APPEND AVAHI_DEFINITIONS -DHAVE_LIBAVAHI_COMMON=1 -DHAVE_LIBAVAHI_CLIENT=1)