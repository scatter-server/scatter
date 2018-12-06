# Threads
find_package(Threads REQUIRED)

# Boost
set(Boost_DEBUG OFF)
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
find_package(Boost 1.54.0 COMPONENTS system thread random filesystem REQUIRED)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
	find_package(Boost 1.54.0 COMPONENTS regex REQUIRED)
endif ()


# fmt
add_subdirectory(${PROJECT_LIBS_DIR}/fmt)


# OpenSSL (libssl/libcrypto)
string(TOLOWER ${CMAKE_SYSTEM_NAME} SYSTEM_LOWER)
set(OPENSSL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/openssl/${SYSTEM_LOWER}_${CMAKE_SYSTEM_PROCESSOR}")
set(OPENSSL_USE_STATIC_LIBS ON)
find_package(OpenSSL 1.1.0 REQUIRED)


# ToolBox++
add_subdirectory(${PROJECT_LIBS_DIR}/toolboxpp)
set_target_properties(
	toolboxpp PROPERTIES
	ENABLE_STATIC ON
)

# Json nlohmann
set(JSON_BuildTests OFF CACHE BOOL "Build json test" FORCE)
add_subdirectory(${PROJECT_LIBS_DIR}/json)

# Thread
find_package(Threads REQUIRED)


# cURL
if (CURL_ROOT_PATH)
	message(STATUS "Curl search path: ${CURL_ROOT_PATH}")
	find_library(CURL_LIBRARIES libcurl.a curl PATHS "${CURL_ROOT_PATH}" NO_DEFAULT_PATH)
	if (CURL_LIBRARIES-NOTFOUND)
		message(FATAL_ERROR "Curl not found in ${CURL_ROOT_PATH}/lib")
	endif ()
	set(CURL_VERSION_STRING "<unknown>")
	set(CURL_INCLUDE_DIRS ${CURL_ROOT_PATH}/include)
elseif (CURL_STATIC)
	find_library(CURL_LIBRARIES libcurl.a curl)
	if (CURL_LIBRARIES-NOTFOUND)
		message(FATAL_ERROR "Curl not found")
	endif ()
	set(CURL_VERSION_STRING "<unknown>")
	set(CURL_INCLUDE_DIRS ${CURL_ROOT_PATH}/include)
else ()
	find_package(CURL 7.26.0 REQUIRED)
endif ()


# date lib
add_subdirectory(${PROJECT_LIBS_DIR}/date)
target_compile_options(tz PRIVATE -Wno-deprecated-declarations)
set(USE_SYSTEM_TZ_DB ON CACHE BOOL "Enable system timezone DB" FORCE)
set_target_properties(
	tz PROPERTIES
	USE_SYSTEM_TZ_DB On
	BUILD_SHARED_LIBS Off
)


if (ENABLE_REDIS_TARGET)
	add_definitions(-DENABLE_REDIS_TARGET)
	# Redis client
	add_subdirectory(libs/cpp_redis)
	set_target_properties(
		cpp_redis PROPERTIES
		LOGGING_ENABLED Off
		USE_CUSTOM_TCP_CLIENT Off
		BUILD_EXAMPLES Off
		BUILD_TESTS Off
	)
endif ()


#set(LINK_LIBS "")
#macro(add_lib libname)
#	#    find_library(TEST_LIB_${libname} ${libname})
#	add_library(${libname} STATIC IMPORTED)
#
#	set(LIB_FILE_SUFFIX_${libname} ".so")
#	if (${ARGC} GREATER 1)
#		set(LIB_FILE_SUFFIX_${libname} ${ARGV1})
#	endif ()
#
#	set_target_properties(${libname} PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/lib${libname}${LIB_FILE_SUFFIX_${libname}})
#	set(TEST_LIB_${libname} ${PROJECT_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/lib${libname}${LIB_FILE_SUFFIX_${libname}})
#
#	string(TOUPPER "lib${libname}" DEF_NAME_${libname}_UPPER)
#	string(REPLACE "-" "_" DEF_NAME_${libname} ${DEF_NAME_${libname}_UPPER})
#	add_definitions(-DHAVE_${DEF_NAME_${libname}}=1)
#	set(HAVE_${DEF_NAME_${libname}} 1)
#	message(STATUS "Found: lib${libname} – ${TEST_LIB_${libname}}")
#	#		target_link_libraries(${PROJECT_NAME} ${libname})
#	list(APPEND LINK_LIBS ${libname})
#endmacro()

include(CMakeParseArguments)
function (linkdeps)
	cmake_parse_arguments(LINK_LIBS "" "TARGET" "DEPENDENCIES" ${ARGN})
	set(DEPS_PROJECT ${LINK_LIBS_TARGET})
	message(STATUS "Link libraries to target \"${DEPS_PROJECT}\":")
	#		message("${LINK_LIBS_TARGET}")
	#		message(${LINK_LIBS_DEPENDENCIES})

	list(TRANSFORM LINK_LIBS_DEPENDENCIES TOLOWER OUTPUT_VARIABLE LIBS_REQ)
	list(FIND LIBS_REQ "all" HAVE_ALL)
	list(FIND LIBS_REQ "threads" HAVE_THREADS)
	list(FIND LIBS_REQ "boost" HAVE_BOOST)
	list(FIND LIBS_REQ "openssl" HAVE_OPENSSL)
	list(FIND LIBS_REQ "toolbox" HAVE_TOOLBOX)
	list(FIND LIBS_REQ "json" HAVE_JSON)
	list(FIND LIBS_REQ "curl" HAVE_CURL)
	list(FIND LIBS_REQ "date" HAVE_DATE)
	list(FIND LIBS_REQ "redis" HAVE_REDIS)
	list(FIND LIBS_REQ "fmt" HAVE_FMT)

	set(LINK_ALL "0")
	if ("${HAVE_ALL}" GREATER -1)
		set(LINK_ALL "1")
	endif ()

	if ("${HAVE_THREADS}" GREATER -1 OR ${LINK_ALL})
		# Threads @TODO boost-only threads
		target_link_libraries(${DEPS_PROJECT} ${CMAKE_THREAD_LIBS_INIT})
		message(STATUS "\t- threads")
	endif ()

	if ("${HAVE_BOOST}" GREATER -1 OR ${LINK_ALL})
		# Boost
		target_link_libraries(${DEPS_PROJECT} ${Boost_LIBRARIES})
		target_include_directories(${DEPS_PROJECT} PUBLIC ${Boost_INCLUDE_DIR})
		#	message(STATUS "Boost includes: ${Boost_INCLUDE_DIR}; libs: ${Boost_LIBRARIES}")
		if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
			target_compile_definitions(${DEPS_PROJECT} INTERFACE USE_BOOST_REGEX)
			target_link_libraries(${DEPS_PROJECT} ${Boost_LIBRARIES})
			target_include_directories(${DEPS_PROJECT} PUBLIC ${Boost_INCLUDE_DIR})
		endif ()
		message(STATUS "\t- boost")
	endif ()

	if ("${HAVE_OPENSSL}" GREATER -1 OR ${LINK_ALL})
		# OpenSSL
		target_link_libraries(${DEPS_PROJECT} ${OPENSSL_LIBRARIES})
		target_include_directories(${DEPS_PROJECT} PUBLIC ${OPENSSL_INCLUDE_DIR})
		message(STATUS "\t- openssl ${OPENSSL_VERSION} (${OPENSSL_LIBRARIES})")
	endif ()

	if ("${HAVE_TOOLBOX}" GREATER -1 OR ${LINK_ALL})
		# Toolbox++
		target_link_libraries(${DEPS_PROJECT} toolboxpp)
		target_include_directories(${DEPS_PROJECT} PUBLIC ${PROJECT_LIBS_DIR}/toolboxpp/include)
		message(STATUS "\t- toolbox++")
	endif ()

	if ("${HAVE_JSON}" GREATER -1 OR ${LINK_ALL})
		# JSON
		target_link_libraries(${DEPS_PROJECT} nlohmann_json)
		target_include_directories(${DEPS_PROJECT} PUBLIC ${PROJECT_LIBS_DIR}/json/include/nlohmann)
		target_include_directories(${DEPS_PROJECT} PUBLIC ${PROJECT_LIBS_DIR}/json/include)
		message(STATUS "\t- JSON")
	endif ()

	if ("${HAVE_CURL}" GREATER -1 OR ${LINK_ALL})
		# CURL
		target_link_libraries(${DEPS_PROJECT} ${CURL_LIBRARIES})
		target_include_directories(${DEPS_PROJECT} PUBLIC ${CURL_INCLUDE_DIRS})
		message(STATUS "\t- curl ${CURL_VERSION_STRING} (${CURL_LIBRARIES})")
	endif ()

	if ("${HAVE_FMT}" GREATER -1 OR ${LINK_ALL})
		# FMT
		target_link_libraries(${DEPS_PROJECT} fmt::fmt)
		target_include_directories(${DEPS_PROJECT} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libs/fmt)
		message(STATUS "\t- fmt")
	endif ()

	if ("${HAVE_DATE}" GREATER -1 OR ${LINK_ALL})
		# Date lib
		target_link_libraries(${DEPS_PROJECT} tz)
		target_include_directories(${DEPS_PROJECT} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libs/date/include)
		message(STATUS "\t- date & timezone lib")
	endif ()

	if ("${HAVE_REDIS}" GREATER -1 OR ${LINK_ALL})
		if (ENABLE_REDIS_TARGET)
			# Redis (cpp_redis)
			target_link_libraries(${DEPS_PROJECT} cpp_redis)
			target_include_directories(${DEPS_PROJECT} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libs/cpp_redis/includes)
			message(STATUS "\t- cpp_redis")
		endif ()
	endif ()
endfunction ()