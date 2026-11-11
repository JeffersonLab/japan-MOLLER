cmake_minimum_required(VERSION 3.12)

# Name of this project
project(QwFeedback VERSION 2.0.0 LANGUAGES CXX)


#----------------------------------------------------------------------------
# Sources and headers
#
file(GLOB SRC CONFIGURE_DEPENDS
		 src/*.cc
)
file(GLOB INC CONFIGURE_DEPENDS
		 include/*.h
)

#----------------------------------------------------------------------------
# Compiler flags
#
set_compiler_flags("${ROOT_CXX_FLAGS}")
set_diagnostic_flags(WALL)

#----------------------------------------------------------------------------
# ET library
#
if(NOT ET_FOUND)
  message(WARNING "${PROJECT_NAME} Skipped: ET Not Found!")
  # return()
endif()

add_subdirectory(EPICS)
if(NOT EPICS_FOUND)
  message(WARNING "${PROJECT_NAME} Skipped: EPICS Not Found!")
  return()
endif()
message(STATUS "NOTICE: ${PROJECT_NAME} will be built!")

add_library(${PROJECT_NAME} SHARED ${SRC})
target_link_libraries(${PROJECT_NAME} PUBLIC
	QwEPICS
	${CMAKE_PROJECT_NAME}
)

target_sources(${PROJECT_NAME}
	PUBLIC
	FILE_SET HEADERS
	BASE_DIRS include
	FILES ${INC}
)

install(TARGETS ${PROJECT_NAME}
    EXPORT ${CMAKE_PROJECT_NAME}
    FILE_SET HEADERS
)


#----------------------------------------------------------------------------
# executables
#
file(GLOB exefiles CONFIGURE_DEPENDS
  main/*.cc
  main/*.cc
)
foreach(file ${exefiles})
  get_filename_component(filename ${file} NAME_WE)
  string(TOLOWER ${filename} filelower)

  add_executable(${filelower} ${file})

  target_link_libraries(${filelower}
    PRIVATE
      ${CMAKE_PROJECT_NAME}
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

