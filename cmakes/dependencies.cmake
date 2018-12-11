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
add_subdirectory(${PROJECT_LIBS_DIR}/fmt EXCLUDE_FROM_ALL)
target_compile_options(fmt PUBLIC "-fPIC")


# OpenSSL (libssl/libcrypto)
string(TOLOWER ${CMAKE_SYSTEM_NAME} SYSTEM_LOWER)
set(OPENSSL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/openssl/${SYSTEM_LOWER}_${CMAKE_SYSTEM_PROCESSOR}")
set(OPENSSL_USE_STATIC_LIBS ON)
include_directories(${OPENSSL_ROOT_DIR}/include)
find_package(OpenSSL 1.1.0 REQUIRED)



# ToolBox++
add_subdirectory(${PROJECT_LIBS_DIR}/toolboxpp EXCLUDE_FROM_ALL)
target_compile_options(toolboxpp PUBLIC "-fPIC")
set_target_properties(
	toolboxpp PROPERTIES
	ENABLE_STATIC ON
)

# Json nlohmann
option(JSON_BuildTests "" OFF)
option(JSON_MultipleHeaders "" OFF)
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
option(USE_SYSTEM_TZ_DB "" ON)
option(DISABLE_STRING_VIEW "" ON)
option(ENABLE_DATE_TESTING "" OFF)
option(BUILD_SHARED_LIBS "" OFF)
add_subdirectory(${PROJECT_LIBS_DIR}/date EXCLUDE_FROM_ALL)
target_compile_options(tz PRIVATE -Wno-deprecated-declarations)
target_compile_options(tz PUBLIC "-fPIC")

# Redis client
add_subdirectory(${PROJECT_LIBS_DIR}/cpp_redis EXCLUDE_FROM_ALL)
set_target_properties(
	cpp_redis PROPERTIES
	LOGGING_ENABLED Off
	USE_CUSTOM_TCP_CLIENT Off
	BUILD_EXAMPLES Off
	BUILD_TESTS Off
)

# AMQP
option(AMQP-CPP_BUILD_SHARED "" OFF)
option(AMQP-CPP_LINUX_TCP "" ON)
option(AMQP-CPP_BUILD_EXAMPLES "" OFF)
add_subdirectory(${PROJECT_LIBS_DIR}/amqp-cpp EXCLUDE_FROM_ALL)
target_compile_options(amqpcpp PUBLIC "-fPIC")


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
	list(FIND LIBS_REQ "amqp" HAVE_AMQP)

	if ("${HAVE_THREADS}" GREATER -1)
		# Threads @TODO boost-only threads
		target_link_libraries(${DEPS_PROJECT} ${CMAKE_THREAD_LIBS_INIT})
		message(STATUS "\t- threads")
	endif ()

	if ("${HAVE_BOOST}" GREATER -1)
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

	if ("${HAVE_OPENSSL}" GREATER -1)
		# OpenSSL
		target_link_libraries(${DEPS_PROJECT} ${OPENSSL_LIBRARIES})
		target_include_directories(${DEPS_PROJECT} PUBLIC ${OPENSSL_INCLUDE_DIR})
		message(STATUS "\t- openssl ${OPENSSL_VERSION} (${OPENSSL_LIBRARIES})")
	endif ()

	if ("${HAVE_TOOLBOX}" GREATER -1)
		# Toolbox++
		target_link_libraries(${DEPS_PROJECT} toolboxpp)
		target_include_directories(${DEPS_PROJECT} PUBLIC ${PROJECT_LIBS_DIR}/toolboxpp/include)
		message(STATUS "\t- toolbox++")
	endif ()

	if ("${HAVE_JSON}" GREATER -1)
		# JSON
		target_link_libraries(${DEPS_PROJECT} nlohmann_json)
		target_include_directories(${DEPS_PROJECT} PRIVATE ${PROJECT_LIBS_DIR}/json/include)
		message(STATUS "\t- JSON")
	endif ()

	if ("${HAVE_CURL}" GREATER -1)
		# CURL
		target_link_libraries(${DEPS_PROJECT} ${CURL_LIBRARIES})
		target_include_directories(${DEPS_PROJECT} PUBLIC ${CURL_INCLUDE_DIRS})
		message(STATUS "\t- curl ${CURL_VERSION_STRING} (${CURL_LIBRARIES})")
	endif ()

	if ("${HAVE_FMT}" GREATER -1)
		# FMT
		target_link_libraries(${DEPS_PROJECT} fmt::fmt)
		target_include_directories(${DEPS_PROJECT} PUBLIC ${PROJECT_LIBS_DIR}/fmt)
		message(STATUS "\t- fmt")
	endif ()

	if ("${HAVE_DATE}" GREATER -1)
		# Date lib
		target_link_libraries(${DEPS_PROJECT} tz)
		target_link_libraries(${DEPS_PROJECT} date_interface)
		target_include_directories(${DEPS_PROJECT} PUBLIC ${PROJECT_LIBS_DIR}/date/include)
		message(STATUS "\t- date & timezone lib")
	endif ()

	if ("${HAVE_REDIS}" GREATER -1)
		# Redis (cpp_redis)
		target_link_libraries(${DEPS_PROJECT} cpp_redis)
		target_include_directories(${DEPS_PROJECT} PUBLIC ${PROJECT_LIBS_DIR}/cpp_redis/includes)
		message(STATUS "\t- cpp_redis")
	endif ()

	if ("${HAVE_AMQP}" GREATER -1)
		# AMQP (amqpcpp)
		target_link_libraries(${DEPS_PROJECT} amqpcpp)
		target_include_directories(${DEPS_PROJECT} PUBLIC ${PROJECT_LIBS_DIR}/amqp/include)
		message(STATUS "\t- AMQP")
	endif ()
endfunction ()