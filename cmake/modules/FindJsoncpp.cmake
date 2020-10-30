# - Find taf
#
# -*- cmake -*-
#
# Find the native EVENT includes and library
#
#  JSONCPP_INCLUDE_DIR - where to find *.h, etc.
#  JSONCPP_LIBRARIES   - List of libraries when using JSONCPP.
#  JSONCPP_FOUND       - True if JSONCPP found.

if(MSVC)
    IF(MT_MODE)
        set(Json-Ext "${Json-Ext}-mt")
    ENDIF()

    IF (CMAKE_BUILD_TYPE STREQUAL "Debug")
        message("CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
        set(Json-Ext "${Json-Ext}.debug")
    ENDIF ()
else()
    set(Json-Ext "") 
    IF (CMAKE_BUILD_TYPE STREQUAL "Debug")
        message("CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
        set(Json-Ext ${Json-Ext}.debug)
    ENDIF ()
endif()


find_path(JSONCPP_INCLUDE_DIR
        NAMES json/json.h
        PATHS /usr/local/thirdparty/include C:/thirdparty/include
        NO_DEFAULT_PATH 
        PATH_SUFFIXES
        include)

#message(jsoncpp${Json-Ext})
find_library(JSONCPP_LIB
        NAMES jsoncpp${Json-Ext}
        PATHS /usr/local/thirdparty/lib C:/thirdparty/lib
        NO_DEFAULT_PATH 
        PATH_SUFFIXES 
        lib)


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Jsoncpp JSONCPP_INCLUDE_DIR JSONCPP_LIB)
mark_as_advanced(JSONCPP_INCLUDE_DIR JSONCPP_LIB )

if(JSONCPP_FOUND)
    set(JSONCPP_INCLUDE_DIRS ${JSONCPP_INCLUDE_DIR})
    set(JSONCPP_LIBRARIES ${JSONCPP_LIB} )
    add_definitions( -DWITH_JSONCPP)
    message("----------------------------------------------------")
    message("JSONCPP_FOUND:               ${JSONCPP_FOUND}")
    message("JSONCPP_INCLUDE_DIRS:        ${JSONCPP_INCLUDE_DIRS}")
    message("JSONCPP_LIBRARIES:           ${JSONCPP_LIBRARIES}")
    #-------------------------------------------------------------
endif()

