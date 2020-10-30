#生成jce文件的函数
macro(gen_jce_files TARGET JCE_OUT)
	#jce文件处理
	file(GLOB_RECURSE JCE_INPUT ${LIB_DIR}/*.jce)
	set(JCE2CPP_FLAG "--json")
	set(JCE2CPP_INCULDE --include=${LIB_DIR})
	set(JCE_GEN_DIR ${CMAKE_CURRENT_SOURCE_DIR})
#	message(STATUS "TARGET = ${TARGET}")
#	message(STATUS "JCE2CPP_FLAG = ${JCE2CPP_FLAG} ${JCE2CPP_INCULDE}")

	SET(jce_list "")

	foreach(JCE_FILE ${JCE_INPUT})
		get_filename_component(JCE_NAME ${JCE_FILE} NAME_WE)

		file(STRINGS ${JCE_FILE} file_string)
#		message("file_string = "  ${JCE_FILE}  ${file_string} )

		#根据Jce文件中是否有 interface 判断是否需要生成cpp文件
		string(REGEX MATCH "interface( +)[A-Za-z]" interface_name "${file_string}" )
		string(LENGTH "${interface_name}" interface_len)
		#message("interface_name = [${JCE_NAME}] [${interface_name}] [${interface_len}]")

		if( "${interface_name}" STREQUAL "")
			#			message("${JCE_FILE} have no interface ..")
			set(CUR_JCE_GEN
					${PROTO_DIR}/proto/${JCE_NAME}.h
					)
		else ()
			#			message("${JCE_FILE} have interface ..")
			set(CUR_JCE_GEN
					${PROTO_DIR}/proto/${JCE_NAME}.h
					${PROTO_DIR}/proto/${JCE_NAME}.cpp
					)
		endif()

		set(JCE_GEN
				${JCE_GEN}
				${CUR_JCE_GEN}
				)

		LIST(APPEND jce_list ${CUR_JCE_GEN})
		#message(STATUS "JCE_OUT = ${JCE_OUT}")

		add_custom_command(
				OUTPUT ${CUR_JCE_GEN}
				WORKING_DIRECTORY ${JCE_GEN_DIR}
				COMMAND ${JCE2CPP} ${JCE_FILE} ${JCE2CPP_FLAG}  ${JCE2CPP_INCULDE} --dir=${PROTO_DIR}/proto
				DEPENDS ${JCE2CPP} ${JCE_FILE}
		)

	endforeach(JCE_FILE ${JCE_INPUT})
	add_custom_target(${TARGET} ALL DEPENDS ${JCE_GEN})

	SET(${JCE_OUT} ${jce_list})
	#message(STATUS "jce_list = ${jce_list}")

endmacro()


#生成带jce文件的可执行程序
#macro(gen_server TARGET JCE_OUT )
#	#jce文件处理
#	file(GLOB_RECURSE JCE_INPUT *.jce)
#	set(JCE2CPP_FLAG "--json")
#	set(JCE2CPP_INCULDE --include=${LIB_DIR})
#	set(JCE_GEN_DIR ${CMAKE_CURRENT_SOURCE_DIR})
#	message(STATUS "TARGET = ${TARGET}")
#	SET(jce_list "")
#
#	include_directories(.)
#	#aux_source_directory(. SRC_FILES  )
#	FILE(GLOB_RECURSE SRC_FILES  "*.cc" "*.cpp" )
#	if (JCE_INPUT)
#		#message(STATUS "SRC_FILES = ${SRC_FILES}")
#		foreach(JCE_FILE ${JCE_INPUT})
#			get_filename_component(JCE_NAME ${JCE_FILE} NAME_WE)
#			file(STRINGS ${JCE_FILE} file_string)
#			#message("file_string = "  ${JCE_FILE}  ${file_string} )
#
#			#根据Jce文件中是否有 interface 判断是否需要生成cpp文件
#			string(REGEX MATCH "interface [A-Za-z]*" interface_name ${file_string} )
#			string(LENGTH "${interface_name}" interface_len)
#			#message("interface_name = [${JCE_NAME}] [${interface_name}] [${interface_len}]")
#
#			if( "${interface_name}" STREQUAL "")
#				#				message("${JCE_FILE} have no interface ..")
#				set(CUR_JCE_GEN
#						${PROTO_DIR}/proto/${JCE_NAME}.h
#						)
#			else ()
#				#				message("${JCE_FILE} have interface ..")
#				set(CUR_JCE_GEN
#						${PROTO_DIR}/proto/${JCE_NAME}.h
#						${PROTO_DIR}/proto/${JCE_NAME}.cpp
#						)
#			endif()
#
#			set(JCE_GEN
#					${JCE_GEN}
#					${CUR_JCE_GEN}
#					)
#
#			LIST(APPEND jce_list ${CUR_JCE_GEN})
#			#list(APPEND ${JCE_OUT} ${CUR_JCE_GEN_WITH_CPP} )
#			message(STATUS "jce_list = ${jce_list}")
#
#			add_custom_command(
#					OUTPUT ${CUR_JCE_GEN}
#					WORKING_DIRECTORY ${JCE_GEN_DIR}
#					COMMAND ${JCE2CPP} ${JCE_FILE} ${JCE2CPP_FLAG} ${JCE2CPP_INCULDE} --dir=${PROTO_DIR}/proto
#					DEPENDS ${JCE2CPP} ${JCE_FILE}
#			)
#
#		endforeach(JCE_FILE ${JCE_INPUT})
#		set(JCE_TARGET "jce_${TARGET}")
#		add_custom_target(${JCE_TARGET} ALL DEPENDS ${CUR_JCE_GEN})
#
#		add_executable(${TARGET} ${SRC_FILES} ${jce_list})
#		add_dependencies(${TARGET} ${JCE_TARGET})
#
#		SET(${JCE_OUT} ${jce_list})
#	else ()
#		#链接库
#		add_executable(${TARGET} ${SRC_FILES} )
#	endif ()
#	#do performance test,link profiler &unwind lib
#	#set(LIB_PROFILER -lprofiler -lunwind)
#	#target_link_libraries(${TARGET}  ${LIB_ALGO}  ${LIB_TAF} ${LIB_PTHREAD} ${LIB_PROFILER})
#	target_link_libraries(${TARGET} ${ALL_DEPEND_LIBS})
#	install (TARGETS ${TARGET} RUNTIME DESTINATION ${BIN_INSTALL})
#endmacro()

macro(get_timestamp _commit_time _build_time _version_time)
	find_package(Git QUIET)     # 查找Git，QUIET静默方式不报错
	set(RET 0)
	if (GIT_FOUND)
		execute_process(          # 执行一个子进程
				COMMAND ${GIT_EXECUTABLE} log -1 --format=%cd --date=format:%Y%m%d%H%M%S
				RESULT_VARIABLE RET                 # 返回值存入变量
				OUTPUT_VARIABLE ${_commit_time}     # 输出字符串存入变量
				OUTPUT_STRIP_TRAILING_WHITESPACE    # 删除字符串尾的换行符
				WORKING_DIRECTORY                   # 执行路径
				${CMAKE_CURRENT_SOURCE_DIR}
				)
	endif ()

	string(TIMESTAMP ${_build_time} "%Y%m%d%H%M%S") # 获取生成时间

	if (NOT RET EQUAL 0)
		message(AUTHOR_WARNING "can't find .git, use build date instead of commit date!!!")
		set(${_version_time} ${${_build_time}})
	else ()
		set(${_version_time} ${${_commit_time}})
	endif ()
endmacro()

macro(get_commit _commit_hash _commit_number)
	find_package(Git QUIET)     # 查找Git，QUIET静默方式不报错
	set(RET 0)
	set(${_commit_hash} 999999)
	set(${_commit_number} 999999)
	if (GIT_FOUND)
		execute_process(          # 执行一个子进程
				COMMAND ${GIT_EXECUTABLE} log -1 --pretty=format:%h
				RESULT_VARIABLE RET                 # 返回值存入变量
				OUTPUT_VARIABLE ${_commit_hash}     # 输出字符串存入变量
				OUTPUT_STRIP_TRAILING_WHITESPACE    # 删除字符串尾的换行符
				WORKING_DIRECTORY                   # 执行路径
				${CMAKE_CURRENT_SOURCE_DIR}
				)

		execute_process(          # 执行一个子进程
				COMMAND ${GIT_EXECUTABLE} rev-list HEAD --count
				RESULT_VARIABLE RET                 # 返回值存入变量
				OUTPUT_VARIABLE ${_commit_number}     # 输出字符串存入变量
				OUTPUT_STRIP_TRAILING_WHITESPACE    # 删除字符串尾的换行符
				WORKING_DIRECTORY                   # 执行路径
				${CMAKE_CURRENT_SOURCE_DIR}
				)
	endif ()
endmacro()

# get git hash
macro(get_git_hash _git_hash)   # 宏的开始
	find_package(Git QUIET)     # 查找Git，QUIET静默方式不报错
	if (GIT_FOUND)
		execute_process(          # 执行一个子进程
				COMMAND ${GIT_EXECUTABLE} log -1 --pretty=format:%h # 命令
				OUTPUT_VARIABLE ${_git_hash}        # 输出字符串存入变量
				OUTPUT_STRIP_TRAILING_WHITESPACE    # 删除字符串尾的换行符
				ERROR_QUIET                         # 对执行错误静默
				WORKING_DIRECTORY                   # 执行路径
				${CMAKE_CURRENT_SOURCE_DIR}
				)
	endif ()
endmacro()

# get git branch
macro(get_git_branch _git_branch)   # 宏的开始
	find_package(Git QUIET)     # 查找Git，QUIET静默方式不报错
	if (GIT_FOUND)
		execute_process(          # 执行一个子进程
				COMMAND ${GIT_EXECUTABLE} symbolic-ref --short -q HEAD
				OUTPUT_VARIABLE ${_git_branch}        # 输出字符串存入变量
				OUTPUT_STRIP_TRAILING_WHITESPACE    # 删除字符串尾的换行符
				ERROR_QUIET                         # 对执行错误静默
				WORKING_DIRECTORY                   # 执行路径
				${CMAKE_CURRENT_SOURCE_DIR}
				)
	endif ()
endmacro()

function(enable_warnings target_name)
	if(MSVC)
		target_compile_options(${target_name} PRIVATE /W4)
	elseif(CMAKE_CXX_COMPILER_ID MATCHES "(GNU|Intel|Clang)")
		target_compile_options(${target_name} PRIVATE -w -Wextra -Wconversion -Wcast-qual
				-Wdeprecated)
	endif()

	if(PYBIND11_WERROR)
		if(MSVC)
			target_compile_options(${target_name} PRIVATE /WX)
		elseif(CMAKE_CXX_COMPILER_ID MATCHES "(GNU|Intel|Clang)")
			target_compile_options(${target_name} PRIVATE -Werror)
		endif()
	endif()

	# Needs to be readded since the ordering requires these to be after the ones above
	if(CMAKE_CXX_STANDARD
			AND CMAKE_CXX_COMPILER_ID MATCHES "Clang"
			AND PYTHON_VERSION VERSION_LESS 3.0)
		if(CMAKE_CXX_STANDARD LESS 17)
			target_compile_options(${target_name} PUBLIC -Wno-deprecated-register)
		else()
			target_compile_options(${target_name} PUBLIC -Wno-register)
		endif()
	endif()
endfunction()