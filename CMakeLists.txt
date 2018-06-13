cmake_minimum_required(VERSION 3.0)
project(wsserver VERSION 1.1.0)
set(CMAKE_CXX_STANDARD 14)
set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(PROJECT_LIBS_DIR ${PROJECT_ROOT}/libs)

## Build type
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug")
endif ()

### Configuration
if (NOT CMAKE_INSTALL_PREFIX)
	set(CMAKE_INSTALL_PREFIX /usr)
endif ()
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")

include(cmakes/opts.cmake)
include(cmakes/configure.cmake)

# common
set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -Wextra -Wno-parentheses ${CMAKE_CXX_FLAGS}")

# linux clang
if (NOT APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	set(CMAKE_CXX_FLAGS "-stdlib=libc++ ${CMAKE_CXX_FLAGS} ")

# apple clang
elseif (APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
	set(CMAKE_CXX_FLAGS "-stdlib=libc++ ${CMAKE_CXX_FLAGS}")
	set(CMAKE_CXX_LINKER_FLAGS "${CMAKE_CXX_LINKER_FLAGS} -std=c++14 -stdlib=libc++ -undefined dynamic_lookup")

# MS compiler
elseif (MSVC)
	message(STATUS "Build for Windows")
	add_compile_options(/W1)

#gnu only
else ()
	set(CMAKE_CXX_FLAGS "-rdynamic ${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
	set(CMAKE_CXX_LINKER_FLAGS "")
endif ()


set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${CXX_FLAGS} -g3 -O0") #-fsanitize=thread -fno-omit-frame-pointer
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${CXX_FLAGS} -O3")

# ARCH
if (WITH_ARCH)
	set(PROJECT_ARCH ${WITH_ARCH})
else ()
	include(cmakes/FindLinuxPlatform.cmake)
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

add_definitions(-DWSSERVER_VERSION="${PROJECT_VERSION}")
add_definitions(-DWS_BIN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/bin")
if (ENABLE_SSL)
	add_definitions(-DUSE_SECURE_SERVER=1)
endif ()


### Dependencies
include(cmakes/sources_sets.cmake)
include(cmakes/dependencies.cmake)


add_executable(${PROJECT_NAME} src/main.cpp ${SERVER_EXEC_SRCS})

linkdeps(${PROJECT_NAME})
if (WITH_BENCHMARK)
	add_executable(wssbench
	               src/benchmark/main.cpp
	               ${COMMON_LIBS_SRC}
	               ${WS_COMMON_SRC} ${WS_CLIENT_SRC})

	linkdeps(wssbench)
endif ()

if (WITH_TEST)
	include(cmakes/testing.cmake)
endif ()


if ($ENV{USER} STREQUAL "root")
	include(cmakes/install.cmake)
else ()
	message(STATUS "INSTALL:
	To enable install command,
	please run script as root user,
	because binaries and configs must be installed into the system dirs.
	")
endif ()

