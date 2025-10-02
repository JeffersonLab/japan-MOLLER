cmake_minimum_required(VERSION 3.5)
cmake_policy(SET CMP0079 NEW)

#----------------------------------------------------------------------------
# Set name of lib being built
#
set(LIBNAME eviowrapper)

#----------------------------------------------------------------------------
# Sources and headers
#
file(GLOB my_evio_headers
  include/THaCoda*.h
  include/CustomAlloc.h
  include/Helper.h
  )
file(GLOB my_evio_sources
  src/THaCoda*.C
  )

#----------------------------------------------------------------------------
# EVIO
find_package(EVIO CONFIG QUIET)
if(NOT EVIO_FOUND)
  find_package(EVIO MODULE)
  if(NOT EVIO_FOUND)
    include(FetchBuildEVIO)
  endif()
endif()

#----------------------------------------------------------------------------
# CODA ET
#
find_package(ET CONFIG QUIET)
if(NOT ET_FOUND)
  find_package(ET MODULE)
endif()
if(ET_FOUND)
  	add_definitions(-D__CODA_ET)
	# Sources and headers
	file(GLOB my_et_headers
		include/THaEtClient.h
  )
	file(GLOB my_et_sources
		src/THaEtClient.C
  )
	set(my_evio_sources ${my_evio_sources} ${my_et_sources})
	set(my_evio_headers ${my_evio_headers} ${my_et_headers})
endif()


#----------------------------------------------------------------------------
# dictionaries
#
message(STATUS "Generating evio dictionaries for ROOT ${ROOT_VERSION}...")
foreach(file ${my_evio_headers})
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
    # Add dictionary to evio sources
    set(my_evio_sources ${my_evio_sources} ${file_name}Dict.cxx)
  endif()
endforeach()

#----------------------------------------------------------------------------
# evio library
#
add_library(eviowrapper SHARED ${my_evio_sources})

target_include_directories(${LIBNAME}
  PUBLIC
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
		$<BUILD_INTERFACE:${ET_INCLUDE_DIR}>
  )

target_compile_options(eviowrapper
  PUBLIC
    ${${PROJECT_NAME_UC}_CXX_FLAGS_LIST}
  PRIVATE
    ${${PROJECT_NAME_UC}_DIAG_FLAGS_LIST}
  )

if(ET_FOUND)
target_link_libraries(eviowrapper
  PUBLIC
    EVIO::ET
    ROOT::Core
  PRIVATE
    EVIO::EVIO
  )
else()
target_link_libraries(eviowrapper
  PUBLIC
    ROOT::Core
  PRIVATE
    EVIO::EVIO
  )
endif()

install(TARGETS eviowrapper
  EXPORT ${MAIN_PROJECT_NAME_LC}-exports
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

install(FILES ${my_evio_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

