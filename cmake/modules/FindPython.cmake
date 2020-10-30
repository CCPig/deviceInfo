# handle anaconda dependencies
option(ANACONDA_PYTHON_VERBOSE "Anaconda dependency info" OFF)


# 默认选取python3.6
if (NOT PYTHON_MAJOR_VERSION)
    set(PYTHON_MAJOR_VERSION 3)
endif()

if (NOT PYTHON_MINOR_VERSION)
    set(PYTHON_MINOR_VERSION 6)
endif()

# 检查anaconda路径
if (NOT CMAKE_FIND_ANACONDA_PYTHON_INCLUDED)
    set(CMAKE_FIND_ANACONDA_PYTHON_INCLUDED 1)
    # find anaconda installation
    set(_cmd conda info --root)
    execute_process(
            COMMAND ${_cmd}
            RESULT_VARIABLE _r
            OUTPUT_VARIABLE _o
            ERROR_VARIABLE _e
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_STRIP_TRAILING_WHITESPACE
    )

    if (ANACONDA_PYTHON_VERBOSE)
        message("Executing conda info --root")
        message("_r = ${_r}")
        message("_o = ${_o}")
        message("_e = ${_e}")
    endif ()

    IF (IS_DIRECTORY ${_o})
        set(ANACONDA_PYTHON_FOUND True)
    endif ()

    if (ANACONDA_PYTHON_FOUND)
        set(ANACONDA_PYTHON_DIR ${_o})

        if (NOT DEFINED ENV{CONDA_DEFAULT_ENV})
            set(env_CONDA_DEFAULT_ENV "base")
            message("Could not find anaconda environment setting; using default base")
        else ()
            set(env_CONDA_DEFAULT_ENV $ENV{CONDA_DEFAULT_ENV})
        endif ()

        # find env
        set(_cmd conda env list --json)
        execute_process(
                COMMAND ${_cmd}
                RESULT_VARIABLE _r
                OUTPUT_VARIABLE _o
                ERROR_VARIABLE _e
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_STRIP_TRAILING_WHITESPACE
        )

        if (ANACONDA_PYTHON_VERBOSE)
            message("Executing conda env list --json")
            message("_r = ${_r}")
            message("_o = ${_o}")
            message("_e = ${_e}")
        endif ()

        sbeParseJson(PYTHON_LISTS _o)
        if (WIN32)
            foreach (var ${PYTHON_LISTS.envs})
                if (EXISTS ${PYTHON_LISTS.envs_${var}}/libs/python${PYTHON_MAJOR_VERSION}${PYTHON_MINOR_VERSION}.lib)
                    set(CONDA_ENV_PATH ${PYTHON_LISTS.envs_${var}})
                endif ()
            endforeach ()
        else ()
            foreach (var ${PYTHON_LISTS.envs})
                STRING(REGEX REPLACE ".+/(.+)$" "\\1" conda_name ${PYTHON_LISTS.envs_${var}})
                if (EXISTS ${PYTHON_LISTS.envs_${var}}/bin/python${PYTHON_MAJOR_VERSION}.${PYTHON_MINOR_VERSION})
                    set(CONDA_ENV_PATH ${PYTHON_LISTS.envs_${var}})
                endif ()
            endforeach ()
        endif ()

        if (CONDA_ENV_PATH)
            # find python version
            if (WIN32)
                set(_cmd "CONDA_ENV_PATH/python.exe" --version)
            else ()
                set(_cmd ${CONDA_ENV_PATH}/bin/python --version)
            endif ()

            execute_process(
                    COMMAND ${_cmd}
                    WORKING_DIRECTORY ${CONDA_ENV_PATH}
                    RESULT_VARIABLE _r
                    OUTPUT_VARIABLE _o
                    ERROR_VARIABLE _e
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_STRIP_TRAILING_WHITESPACE
            )

            if (ANACONDA_PYTHON_VERBOSE)
                message("Executing ${_cmd}")
                message("_r = ${_r}")
                message("_o = ${_o}")
                message("_e = ${_e}")
            endif ()

            if (_e STREQUAL "")
                string(REGEX MATCH "Python ([0-9]+)[.]([0-9]+)[.]([0-9]+)" _py_version_found "${_o}")
            else ()
                string(REGEX MATCH "Python ([0-9]+)[.]([0-9]+)[.]([0-9]+)" _py_version_found "${_e}")
            endif ()

            set(_py_version_major ${CMAKE_MATCH_1})
            set(_py_version_minor ${CMAKE_MATCH_2})
            set(_py_version_patch ${CMAKE_MATCH_3})
            set(ANACONDA_PYTHON_VERSION ${_py_version_major}.${_py_version_minor})

            if (${_py_version_major} MATCHES 2)
                set(_py_ext "")
            else ()
                set(_py_ext "m")
            endif ()

            if (WIN32)
                set(_py_id "python${ANACONDA_PYTHON_VERSION}")
                set(PYTHON_INCLUDE_DIRS "${CONDA_ENV_PATH}/include" CACHE INTERNAL "")
                set(PYTHON_LIBRARIES "${CONDA_ENV_PATH}/libs/python${PYTHON_MAJOR_VERSION}${PYTHON_MINOR_VERSION}.lib" CACHE INTERNAL "")
                set(PYTHON_COMMAND "${CONDA_ENV_PATH}/python" CACHE INTERNAL "")
                set(PIP_COMMAND "${CONDA_ENV_PATH}/Scripts/pip" CACHE INTERNAL "")
            else ()
                set(_py_id "python${ANACONDA_PYTHON_VERSION}${_py_ext}")
                set(PYTHON_INCLUDE_DIRS "${CONDA_ENV_PATH}/include/${_py_id}" CACHE INTERNAL "")
                set(PYTHON_LIBRARIES "${CONDA_ENV_PATH}/lib/lib${_py_id}${CMAKE_SHARED_LIBRARY_SUFFIX}" CACHE INTERNAL "")
                set(PYTHON_COMMAND "${CONDA_ENV_PATH}/bin/python" CACHE INTERNAL "")
                set(PIP_COMMAND "${CONDA_ENV_PATH}/bin/pip" CACHE INTERNAL "")
            endif ()

            set(PYTHONLIBS_FOUND TRUE)
        else ()
