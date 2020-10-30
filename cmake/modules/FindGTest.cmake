# - Find taf
#
# -*- cmake -*-
#
# Find the native TAF includes and library
#
#  GTEST_INCLUDE_DIRS - where to find mysql.h, etc.
#  GTEST_LIBRARIES   - List of libraries when using TAF.
#  GTEST_FOUND       - True if TAF found.

if(MSVC)
    IF (MT_MODE)
        set(GTest-Ext ${GTest-Ext}-mt)
    ENDIF()

    IF (CMAKE_BUILD_TYPE STREQUAL "Debug")
        message("CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
        set(GTest-Ext ${GTest-Ext}.debug)
    ENDIF ()
else()
    set(GTest-Ext "") 
    IF (CMAKE_BUILD_TYPE STREQUAL "Debug")
        message("CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
        set(GTest-Ext ${GTest-Ext}.debug)
    ENDIF ()
endif()

find_path(GTEST_INCLUDE_DIR
        NAMES gtest/gtest.h
        PATHS
        /usr/local/thirdparty/ C:/thirdparty/
        NO_DEFAULT_PATH
        PATH_SUFFIXES
        include)

find_library(_GTEST_LIB
        NAMES gtest${GTest-Ext}
        PATHS
        /usr/local/thirdparty/ C:/thirdparty/
        NO_DEFAULT_PATH
        PATH_SUFFIXES
        lib)

find_library(_GTEST_MAIN_LIB
        NAMES gtest${GTest-Ext}
        PATHS
        /usr/local/thirdparty/ C:/thirdparty/
        NO_DEFAULT_PATH
        PATH_SUFFIXES
        lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GTest DEFAULT_MSG _GTEST_LIB _GTEST_MAIN_LIB GTEST_INCLUDE_DIR)
mark_as_advanced(GTEST_INCLUDE_DIR _GTEST_LIB _GTEST_MAIN_LIB)

if(GTEST_FOUND)
    set(GTEST_INCLUDE_DIRS ${GTEST_INCLUDE_DIR})
    set(GTEST_LIBRARIES ${_GTEST_LIB} ${_GTEST_MAIN_LIB})

    add_definitions( -DWITH_GTEST )
    message("----------------------------------------------------")
    message("GTest_FOUND:                 ${GTEST_FOUND}")
    message("GTEST_INCLUDE_DIRS:          ${GTEST_INCLUDE_DIRS}")
    message("GTEST_LIBRARIES:             ${GTEST_LIBRARIES}")
    #-------------------------------------------------------------
endif()

