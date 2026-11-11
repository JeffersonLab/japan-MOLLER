find_path(EPICS_INCLUDE_DIR
    NAMES epicsexcept.h epicsTypes.h
    HINTS ENV EPICS
    PATH_SUFFIXES include
)

find_path(EPICS_OS_INCLUDE_DIR
    NAMES osdSock.h osdTime.h
    HINTS ENV EPICS
    PATH_SUFFIXES include/os/Linux include/os/default
)
find_path(EPICS_GCC_INCLUDE_DIR
    NAMES compilerSpecific.h epicsAtomicCD.h
    HINTS ENV EPICS
    PATH_SUFFIXES include/compiler include/compiler/gcc
)

if(NOT EPICS_FIND_COMPONENTS)
    set(EPICS_FIND_COMPONENTS Com ca dbCore dbRecStd gdd cas Cap5)
endif()
foreach(comp ${EPICS_FIND_COMPONENTS})
    # Search for the library on disk
    find_library(EPICS_${comp}_LIBRARY
        NAMES ${comp}
        HINTS ENV EPICS
        PATH_SUFFIXES lib/linux-x86_64 lib
    )

    # Check if this specific component was successfully found
    if(EPICS_${comp}_LIBRARY)
        set(EPICS_${comp}_FOUND TRUE)
    else()
        set(EPICS_${comp}_FOUND FALSE)
    endif()
endforeach()

# Standardize status messaging (Found/Not Found) and error handling
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(EPICS
    REQUIRED_VARS EPICS_Com_LIBRARY EPICS_INCLUDE_DIR EPICS_OS_INCLUDE_DIR EPICS_GCC_INCLUDE_DIR 
	HANDLE_COMPONENTS
)
if(EPICS_FOUND)
    foreach(comp ${EPICS_FIND_COMPONENTS})
        if(EPICS_${comp}_FOUND AND NOT TARGET EPICS::${comp})
            add_library(EPICS::${comp} UNKNOWN IMPORTED)
            set_target_properties(EPICS::${comp} PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${EPICS_INCLUDE_DIR};${EPICS_OS_INCLUDE_DIR};${EPICS_GCC_INCLUDE_DIR}"
                IMPORTED_LOCATION "${EPICS_${comp}_LIBRARY}"
            )

            # Optional: Establish internal EPICS dependencies if applicable
            # (e.g., Channel Access 'ca' requires 'Com')
            if(comp STREQUAL "ca" AND TARGET EPICS::Com)
                set_property(TARGET EPICS::ca APPEND PROPERTY
                    INTERFACE_LINK_LIBRARIES EPICS::Com
                )
            endif()
        endif()
    endforeach()
endif()
