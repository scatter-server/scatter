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
if (NOT OPENSSL_ROOT_DIR)
	set(OPENSSL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/openssl/${SYSTEM_LOWER}_${CMAKE_SYSTEM_PROCESSOR}")
endif ()
set(OPENSSL_USE_STATIC_LIBS ON)
find_package(OpenSSL 1.1.0 REQUIRED)


# ToolBox++
add_subdirectory(${PROJECT_LIBS_DIR}/toolboxpp)
set_target_properties(
	toolboxpp PROPERTIES
	ENABLE_STATIC ON
)

# HTTB
add_subdirectory(${PROJECT_LIBS_DIR}/httb)

# Json nlohmann
set(JSON_BuildTests OFF CACHE BOOL "Build json test" FORCE)
add_subdirectory(${PROJECT_LIBS_DIR}/json)

# Thread
find_package(Threads REQUIRED)


# cURL
#if (CURL_ROOT_PATH)
#	message(STATUS "Curl search path: ${CURL_ROOT_PATH}")
#	find_library(CURL_LIBRARIES libcurl.a curl PATHS "${CURL_ROOT_PATH}" NO_DEFAULT_PATH)
#	if (CURL_LIBRARIES-NOTFOUND)
#		message(FATAL_ERROR "Curl not found in ${CURL_ROOT_PATH}/lib")
#	endif ()
#	set(CURL_VERSION_STRING "<unknown>")
#	set(CURL_INCLUDE_DIRS ${CURL_ROOT_PATH}/include)
#elseif (CURL_STATIC)
#	find_library(CURL_LIBRARIES libcurl.a curl)
#	if (CURL_LIBRARIES-NOTFOUND)
#		message(FATAL_ERROR "Curl not found")
#	endif ()
#	set(CURL_VERSION_STRING "<unknown>")
#	set(CURL_INCLUDE_DIRS ${CURL_ROOT_PATH}/include)
#else ()
#	find_package(CURL 7.26.0 REQUIRED)
#endif ()


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

function (linkdeps DEPS_PROJECT)
	message(STATUS "Link libraries to target \"${DEPS_PROJECT}\":")

	# Threads @TODO boost-only threads
	target_link_libraries(${DEPS_PROJECT} ${CMAKE_THREAD_LIBS_INIT})
	message(STATUS "\t- threads")

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

	# OpenSSL
	target_link_libraries(${DEPS_PROJECT} ${OPENSSL_LIBRARIES})
	target_include_directories(${DEPS_PROJECT} PUBLIC ${OPENSSL_INCLUDE_DIR})
	message(STATUS "\t- openssl ${OPENSSL_VERSION} (${OPENSSL_LIBRARIES})")

	# Toolbox++
	target_link_libraries(${DEPS_PROJECT} toolboxpp)
	target_include_directories(${DEPS_PROJECT} PUBLIC ${PROJECT_LIBS_DIR}/toolboxpp/include)
	message(STATUS "\t- toolbox++")

	# http client
	target_link_libraries(${DEPS_PROJECT} httb)
	target_include_directories(${DEPS_PROJECT} PUBLIC ${PROJECT_LIBS_DIR}/httb/include)
	message(STATUS "\t- httb")

	# JSON
	target_link_libraries(${DEPS_PROJECT} nlohmann_json)
	target_include_directories(${DEPS_PROJECT} PUBLIC ${PROJECT_LIBS_DIR}/json/include/nlohmann)
	target_include_directories(${DEPS_PROJECT} PUBLIC ${PROJECT_LIBS_DIR}/json/include)
	message(STATUS "\t- JSON")

	# CURL
	#	target_link_libraries(${DEPS_PROJECT} ${CURL_LIBRARIES})
	#	target_include_directories(${DEPS_PROJECT} PUBLIC ${CURL_INCLUDE_DIRS})
	#	message(STATUS "\t- curl ${CURL_VERSION_STRING} (${CURL_LIBRARIES})")

	# FMT
	target_link_libraries(${DEPS_PROJECT} fmt::fmt)
	target_include_directories(${DEPS_PROJECT} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libs/fmt)
	message(STATUS "\t- fmt")

	# Date lib
	target_link_libraries(${DEPS_PROJECT} tz)
	target_include_directories(${DEPS_PROJECT} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libs/date/include)
	message(STATUS "\t- date & timezone lib")

	# Threads
	target_link_libraries(${DEPS_PROJECT} ${CMAKE_THREAD_LIBS_INIT})
	message(STATUS "\t- threads")

	if (ENABLE_REDIS_TARGET)
		# Redis (cpp_redis)
		target_link_libraries(${DEPS_PROJECT} cpp_redis)
		target_include_directories(${DEPS_PROJECT} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libs/cpp_redis/includes)
		message(STATUS "\t- cpp_redis")
	endif ()
endfunction ()