#            message("Not found: anaconda root directory...")
#            message("Trying system python install...")
            if (WIN32)
                set(PYTHON_INCLUDE_DIRS "D:/Anaconda3/envs/py36/include" CACHE INTERNAL "")
                set(PYTHON_LIBRARIES "D:/Anaconda3/envs/py36/libs/python3.lib" CACHE INTERNAL "")
                set(PYTHON_COMMAND "D:/Anaconda3/envs/py36/python" CACHE INTERNAL "")
                set(PIP_COMMAND "D:/Anaconda3/envs/py36/Scripts/pip" CACHE INTERNAL "")
                set(PYTHONLIBS_FOUND TRUE)
            else ()
                FindPythonLibs()
            endif ()
        endif ()

    else ()
#        message("Not found: anaconda root directory...")
#        message("Trying system python install...")
        if (WIN32)
            set(PYTHON_INCLUDE_DIRS "D:/Anaconda3/envs/py36/include" CACHE INTERNAL "")
            set(PYTHON_LIBRARIES "D:/Anaconda3/envs/py36/libs/python3.lib" CACHE INTERNAL "")
            set(PYTHON_COMMAND "D:/Anaconda3/envs/py36/python" CACHE INTERNAL "")
            set(PIP_COMMAND "D:/Anaconda3/envs/py36/Scripts/pip" CACHE INTERNAL "")
            set(PYTHONLIBS_FOUND TRUE)
        else ()
            FindPythonLibs()
        endif ()
    endif ()

    IF (UNIX)
        set(BOOST_PYTHON3 libboost_python${PYTHON_MAJOR_VERSION}${PYTHON_MINOR_VERSION}.a libboost_numpy${PYTHON_MAJOR_VERSION}${PYTHON_MINOR_VERSION}.a)
    ELSEIF (WIN32)

        IF (CMAKE_BUILD_TYPE MATCHES "Debug")
            if(MT_MODE)
                set(BOOST_DEBUG_FLAG "-sgd")
            else()
                set(BOOST_DEBUG_FLAG "-gd")
            endif(MT_MODE)
        ELSE()
            if(MT_MODE)
                set(BOOST_DEBUG_FLAG "-s")
            else()
                set(BOOST_DEBUG_FLAG "")
            endif(MT_MODE)
        ENDIF ()

        set(BOOST_PYTHON3 libboost_python${PYTHON_MAJOR_VERSION}${PYTHON_MINOR_VERSION}-vc140-mt${BOOST_DEBUG_FLAG}-x64-1_69.lib libboost_numpy${PYTHON_MAJOR_VERSION}${PYTHON_MINOR_VERSION}-vc140-mt${BOOST_DEBUG_FLAG}-x64-1_69.lib)
        
    ENDIF (UNIX)


    IF (APPLE)
        set(PYTHON_PLATFORM macosx_10_9_x86_64)
    ELSEIF (UNIX)
        set(PYTHON_PLATFORM linux_x86_64)
    ELSEIF (WIN32)
        set(PYTHON_PLATFORM win_amd64)
    ELSE()
        message(FATAL_ERROR "unsupport platform!")
    ENDIF (APPLE)

    if (PYTHONLIBS_FOUND)
        message("----------------------------------------------------")
        message("PYTHON_VERSION:            python${PYTHON_MAJOR_VERSION}.${PYTHON_MINOR_VERSION}")
        message("PYTHON_PLATFORM:           ${PYTHON_PLATFORM}")
        message("ANACONDA_PYTHON_DIR:       ${ANACONDA_PYTHON_DIR}")
        message("PYTHON_LIBRARIES:          ${PYTHON_LIBRARIES}")
        message("PYTHON_INCLUDE_DIRS:       ${PYTHON_INCLUDE_DIRS}")
        message("PYTHON_COMMAND:            ${PYTHON_COMMAND}")
        message("PIP_COMMAND:               ${PIP_COMMAND}")
        message("BOOST_PYTHON3:             ${BOOST_PYTHON3}")
    endif (PYTHONLIBS_FOUND)
endif ()