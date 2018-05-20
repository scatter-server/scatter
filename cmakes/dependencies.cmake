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

# fmt
add_subdirectory(${PROJECT_LIBS_DIR}/fmt)


# OpenSSL (libssl)
if (APPLE AND NOT OPENSSL_ROOT_DIR)
	set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl")
endif ()
find_package(OpenSSL 1.0.0 REQUIRED)


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
find_package(CURL 7.26.0 REQUIRED)


# date lib
add_subdirectory(${PROJECT_LIBS_DIR}/date)


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
	target_include_directories(${DEPS_PROJECT} PUBLIC ${PROJECT_LIBS_DIR}/toolboxpp/include)
	message(STATUS "\t- toolbox++")

	# JSON
	target_link_libraries(${DEPS_PROJECT} nlohmann_json)
	target_include_directories(${DEPS_PROJECT} PUBLIC ${PROJECT_LIBS_DIR}/json/src)
	message(STATUS "\t- JSON")

	# CURL
	target_link_libraries(${DEPS_PROJECT} ${CURL_LIBRARIES})
	target_include_directories(${DEPS_PROJECT} PUBLIC ${CURL_INCLUDE_DIRS})
	message(STATUS "\t- curl")

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