# thirdparty
if (UNIX)
    set(THIRDPARTY_DIR /usr/local/thirdparty)
else ()
    set(THIRDPARTY_DIR C:/thirdparty)
endif ()

if (IS_DIRECTORY ${THIRDPARTY_DIR}/cmake)
    set(THIRD_TARGET "")
else ()
    ExternalProject_Add(thirdparty
            URL http://gitlab.whup.com/tafdev/thirdparty/-/archive/master/thirdparty-master.tar.gz
            SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/thirdparty
            CMAKE_ARGS -Wno-dev
            CMAKE_GENERATOR "${CMAKE_GENERATOR}"
            LOG_BUILD 0
            INSTALL_DIR ${THIRDPARTY_DIR}
            )
    set(THIRD_TARGET "thirdparty")
endif ()

#外部静态库
set(THIRD_LIB_PATH "${THIRDPARTY_DIR}/lib")
set(THIRD_PARTY_INC "${THIRDPARTY_DIR}/include")

IF (UNIX)
    set(LIB_PTHREAD -lpthread -ldl)
    IF (NOT APPLE)
        set(LIB_PTHREAD ${LIB_PTHREAD} -lrt)
    ENDIF ()
ELSE ()
    set(LIB_PTHREAD "")
ENDIF ()

message("----------------------------------------------------")
message("THIRDPARTY_DIR:            ${THIRDPARTY_DIR}")
message("THIRD_LIB_PATH:            ${THIRD_LIB_PATH}")
message("THIRD_PARTY_INC:           ${THIRD_PARTY_INC}")
message("LIB_PTHREAD:               ${LIB_PTHREAD}")

# tsb
find_package(Tsb)
if (TSB_FOUND)
    include_directories(${TSB_INCLUDE_DIRS})
endif ()

#gtest
find_package(GTest)
if (GTEST_FOUND)
    include_directories(${GTEST_INCLUDE_DIRS})
else ()
    message(FATAL_ERROR "gtest not found!!")
endif ()
#-------------------------------------------------------------

#jsoncpp
find_package(Jsoncpp)
if (JSONCPP_FOUND)
    include_directories(${JSONCPP_INCLUDE_DIRS})
else ()
    message(FATAL_ERROR "jsoncpp not found!!")
endif ()
#-------------------------------------------------------------

# event
find_package(Event)
if (EVENT_FOUND)
    include_directories(${EVENT_INCLUDE_DIRS})
else ()
    message(FATAL_ERROR "taf event not found!!")
endif ()
#-------------------------------------------------------------

# taf
find_package(Taf)
if (TAF_FOUND)
    include_directories(${TAF_INCLUDE_DIRS})
else ()
    message(FATAL_ERROR "taf not found!!")
endif ()
#-------------------------------------------------------------

# python
find_package(Python REQUIRED)

set(PYTHON_EXECUTABLE ${PYTHON_COMMAND})

add_library(pybind11_headers INTERFACE)
add_library(pybind11::pybind11_headers ALIAS pybind11_headers) # to match exported target
add_library(pybind11::headers ALIAS pybind11_headers) # easier to use/remember
include(cmake/modules/pybind11Common.cmake)
#-------------------------------------------------------------

set(ALL_DEPEND_LIBS ${GQUANTDATA_LIBRARIES} ${TSB_LIBRARIES} ${EVENT_LIBRARIES} ${TAF_LIBRARIES} ${JSONCPP_LIBRARIES} ${LIB_PTHREAD})

message("ALL_DEPEND_LIBS:           ${ALL_DEPEND_LIBS}")