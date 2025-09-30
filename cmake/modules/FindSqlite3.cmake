# * Try to find sqlite3 Find sqlite3 headers, libraries and the answer to all
#   questions.
#
# SQLITE3_FOUND               True if sqlite3 got found SQLITE3_INCLUDEDIR
# Location of sqlite3 headers SQLITE3_LIBRARIES           List of libaries to
# use sqlite3 SQLITE3_DEFINITIONS         Definitions to compile sqlite3
#
# Copyright (c) 2007 Juha Tuomala <tuju@iki.fi> Copyright (c) 2007 Daniel Gollub
# <gollub@b1-systems.de> Copyright (c) 2007 Alban Browaeys <prahal@yahoo.com>
#
# Redistribution and use is allowed according to the terms of the New BSD
# license. For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#

include(FindPkgConfig)
# Take care about sqlite3.pc settings
if(Sqlite3_FIND_REQUIRED)
  set(_pkgconfig_REQUIRED "REQUIRED")
else(Sqlite3_FIND_REQUIRED)
  set(_pkgconfig_REQUIRED "")
endif(Sqlite3_FIND_REQUIRED)

if(SQLITE3_MIN_VERSION)
  pkg_search_module(SQLITE3 ${_pkgconfig_REQUIRED}
                    sqlite3>=${SQLITE3_MIN_VERSION})
else(SQLITE3_MIN_VERSION)
  pkg_search_module(SQLITE3 ${_pkgconfig_REQUIRED} sqlite3)
endif(SQLITE3_MIN_VERSION)

# Look for sqlite3 include dir and libraries w/o pkgconfig
if(NOT SQLITE3_FOUND AND NOT PKG_CONFIG_FOUND)
  find_path(_sqlite3_include_DIR sqlite3.h
            PATHS /opt/local/include/ /sw/include/ /usr/local/include/
                  /usr/include/)
  find_library(
    _sqlite3_link_DIR sqlite3
    PATHS /opt/local/lib
          /sw/lib
          /usr/lib
          /usr/local/lib
          /usr/lib64
          /usr/local/lib64
          /opt/lib64)
  if(_sqlite3_include_DIR AND _sqlite3_link_DIR)
    set(_sqlite3_FOUND TRUE)
  endif(_sqlite3_include_DIR AND _sqlite3_link_DIR)

  if(_sqlite3_FOUND)
    set(SQLITE3_INCLUDE_DIRS ${_sqlite3_include_DIR})
    set(SQLITE3_LIBRARIES ${_sqlite3_link_DIR})
  endif(_sqlite3_FOUND)

  # Report results
  if(SQLITE3_LIBRARIES
     AND SQLITE3_INCLUDE_DIRS
     AND _sqlite3_FOUND)
    set(SQLITE3_FOUND 1)
    if(NOT Sqlite3_FIND_QUIETLY)
      message(
        STATUS "Found sqlite3: ${SQLITE3_LIBRARIES} ${SQLITE3_INCLUDE_DIRS}")
    endif(NOT Sqlite3_FIND_QUIETLY)
  else(
    SQLITE3_LIBRARIES
    AND SQLITE3_INCLUDE_DIRS
    AND _sqlite3_FOUND)
    if(Sqlite3_FIND_REQUIRED)
      message(SEND_ERROR "Could NOT find sqlite3")
    else(Sqlite3_FIND_REQUIRED)
      if(NOT Sqlite3_FIND_QUIETLY)
        message(STATUS "Could NOT find sqlite3")
      endif(NOT Sqlite3_FIND_QUIETLY)
    endif(Sqlite3_FIND_REQUIRED)
  endif(
    SQLITE3_LIBRARIES
    AND SQLITE3_INCLUDE_DIRS
    AND _sqlite3_FOUND)

endif(NOT SQLITE3_FOUND AND NOT PKG_CONFIG_FOUND)

# Hide advanced variables from CMake GUIs
mark_as_advanced(SQLITE3_LIBRARIES SQLITE3_INCLUDE_DIRS)
