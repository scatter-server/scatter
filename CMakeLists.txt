cmake_minimum_required(VERSION 3.0)
project(wsserver VERSION 1.0.1)

# ARCH
if (WITH_ARCH)
	set(PROJECT_ARCH ${WITH_ARCH})
else ()
	include(cmakes/FindLinuxPlatform.cmake)
	if (IS_DEBIAN)
		if (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
			set(PROJECT_ARCH "amd64")
		elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "unknown")
			set(PROJECT_ARCH "i386")
		else ()
			set(PROJECT_ARCH ${CMAKE_SYSTEM_PROCESSOR})
		endif ()
	else ()
		set(PROJECT_ARCH ${CMAKE_SYSTEM_PROCESSOR})
	endif ()
endif ()
message(STATUS "Build arch: ${PROJECT_ARCH}")

## Build type
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug")
endif ()

### Configuration
set(CMAKE_ROOT_MODULE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

include(cmakes/configure.cmake)

# gnu
set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -Wno-unused-parameter ${CMAKE_CXX_FLAGS} -Wno-vla -Wno-vla-extension -Wno-c++1z-extensions")

# linux clang
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	set(CMAKE_CXX_FLAGS "-stdlib=libc++ ${CMAKE_CXX_FLAGS} ")
elseif (APPLE) # apple clang
	set(CMAKE_CXX_FLAGS "-stdlib=libc++ ${CMAKE_CXX_FLAGS}")
	set(CMAKE_CXX_LINKER_FLAGS "${CMAKE_CXX_LINKER_FLAGS} -std=c++14 -stdlib=libc++ -undefined dynamic_lookup")
elseif (MSVC) # visual studio
	message(STATUS "Build for Windows")
	add_compile_options(/W1)
else () #gnu only
	set(CMAKE_CXX_FLAGS "-rdynamic ${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
endif ()


set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${CXX_FLAGS} -Wextra -pedantic -O0") #-Werror
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${CXX_FLAGS} -O3")

option(USE_SSL "Certifacates required" OFF)

add_definitions(-DWS_BIN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/bin")

if (USE_SSL)
	add_definitions(-DUSE_SECURE_SERVER=1)
endif ()


### Dependencies

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
find_library(TOOLBOXPP_LIBRARIES NAMES toolboxpp)
if (TOOLBOXPP_LIBRARIES-NOTFOUND)
	message(CRITICAL "ToolBox++ not found")
endif ()

# cURL
find_package(CURL 7.26.0 REQUIRED)


include(cmakes/sources_sets.cmake)
include(cmakes/dependencies.cmake)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/ws)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/json/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/http)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/ini-parser/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/args)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/concurrentqueue)


### Sources
set(SERVER_SRC
    src/server/chat/ChatMessageServer.h
    src/server/chat/ChatMessageServer.cpp
    src/server/defs.h
    src/server/chat/Message.h
    src/server/chat/Message.cpp
    src/server/helpers/threadsafe.hpp
    src/server/restapi/RestServer.cpp
    src/server/restapi/RestServer.h
    src/server/restapi/ChatRestServer.cpp
    src/server/restapi/ChatRestServer.h
    src/server/helpers/helpers.h
    src/server/helpers/helpers.cpp
    src/server/web/HttpClient.cpp
    src/server/web/HttpClient.h
    src/server/event/EventNotifier.h
    src/server/event/PostbackTarget.cpp
    src/server/event/PostbackTarget.h
    src/server/event/EventTarget.hpp
    src/server/helpers/base64.cpp
    src/server/helpers/base64.h
    src/server/base/StandaloneService.h
    src/server/event/EventNotifier.cpp
    src/server/base/ServerStarter.cpp
    src/server/base/ServerStarter.h
    src/server/base/Settings.hpp
    src/server/base/Auth.h
    src/server/base/Auth.cpp
    src/server/chat/ConnectionStorage.cpp
    src/server/chat/ConnectionStorage.h
    src/server/chat/Statistics.cpp
    src/server/chat/Statistics.h
    src/server/base/unid.cpp
    src/server/base/unid.h
    )

set(SERVER_EXEC_SRCS
    ${SERVER_SRC}
    ${COMMON_LIBS_SRC}
    ${WS_SERVER_SRC} ${WS_COMMON_SRC}
    ${HTTP_SERVER_SRC} ${HTTP_CLIENT_SRC} ${HTTP_COMMON_SRC}
    )

add_subdirectory(libs/fmt/fmt)
add_executable(${PROJECT_NAME} src/server/main.cpp ${SERVER_EXEC_SRCS})

add_subdirectory(libs/hiredis)

linkdeps(${PROJECT_NAME})
if (WITH_BENCHMARK)
	add_subdirectory(src/benchmark)
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

