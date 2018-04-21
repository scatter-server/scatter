# Threads
find_package(Threads REQUIRED)

# Boost
set(Boost_DEBUG OFF)
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
find_package(Boost 1.54.0 COMPONENTS system thread coroutine context random REQUIRED)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
	find_package(Boost 1.54.0 COMPONENTS regex REQUIRED)
endif ()


# OpenSSL (libssl)
if (APPLE)
	set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl")
endif ()
find_package(OpenSSL 1.0.0 REQUIRED)


# ToolBox++
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libs/toolboxpp)
set_target_properties(
	toolboxpp PROPERTIES
	ENABLE_STATIC ON
)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/toolboxpp/include)

# cURL
find_package(CURL 7.26.0 REQUIRED)

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
	message(STATUS "\t- openssl (${OPENSSL_LIBRARIES})")

	# Toolbox++
	target_link_libraries(${DEPS_PROJECT} toolboxpp)
	message(STATUS "\t- toolbox++")

	# CURL
	target_link_libraries(${DEPS_PROJECT} ${CURL_LIBRARIES})
	target_include_directories(${DEPS_PROJECT} PUBLIC ${CURL_INCLUDE_DIRS})
	message(STATUS "\t- curl")

	# FMT
	target_link_libraries(${DEPS_PROJECT} fmt::fmt)
	target_include_directories(${DEPS_PROJECT} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libs/fmt)
	message(STATUS "\t- fmt")


	if (ENABLE_REDIS_TARGET)
		# Redis (cpp_redis)
		target_link_libraries(${DEPS_PROJECT} cpp_redis)
		target_include_directories(${DEPS_PROJECT} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libs/cpp_redis/includes)
		message(STATUS "\t- cpp_redis")
	endif ()
endfunction ()