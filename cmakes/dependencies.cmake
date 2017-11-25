function (linkdeps DEPS_PROJECT)
	message(STATUS "Link deps libraries for ${DEPS_PROJECT}")

	set(Boost_USE_MULTITHREADED ON)
	set(Boost_DEBUG OFF)
	# Threads
	find_package(Threads REQUIRED)
	target_link_libraries(${DEPS_PROJECT} ${CMAKE_THREAD_LIBS_INIT})

	# Boost
	set(Boost_USE_STATIC_LIBS OFF)
	set(Boost_USE_MULTITHREADED ON)
	find_package(Boost 1.54.0 COMPONENTS system thread coroutine context random REQUIRED)
	target_link_libraries(${DEPS_PROJECT} ${Boost_LIBRARIES})
	target_include_directories(${DEPS_PROJECT} PUBLIC ${Boost_INCLUDE_DIR})
	message(STATUS "Boost includes: ${Boost_INCLUDE_DIR}; libs: ${Boost_LIBRARIES}")
	if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
		target_compile_definitions(${DEPS_PROJECT} INTERFACE USE_BOOST_REGEX)
		find_package(Boost 1.54.0 COMPONENTS regex REQUIRED)
		target_link_libraries(${DEPS_PROJECT} ${Boost_LIBRARIES})
		target_include_directories(${DEPS_PROJECT} PUBLIC ${Boost_INCLUDE_DIR})
	endif ()

	# OpenSSL
	if (APPLE)
		set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl")
	endif ()
	find_package(OpenSSL REQUIRED)
	target_link_libraries(${DEPS_PROJECT} ${OPENSSL_LIBRARIES})
	target_include_directories(${DEPS_PROJECT} PUBLIC ${OPENSSL_INCLUDE_DIR})

	# Helper
	find_library(TOOLBOXPP_LIBRARIES NAMES toolboxpp)
	if (TOOLBOXPP_LIBRARIES-NOTFOUND)
		message(CRITICAL "ToolBox++ not found")
	endif ()
	target_link_libraries(${DEPS_PROJECT} ${TOOLBOXPP_LIBRARIES})

	# CURL
	find_package(CURL REQUIRED)
	target_link_libraries(${DEPS_PROJECT} ${CURL_LIBRARIES})
	target_include_directories(${DEPS_PROJECT} PUBLIC ${CURL_INCLUDE_DIRS})

endfunction ()