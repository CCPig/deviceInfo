# - Find taf
#
# -*- cmake -*-
#
# Find the native TAF includes and library
#
#  TAF_INCLUDE_DIR - where to find mysql.h, etc.
#  TAF_LIBRARIES   - List of libraries when using TAF.
#  TAF_FOUND       - True if TAF found.

IF (MT_MODE)
    set(Taf-Ext ${Taf-Ext}-mt)
ENDIF()

IF (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
    set(Taf-Ext ${Taf-Ext}.debug)
ENDIF ()

set(TAF_NAME "taf/cpp")

#IF (SSL_MODE)
#    #set(TAF_NAME "upgssl/taf/cpp")
#    set(TAF_NAME "taf/cpp")
#ENDIF()

set(TAF_FOUND_PATH /usr/local/${TAF_NAME}/ C:/${TAF_NAME}/)
set(TAF_TRD_FOUND_INC /usr/local/${TAF_NAME}/thirdparty/include C:/${TAF_NAME}/thirdparty/include/)
set(TAF_TRD_FOUND_LIB /usr/local/${TAF_NAME}/thirdparty/lib     C:/${TAF_NAME}/thirdparty/lib)

find_path(TAF_INCLUDE_DIR
        NAMES servant/taf_logger.h
        PATHS
        ${TAF_FOUND_PATH}
        PATH_SUFFIXES
        include)

find_path(TAF_TOOLS_DIR
        NAMES jce2cpp jce2cpp.exe
        PATHS
        ${TAF_FOUND_PATH}
        PATH_SUFFIXES
        tools)

find_library(_TAF_UTIL_LIB
        NAMES tafutil${Taf-Ext}
        PATHS
        ${TAF_FOUND_PATH}
        PATH_SUFFIXES
        lib)

find_library(_TAF_SERVANT_LIB
        NAMES tafservant${Taf-Ext}
        PATHS
        ${TAF_FOUND_PATH}
        PATH_SUFFIXES
        lib)

find_library(_PARSE_LIB
        NAMES parse${Taf-Ext}
        PATHS
        ${TAF_FOUND_PATH}
        PATH_SUFFIXES
        lib)

find_path(TAF_USE_SSL_INCLUDE_DIR
        NAMES openssl/ssl.h
        PATHS
        ${TAF_TRD_FOUND_INC}
        NO_DEFAULT_PATH
        PATH_SUFFIXES
        include)

find_library(TAF_USE_SSL_LIB
        NAMES ssl libssl
        PATHS
        ${TAF_TRD_FOUND_LIB}
        NO_DEFAULT_PATH
        PATH_SUFFIXES
        lib)

find_library(TAF_USE_CRYPTO_LIB
        NAMES crypto libcrypto
        PATHS
        ${TAF_TRD_FOUND_LIB}
        NO_DEFAULT_PATH
        PATH_SUFFIXES
        lib)


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Taf DEFAULT_MSG _TAF_UTIL_LIB _TAF_SERVANT_LIB _PARSE_LIB TAF_INCLUDE_DIR TAF_TOOLS_DIR)
IF (SSL_MODE)
    mark_as_advanced(TAF_INCLUDE_DIR TAF_TOOLS_DIR _TAF_UTIL_LIB _TAF_SERVANT_LIB _PARSE_LIB TAF_USE_SSL_LIB TAF_USE_CRYPTO_LIB)
elseif()
    mark_as_advanced(TAF_INCLUDE_DIR TAF_TOOLS_DIR _TAF_UTIL_LIB _TAF_SERVANT_LIB _PARSE_LIB)
endif()

if(Taf_FOUND)
    set(TAF_INCLUDE_DIRS ${TAF_INCLUDE_DIR})
    set(TAF_LIBRARIES ${_TAF_SERVANT_LIB} ${_TAF_UTIL_LIB} ${_PARSE_LIB})

    if (MSVC)
        set(JCE2CPP "${TAF_TOOLS_DIR}/jce2cpp.exe" CACHE INTERNAL "")
    else()
        set(JCE2CPP "${TAF_TOOLS_DIR}/jce2cpp" CACHE INTERNAL "")
    endif()

    if (TAF_USE_SSL_LIB STREQUAL TAF_USE_SSL_LIB-NOTFOUND)
    else()
        if(MSVC)
            set(TAF_LIBRARIES ${TAF_LIBRARIES} ${TAF_USE_SSL_LIB} ${TAF_USE_CRYPTO_LIB} Crypt32)
        else()
            set(TAF_LIBRARIES ${TAF_LIBRARIES} ${TAF_USE_SSL_LIB} ${TAF_USE_CRYPTO_LIB})
        endif()
    endif()

    add_definitions( -DWITH_TAF )
    message("------------------- TAF -------------------------------")
    message("Taf_FOUND:                 ${TAF_FOUND}")
    message("TAF_INCLUDE_DIRS:          ${TAF_INCLUDE_DIRS}")
    message("TAF_LIBRARIES:             ${TAF_LIBRARIES}")
    message("JCE2CPP:                   ${JCE2CPP}")
    if (TAF_USE_SSL_LIB STREQUAL TAF_USE_SSL_LIB-NOTFOUND)
    else()
    message("TAF_USE_SSL_LIB:           ${TAF_USE_SSL_LIB}")
    message("TAF_USE_CRYPTO_LIB:        ${TAF_USE_CRYPTO_LIB}")
    endif()
    message("------------------- TAF -------------------------------")
    #-------------------------------------------------------------
endif()

