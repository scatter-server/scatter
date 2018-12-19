project(scatter-server)
cmake_minimum_required(VERSION 3.12)

set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(PROJECT_LIBS_DIR ${PROJECT_ROOT}/libs)

# c++14 - minimum, we're using generalized lambda capturing
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmakes/dependencies.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmakes/opts.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmakes/configure.cmake)
include_directories(src/public)

## Build type
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug")
endif ()

if (APPLE)
	add_definitions(-DSCATTER_LIB_SUFFIX="dylib")
elseif (UNIX)
	add_definitions(-DSCATTER_LIB_SUFFIX="so")
elseif (WIN32)
	add_definitions(-DSCATTER_LIB_SUFFIX="dll")
else ()
	message(FATAL_ERROR "Unsupported OS")
endif ()

add_definitions(-DSCATTER_VERSION="${PROJECT_VERSION}")
add_definitions(-DWS_BIN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/bin")

# ARCH
if (WITH_ARCH)
	set(PROJECT_ARCH ${WITH_ARCH})
else ()
	include(${PROJECT_ROOT}/cmakes/FindLinuxPlatform.cmake)
	if (IS_DEBIAN)
		if (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
			set(PROJECT_ARCH "amd64")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64")
		elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "unknown")
			set(PROJECT_ARCH "i386")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
		else ()
			set(PROJECT_ARCH ${CMAKE_SYSTEM_PROCESSOR})

			if (CMAKE_SYSTEM_PROCESSOR STREQUAL "amd64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
				set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64")
			else ()
				set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
			endif ()
		endif ()
	else ()
		set(PROJECT_ARCH ${CMAKE_SYSTEM_PROCESSOR})
		if (CMAKE_SYSTEM_PROCESSOR STREQUAL "amd64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64")
		else ()
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
		endif ()
	endif ()
endif ()
message(STATUS "Build arch: ${PROJECT_ARCH}")

# common
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -Wextra -fPIC ${CMAKE_CXX_FLAGS}")


set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# linux clang
if (NOT APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	set(CMAKE_CXX_FLAGS "-stdlib=libc++ -Wl,-rpath=${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_CXX_FLAGS} ")

	# MOJAVE
elseif (APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	set(CMAKE_MACOSX_RPATH 1)
	set(CMAKE_CXX_FLAGS "-stdlib=libc++ ${CMAKE_CXX_FLAGS}")
	set(CMAKE_CXX_LINKER_FLAGS "${CMAKE_CXX_LINKER_FLAGS} -std=c++14 -stdlib=libc++ -undefined dynamic_lookup")

	# apple clang
elseif (APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
	set(CMAKE_MACOSX_RPATH 1)
	set(CMAKE_CXX_FLAGS "-stdlib=libc++ ${CMAKE_CXX_FLAGS}")
	set(CMAKE_CXX_LINKER_FLAGS "${CMAKE_CXX_LINKER_FLAGS} -std=c++14 -stdlib=libc++ -undefined dynamic_lookup")

	# MS compiler
elseif (MSVC)
	message(STATUS "Build for Windows")
	add_compile_options(/W1)

	#gnu only
else ()
	set(CMAKE_CXX_FLAGS "-rdynamic ${CMAKE_CXX_FLAGS}    ") #-Wl,-rpath=${CMAKE_INSTALL_PREFIX}/lib
	set(CMAKE_CXX_LINKER_FLAGS "")
endif ()

set(WARN_FLAGS "-pedantic -Wall -Wno-padded -Wno-switch-enum -Wno-undef")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${CXX_FLAGS} -g3 -O0 ${WARN_FLAGS} -fno-omit-frame-pointer") #-fsanitize=thread -fno-omit-frame-pointer
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${CXX_FLAGS} -O3")


set(PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/public/scatter/StatusCode.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/public/scatter/ScatterCore.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/public/scatter/Target.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/public/scatter/utility.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/public/scatter/unid.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/public/scatter/Message.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/public/scatter/Auth.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/public/scatter/HttpClient.h)

add_subdirectory(src)
add_dependencies(scatter_core tz)

if (ENABLE_REDIS_TARGET)
	add_subdirectory(extensions/redis-target)
	add_dependencies(scatter_event_redis scatter_core)
endif ()

if (ENABLE_AMQP_TARGET)
	add_subdirectory(extensions/amqp-target)
	add_dependencies(scatter_event_amqp scatter_core)
endif ()


####################################
## Libevent
####################################
#
#find_library(LIBEVENT_LIB event)
#
##message("LIBEVENT: ${LIBEVENT_LIB}")
#add_executable(amqpcpp_libevent_example libs/amqp-cpp/examples/libevent.cpp)
#add_dependencies(amqpcpp_libevent_example amqpcpp)
#target_link_libraries(amqpcpp_libevent_example amqpcpp ${LIBEVENT_LIB} pthread dl)
#linkdeps(TARGET amqpcpp_libevent_example DEPENDENCIES openssl)
#target_include_directories(amqpcpp_libevent_example PUBLIC /usr/local/include)


