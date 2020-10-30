# - Find taf
#
# -*- cmake -*-
#
# Find the native EVENT includes and library
#
#  EVENT_INCLUDE_DIRS - where to find mysql.h, etc.
#  EVENT_LIBRARIES   - List of libraries when using EVENT.
#  EVENT_FOUND       - True if EVENT found.

if(MSVC)
    IF (MT_MODE)
        set(Event-Ext ${Event-Ext}-mt)
    ENDIF()

    IF (CMAKE_BUILD_TYPE STREQUAL "Debug")
        message("CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
        set(Event-Ext ${Event-Ext}.debug)
    ENDIF ()
else()
    set(Event-Ext "") 
    IF (CMAKE_BUILD_TYPE STREQUAL "Debug")
        message("CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
        set(Event-Ext ${Event-Ext}.debug)
    ENDIF ()
endif()

find_path(EVENT_INCLUDE_DIR
        NAMES event/EventInterface.h
        PATHS 
        /usr/local/upevent/ C:/upevent/
        NO_DEFAULT_PATH 
        PATH_SUFFIXES
        include)

message("upevent${Event-Ext}")

find_library(EVENT_LIB
        NAMES upevent${Event-Ext}
        PATHS 
        /usr/local/upevent/ C:/upevent/
        NO_DEFAULT_PATH 
        PATH_SUFFIXES
        lib)


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Event DEFAULT_MSG EVENT_LIB EVENT_INCLUDE_DIR )
mark_as_advanced(EVENT_INCLUDE_DIR EVENT_LIB)

if(EVENT_FOUND)
    set(EVENT_INCLUDE_DIRS ${EVENT_INCLUDE_DIR})
    set(EVENT_LIBRARIES ${EVENT_LIB} )
    add_definitions( -DWITH_EVENT )
    message("----------------------------------------------------")
    message("EVENT_FOUND:               ${EVENT_FOUND}")
    message("EVENT_INCLUDE_DIRS:        ${EVENT_INCLUDE_DIRS}")
    message("EVENT_LIBRARIES:           ${EVENT_LIBRARIES}")
    #-------------------------------------------------------------
endif()

