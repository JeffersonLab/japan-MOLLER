cmake_minimum_required(VERSION 3.5)

# Name of this project
project(QwAnalysis VERSION 2.0.0 LANGUAGES CXX)

# Local path for cmake modules
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules/")

# Default install path is the source directory
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    message(STATUS "    Install-prefix was at default -> forcing it to the source-dir" )
    message(STATUS "    Use -DCMAKE_INSTALL_PREFIX=/usr/local to set to something else" )
    set (CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}"
         CACHE PATH "default install path" FORCE)
endif()

# Use our modules and utility functions
include(CMakeEnv)

# Install in GNU-style directory layout
include(GNUInstallDirs)
set(INSTALL_CONFIGDIR "${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}")

# Use, i.e. don't skip the full RPATH for the build tree, and use relative paths
# for relocatable build products
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)

# When building, don't use the install RPATH already (but later on when
# installing)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_SKIP_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# Add the automatically determined parts of the RPATH which point to directories
# outside the build tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Mac support
message(STATUS "System name ${CMAKE_SYSTEM_NAME}")
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_MACOSX_RPATH ON)
    list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
    if("${isSystemDir}" STREQUAL "-1")
        set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
    endif()
endif()

# Export compile commands as json for run-clang-tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#----------------------------------------------------------------------------
# Sources and headers
#
file(GLOB my_project_sources
		 Analysis/src/*.cc
		 Parity/src/*.cc
		 )
list(REMOVE_ITEM my_project_sources
  ${PROJECT_SOURCE_DIR}/Analysis/src/QwF1TDContainer.cc
  ${PROJECT_SOURCE_DIR}/Analysis/src/QwSIS3320_Samples.cc
  )

file(GLOB my_project_headers
		 Analysis/include/*.h
		 Parity/include/*.h
		 )
list(REMOVE_ITEM my_project_headers
		 ${PROJECT_SOURCE_DIR}/Analysis/include/QwF1TDContainer.h
		 ${PROJECT_SOURCE_DIR}/Analysis/include/QwSIS3320_Samples.h
		 )

#----------------------------------------------------------------------------
# MYSQLPP
#
find_package(MYSQLPP)
IF(MYSQLPP_FOUND)
  config_add_dependency(MYSQLPP)
  include_directories(${MYSQLPP_INCLUDE_DIR})
  ## KLUDGE:  For now, do not define the __USE_DATABASE__ flag, because we're still missing some of the database support files. --- P.King, 05jun2018
  message(STATUS "  The \"__USE_DATABASE__\" flag is commented out in CMakeLists.txt; database support is DISABLED.")
  #  add_definitions(-D__USE_DATABASE__)
ELSE(MYSQLPP_FOUND)
  set(MYSQLPP_LIBRARIES "")
  list(REMOVE_ITEM my_project_sources
    ${PROJECT_SOURCE_DIR}/Analysis/src/QwDatabase.cc
  )
endif(MYSQLPP_FOUND)

#----------------------------------------------------------------------------
# Boost
#
find_package(Boost COMPONENTS program_options filesystem system regex REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIR})

#----------------------------------------------------------------------------
# ROOT
#
set(minimum_root_version 6.0)
find_package(ROOT ${minimum_root_version} REQUIRED New Gui)
config_add_dependency(ROOT ${minimum_root_version})


#----------------------------------------------------------------------------
# gitinfo.cc
#
set(PRE_CONFIGURE_FILE ${PROJECT_SOURCE_DIR}/cmake/templates/gitinfo.cc.in)
set(POST_CONFIGURE_FILE ${CMAKE_BINARY_DIR}/gitinfo.cc)
include(CMakeGitInfo)
list(APPEND my_project_sources ${POST_CONFIGURE_FILE})
#----------------------------------------------------------------------------
# Version 
#
configure_file(${PROJECT_SOURCE_DIR}/cmake/templates/version-config.in ${CMAKE_BINARY_DIR}/version-config.h)
list(APPEND my_project_headers ${CMAKE_BINARY_DIR}/version-config.h)


#----------------------------------------------------------------------------
# Compiler flags
#
set_compiler_flags("${ROOT_CXX_FLAGS}")
#set_diagnostic_flags(WALL WEXTRA)
set_diagnostic_flags(WALL)
report_build_info()


#----------------------------------------------------------------------------
# evio library
#
add_subdirectory(evio)
include_directories(evio/include)

#----------------------------------------------------------------------------
# et library
#
find_package(ET)
IF(ET_FOUND)
  #  The QwEventBuffer class also needs to see the "__CODA_ET" define,
  #  not just the EVIO library.
  add_definitions(-D__CODA_ET)
ENDIF(ET_FOUND)

#----------------------------------------------------------------------------
# main dictionaries
#
message(STATUS "Generating ${PROJECT_NAME} dictionaries for ROOT ${ROOT_VERSION}...")
foreach(file ${my_project_headers})
  # Find the header files that have uncommented ClassDef lines
  execute_process(COMMAND grep -l "[^/]ClassDef" ${file} OUTPUT_VARIABLE result)
  if(result)
    # Get path and filename without extension
    get_filename_component(file_path ${file} PATH)
    get_filename_component(file_name ${file} NAME_WE)
    # Add dictionary target with possible LinkDef file
    set(dict ${CMAKE_CURRENT_BINARY_DIR}/${file_name}Dict)
    set(linkdef ${file_path}/${file_name}LinkDef.h)
    build_root_dictionary(${file_name} ${file}
      TARGETS ${PROJECT_NAME}
      LINKDEF ${linkdef}
    )
    # Add dictionary to project sources
    set(my_project_sources ${my_project_sources} ${dict}.cxx)
  endif()
endforeach()


#----------------------------------------------------------------------------
# main library
#
add_library(${PROJECT_NAME} SHARED ${my_project_sources} ${my_project_headers})
add_dependencies(${PROJECT_NAME} check_git_repository)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Analysis/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Parity/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
		$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
  )

target_compile_options(${PROJECT_NAME}
  PUBLIC
    ${${PROJECT_NAME_UC}_CXX_FLAGS_LIST}
  PRIVATE
    ${${PROJECT_NAME_UC}_DIAG_FLAGS_LIST}
  )

if(${CMAKE_CXX_STANDARD} LESS 17)
  target_compile_definitions(${PROJECT_NAME} PUBLIC QW_ENABLE_MAPFILE)
endif()

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    eviowrapper
  PUBLIC
    ROOT::Libraries
    ${MYSQLPP_LIBRARIES}
    ${Boost_LIBRARIES}
  )

install(TARGETS ${PROJECT_NAME}
  EXPORT ${MAIN_PROJECT_NAME_LC}-exports
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
install(FILES ${my_project_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

#----------------------------------------------------------------------------
# executables
#
file(GLOB exefiles
  Parity/main/*.cc
  Analysis/main/*.cc
)
foreach(file ${exefiles})
  get_filename_component(filename ${file} NAME_WE)
  string(TOLOWER ${filename} filelower)

  add_executable(${filelower} ${file})

  target_link_libraries(${filelower}
    PRIVATE
      ${PROJECT_NAME}
  )
  target_compile_options(${filelower}
    PUBLIC
      ${${PROJECT_NAME_UC}_CXX_FLAGS_LIST}
    PRIVATE
      ${${PROJECT_NAME_UC}_DIAG_FLAGS_LIST}
  )
  if(${CMAKE_SYSTEM_NAME} MATCHES Linux)
    target_compile_options(${filelower} PUBLIC -fPIC)
  endif()

  install(TARGETS ${filelower}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
endforeach()

#----------------------------------------------------------------------------
#  Build feedback library and executable
### add_subdirectory(Feedback)

#----------------------------------------------------------------------------
# uninstall
#
if(NOT TARGET uninstall)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY)

    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()


#----------------------------------------------------------------------------
# cmake config
#
add_subdirectory(cmake)

