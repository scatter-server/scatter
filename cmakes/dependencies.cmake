# Threads
find_package(Threads REQUIRED)

# Json nlohmann
set(JSON_BuildTests OFF CACHE BOOL "Build json test" FORCE)
add_subdirectory(${PROJECT_LIBS_DIR}/json)


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
	message(STATUS "\t threads")

	# Boost
	target_link_libraries(${DEPS_PROJECT} CONAN_PKG::boost)
	message(STATUS "\t- boost")

	# OpenSSL
	target_link_libraries(${DEPS_PROJECT} CONAN_PKG::OpenSSL)
	message(STATUS "\t- openssl")

	# Toolbox++
	target_link_libraries(${DEPS_PROJECT} CONAN_PKG::toolboxpp)
	message(STATUS "\t- toolbox++")

	# http client
	target_link_libraries(${DEPS_PROJECT} CONAN_PKG::httb)
	message(STATUS "\t- httb")

	# JSON
	target_link_libraries(${DEPS_PROJECT} nlohmann_json)
	target_include_directories(${DEPS_PROJECT} PUBLIC ${PROJECT_LIBS_DIR}/json/include/nlohmann)
	target_include_directories(${DEPS_PROJECT} PUBLIC ${PROJECT_LIBS_DIR}/json/include)
	message(STATUS "\t- JSON")

	# FMT
	target_link_libraries(${DEPS_PROJECT} CONAN_PKG::fmt)
	message(STATUS "\t- fmt")

	# Date lib
	target_link_libraries(${DEPS_PROJECT} CONAN_PKG::date)
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