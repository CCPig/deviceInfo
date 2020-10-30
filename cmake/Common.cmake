# 版本信息
set(GQUANT_MAJOR_VERSION 2)
set(GQUANT_MINOR_VERSION 2)
get_commit(GQUANT_COMMIT_HASH GQUANT_COMMIT_NUMBER)
set(GQUANT_PATCH_VERSION ${GQUANT_COMMIT_NUMBER})
set(GQUANT_VERSION ${GQUANT_MAJOR_VERSION}.${GQUANT_MINOR_VERSION}.${GQUANT_PATCH_VERSION}_${GQUANT_COMMIT_HASH})
set(GQUANT_PYTHON_VERSION ${GQUANT_MAJOR_VERSION}.${GQUANT_MINOR_VERSION}.${GQUANT_PATCH_VERSION})
get_timestamp(COMMIT_TIMESTAMP BUILD_TIMESTAMP VERSION_TIMESTAMP)

set(GIT_HASH "")
get_git_hash(GIT_HASH)

set(GIT_BRANCH "")
get_git_branch(GIT_BRANCH)

message("----------------------------------------------------")
message("GQUANT_MAJOR_VERSION:         ${GQUANT_MAJOR_VERSION}")
message("GQUANT_MINOR_VERSION:         ${GQUANT_MINOR_VERSION}")
message("GQUANT_PATCH_VERSION:         ${GQUANT_PATCH_VERSION}")
message("GQUANT_COMMIT_HASH:           ${GQUANT_COMMIT_HASH}")
message("GQUANT_VERSION:               ${GQUANT_VERSION}")
message("GQUANT_PYTHON_VERSION:        ${GQUANT_PYTHON_VERSION}")


# build type 默认release
set(CMAKE_BUILD_TYPE Release CACHE STRING "set build type to Release")
IF (CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release")
ENDIF ()

# sqlite使用此选项
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DSQLITE_DEFAULT_PAGE_SIZE=4096 -DSQLITE_DEFAULT_CACHE_SIZE=2000 -DSQLITE_THREADSAFE=1")

IF (UNIX)
    set(PLATFORM linux)
    add_definitions(" -fPIC -Wno-deprecated -fno-strict-aliasing")
    add_definitions("-Wno-builtin-macro-redefined -D__FILE__='\"$(notdir $(abspath $<))\"'")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -Wall -std=c++11 ")
    IF (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "/usr/local/eta" CACHE STRING "set install path" FORCE)
    ENDIF ()

    IF (APPLE)
        set(PLATFORM macos)
        set(CMAKE_MACOSX_RPATH 1)
        SET(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> Scr <TARGET> <LINK_FLAGS> <OBJECTS>")
        SET(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> Scr <TARGET> <LINK_FLAGS> <OBJECTS>")
        SET(CMAKE_C_ARCHIVE_FINISH "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
        SET(CMAKE_CXX_ARCHIVE_FINISH "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
    ENDIF ()

ELSEIF (WIN32)
    set(PLATFORM win32)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_CXX_FLAGS_DEBUG " ${CMAKE_CXX_FLAGS_DEBUG} /bigobj /Zi")
    endif ()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4101 /wd4244 /wd4996 /wd4091 /wd4503 /wd4267 /wd4800 /wd4251 /wd4217 /wd4251")
    IF (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "c:/eta" CACHE STRING "set install path" FORCE)
    ENDIF ()
ELSE ()
    MESSAGE(STATUS "================ ERROR: This platform is unsupported!!! ================")
ENDIF (UNIX)

#开启此命令选项，则远程日志会直接输出到stdout，cmake .. -DRELEASE=ON /  cmake .. -DRELEASE=OFF关闭
IF (VERSION)
    add_definitions(-DVERSION="${VERSION}" -DRLS_MODE)
ENDIF ()

# 编译的可执行程序输出目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 一些特殊路径
set(PROTO_DIR ${PROJECT_BINARY_DIR}/include)
set(LIB_DIR "${PROJECT_SOURCE_DIR}/lib")
set(PYTHON_DIR "${PROJECT_SOURCE_DIR}/sdk/python")
set(PYTHON_BUILD_DIR "${PROJECT_BINARY_DIR}/sdk/python")

# 现有target
# 静态库
set(LIB_QUANT_COMMON common)
set(LIB_DATA_CPP gquantdata)
set(LIB_ANALYSER_CPP strategy)

# 动态库
set(LIB_GQUANT_CPP gquant)
set(PY_PACKAGE_NAME _gquant_analyser)


# 脚本
set(GEN_PYBIND pybindx)
set(TAR_GQUANT_PY tar_gquant)
set(LIB_GQUANT_PY3 gquant_py3)
set(COPY_GQUANT gquant_copy)
set(WHL_GQUANT_SDK gquant_sdk)
set(TAR_STG_CPP tar_analyser)
set(TAR_DAT_CPP tar_data)
set(JCE_TARGET jce_all)

message("----------------------------------------------------")
message("CMAKE_BUILD_TYPE:          ${CMAKE_BUILD_TYPE}")
message("PLATFORM:                  ${PLATFORM}")
message("CMAKE_SOURCE_DIR:          ${CMAKE_SOURCE_DIR}")
message("CMAKE_BINARY_DIR:          ${CMAKE_BINARY_DIR}")
message("PROJECT_SOURCE_DIR:        ${PROJECT_SOURCE_DIR}")
message("CMAKE_INSTALL_PREFIX:      ${CMAKE_INSTALL_PREFIX}")
message("PROTO_DIR:                 ${PROTO_DIR}")
message("LIB_DIR:                   ${LIB_DIR}")
message("BIN:                       ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message("PYTHON_DIR:                ${PYTHON_DIR}")
#-------------------------------------------------------------