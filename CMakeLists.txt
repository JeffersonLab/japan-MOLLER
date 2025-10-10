cmake_minimum_required(VERSION 3.12)

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
file(GLOB my_project_headers
		 Analysis/include/*.h
		 Parity/include/*.h
		 )


#----------------------------------------------------------------------------
# sqlpp11
#
list(APPEND SQLPP11_OPTIONAL_COMPONENTS SQLite3 MySQL MariaDB PostgreSQL)
find_package(Sqlpp11 QUIET OPTIONAL_COMPONENTS ${SQLPP11_OPTIONAL_COMPONENTS})
if(NOT Sqlpp11_FOUND)
  if(EXISTS ${PROJECT_SOURCE_DIR}/thirdparty/sqlpp11/CMakeLists.txt)
    message(STATUS "sqlpp11 included as subproject")
    add_subdirectory(${PROJECT_SOURCE_DIR}/thirdparty/sqlpp11)
    list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/thirdparty/sqlpp11/cmake/modules)
    set(Sqlpp11_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/thirdparty/sqlpp11/include)
    # Treat date dependency of sqlpp11 as system headers
    # to hide compiler warnings
    include_directories(SYSTEM ${date_SOURCE_DIR}/include)
  else()
    message(WARNING "sqlpp11 not available in thirdparty directory. Use 'git clone --recurse-submodules` on new clone or 'git submodule update --init --recursive' on existing repository.")
  endif()
endif()
if(TARGET sqlpp11::sqlpp11)
  config_add_dependency(Sqlpp11)
  include_directories(${Sqlpp11_INCLUDE_DIR})
  add_definitions(-D__USE_DATABASE__)
  list(APPEND Sqlpp11_LIBRARIES sqlpp11::sqlpp11)

  # Identify found backend components
  foreach(component ${SQLPP11_OPTIONAL_COMPONENTS})
    string(TOUPPER ${component} component_upper)
    string(TOLOWER ${component} component_lower)
    if(TARGET sqlpp11::${component_lower})
      add_definitions(-D__USE_DATABASE_${component_upper}__)
      list(APPEND Sqlpp11_LIBRARIES sqlpp11::${component_lower})
    endif()
  endforeach()
  message(STATUS "Found ${Sqlpp11_LIBRARIES}")

  # When building sqlpp11::mysql connector, we need to ensure dependencies are found in parent scope
  if(BUILD_MYSQL_CONNECTOR)
    find_package(MySQL QUIET)
    list(APPEND Sqlpp11_LIBRARIES MySQL::MySQL)
  endif()

  # Generate C++ headers from SQL schema at build time
  set(SCHEMA_OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Parity/include)
  set(SCHEMA_OUTPUT_FILE ${SCHEMA_OUTPUT_DIR}/QwParitySchema.h)
  set(SCHEMA_INPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/Parity/prminput/qwparity_schema.sql)
  
  # Find sqlpp11-ddl2cpp executable
  find_program(SQLPP11_DDL2CPP
    NAMES ddl2cpp
    PATHS thirdparty/sqlpp11/scripts
    DOC "sqlpp11 DDL to C++ converter"
    QUIET
  )
  if(TARGET sqlpp11::ddl2cpp)
    set(SQLPP11_DDL2CPP sqlpp11::ddl2cpp)
  endif()
  if(SQLPP11_DDL2CPP)
    add_custom_command(
      OUTPUT ${SCHEMA_OUTPUT_FILE}
      COMMAND ${SQLPP11_DDL2CPP}
        -identity-naming -no-timestamp-warning
        ${SCHEMA_INPUT_FILE}
        ${SCHEMA_OUTPUT_DIR}/QwParitySchema
        QwParitySchema
      DEPENDS ${SCHEMA_INPUT_FILE}
      COMMENT "Generating C++ headers from SQL schema"
      VERBATIM
    )
    
    # Add a custom target to ensure the schema is generated
    add_custom_target(generate_schema DEPENDS ${SCHEMA_OUTPUT_FILE})
  else()
    message(WARNING "sqlpp11-ddl2cpp not found, SQL schema headers may not be generated")
  endif()
ELSE()
  message(STATUS "Sqlpp11 not found; database functionally disabled")
  set(Sqlpp11_LIBRARIES "")
  list(REMOVE_ITEM my_project_sources
    ${PROJECT_SOURCE_DIR}/Analysis/src/QwDatabase.cc
  )
ENDIF()

#----------------------------------------------------------------------------
# Boost
#
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()
find_package(Boost 1.70 COMPONENTS program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIR})

#----------------------------------------------------------------------------
# ROOT
#
set(minimum_root_version 6.8)
find_package(ROOT ${minimum_root_version} REQUIRED New Gui)
config_add_dependency(ROOT ${minimum_root_version})

# Check ROOT version for RNTuple support (available in ROOT 6.36+)
if(${ROOT_VERSION} VERSION_GREATER_EQUAL "6.36.0")
    message(STATUS "ROOT version ${ROOT_VERSION} supports RNTuple - enabling RNTuple functionality")
    add_definitions(-DHAS_RNTUPLE_SUPPORT)
else()
    message(STATUS "ROOT version ${ROOT_VERSION} does not support RNTuple - disabling RNTuple functionality")
    message(STATUS "RNTuple support requires ROOT 6.36 or higher")
endif()


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
    ROOT_GENERATE_DICTIONARY(${dict} ${file} LINKDEF ${linkdef})
    # Add dictionary to project sources
    set(my_project_sources ${my_project_sources} ${dict}.cxx)
  endif()
endforeach()


#----------------------------------------------------------------------------
# main library
#
add_library(${PROJECT_NAME} SHARED ${my_project_sources} ${my_project_headers})
add_dependencies(${PROJECT_NAME} check_git_repository)

# Add schema generation dependency if sqlpp11 is available
if((Sqlpp11_FOUND OR TARGET sqlpp11::sqlpp11) AND TARGET generate_schema)
  add_dependencies(${PROJECT_NAME} generate_schema)
endif()

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

# TMapFile is broken with C++17 or higher and ROOT < 6.32
if(${CMAKE_CXX_STANDARD} LESS 17 OR ${ROOT_VERSION} VERSION_GREATER_EQUAL 6.32)
  target_compile_definitions(${PROJECT_NAME} PUBLIC QW_ENABLE_MAPFILE)
endif()

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    eviowrapper
  PUBLIC
    ROOT::Core ROOT::Tree ROOT::Hist ROOT::Rint
    $<TARGET_NAME_IF_EXISTS:ROOT::ROOTNTuple>
    ${Boost_LIBRARIES}
  )

# Link sqlpp11 if enabled
if((Sqlpp11_FOUND OR TARGET sqlpp11::sqlpp11) AND TARGET generate_schema)
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
      ${Sqlpp11_LIBRARIES}
  )
  target_include_directories(${PROJECT_NAME}
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/sqlpp11/install/include
  )
endif()

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
      eviowrapper
      ${Sqlpp11_LIBRARIES}
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

