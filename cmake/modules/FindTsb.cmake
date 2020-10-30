# - Find Tsb 
#
# -*- cmake -*-
#
# Find the native TSBDEPS includes and library
#
#  TSB_INCLUDE_DIR - where to find mysql.h, etc.
#  TSB_LIBRARIES   - List of libraries when using TSBDEPS.
#  TSB_FOUND       - True if TSBDEPS found.

IF (MT_MODE)
    set(Tsb-Ext ${Tsb-Ext}-mt)
ENDIF()

#IF (CMAKE_BUILD_TYPE STREQUAL "Debug")
#    message("CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
#    set(Tsb-Ext ${Tsb-Ext}.debug)
#ENDIF ()

set(TSB_FOUND_INC /usr/local/tsb/ c:/tsb /usr/local/thirdparty/ C:/thirdparty/)
set(TSB_FOUND_LIB /usr/local/tsb/ c:/tsb /usr/local/thirdparty/ C:/thirdparty/)
#set(TSB_FOUND_INC /home/upchina/chaochen/project/thirdparty D:/tsb_deps/include)
#set(TSB_FOUND_LIB /home/upchina/chaochen/project/thirdparty/lib/linux D:/tsb_deps/lib/windows/)
message("Tsb-Ext:           ${Tsb-Ext}")

find_path(TSB_INCLUDE_DIR
        NAMES tsb/RocksWrapper.h
        PATHS
        ${TSB_FOUND_INC}
        PATH_SUFFIXES
        include)

find_library(TSB_LIB_
        NAMES tsb${Tsb-Ext}
        PATHS         
        ${TSB_FOUND_LIB} 
        NO_DEFAULT_PATH
        PATH_SUFFIXES
        lib)

find_library(TSB_ROCKSDB_LIB
        NAMES rocksdb${Tsb-Ext}
        PATHS
        ${TSB_FOUND_LIB} 
        NO_DEFAULT_PATH
        PATH_SUFFIXES
        lib)

find_library(TSB_SNAPPY_LIB
        NAMES snappy${Tsb-Ext}
        PATHS
        ${TSB_FOUND_LIB} 
        NO_DEFAULT_PATH
        PATH_SUFFIXES
        lib)

find_library(TSB_SQLPARSER_LIB
        NAMES sqlparser${Tsb-Ext}
        PATHS
        ${TSB_FOUND_LIB} 
        NO_DEFAULT_PATH
        PATH_SUFFIXES
        lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Tsb DEFAULT_MSG TSB_SQLPARSER_LIB TSB_ROCKSDB_LIB TSB_SNAPPY_LIB TSB_INCLUDE_DIR)
mark_as_advanced(TSB_INCLUDE_DIR TSB_SQLPARSER_LIB TSB_ROCKSDB_LIB TSB_SNAPPY_LIB)

if(TSB_FOUND)
    set(TSB_INCLUDE_DIRS ${TSB_INCLUDE_DIR})
    if(MSVC)
        # Shlwapi Rpcrt4 这两个是windows的系统库，配合rocksdb使用
        set(TSB_LIBRARIES ${TSB_LIB_} ${TSB_ROCKSDB_LIB} ${TSB_SNAPPY_LIB} ${TSB_SQLPARSER_LIB} Shlwapi Rpcrt4)
    else()
        set(TSB_LIBRARIES ${TSB_LIB_} ${TSB_ROCKSDB_LIB} ${TSB_SNAPPY_LIB} ${TSB_SQLPARSER_LIB})
    endif()

    add_definitions( -DWITH_TSBDEPS )
    message("------------------- Tsb --------------------------")
    message("Tsb_FOUND:             ${TSB_FOUND}")
    message("TSB_INCLUDE_DIRS:      ${TSB_INCLUDE_DIRS}")
    message("TSB_LIBRARIES:         ${TSB_LIBRARIES}")
    #-------------------------------------------------------------
endif()

