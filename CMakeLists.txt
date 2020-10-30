CMAKE_MINIMUM_REQUIRED(VERSION 3.4)

if(WIN32)
    option(MT_MODE "default compile as md" ON)
    if(MT_MODE)
        set(CMAKE_USER_MAKE_RULES_OVERRIDE "${CMAKE_SOURCE_DIR}/cmake/modules/ChangToMT.cmake")
    endif(MT_MODE)
else()
    option(MT_MODE "default compile as md" OFF)
endif(WIN32)

project(device)
option(SSL_MODE "option for gmssl" ON)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 11)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules/")
set(CMAKE_CXX_STANDARD 11)

include(ExternalProject)
include(cmake/modules/tools.cmake)
include(cmake/modules/JsonParse.cmake)
include(cmake/Common.cmake)
include(cmake/ThirdParty.cmake)

if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP4")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP4")
endif(WIN32)


add_executable(test dev_info_get_unittest.cpp tc_device.cpp tc_device.h HardDriveSerialNumer.h HardDriveSerialNumer.cpp)