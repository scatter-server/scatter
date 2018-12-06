cmake_minimum_required(VERSION 3.0)
project(scatter
        VERSION 2.0.0
        DESCRIPTION "Free and Open Source WebSocket standalone message server"
        HOMEPAGE_URL "https://github.com/scatter"
        LANGUAGES CXX
        )
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


### Dependencies
include(cmakes/dependencies.cmake)

### Sources
set(SERVER_SRC
    src/ServerCore.h

    src/chat/ChatServer.h
    src/chat/ChatServer.cpp
    src/chat/ConnectionStorage.cpp
    src/chat/ConnectionStorage.h
    src/chat/Statistics.cpp
    src/chat/Statistics.h

    src/restapi/RestServer.cpp
    src/restapi/RestServer.h
    src/restapi/ChatRestServer.cpp
    src/restapi/ChatRestServer.h

    src/event/EventNotifier.h
    src/event/EventNotifier.cpp

    src/base/Settings.hpp
    src/base/StandaloneService.h
    src/base/BaseServer.h
    src/base/ServerStarter.h
    src/base/ServerStarter.cpp
    src/base/SocketLayerWrapper.hpp
    src/base/ws/WebsocketServer.hpp
    src/base/http/HttpServer.hpp
    )

set(SCATTER_CORE_LIB_SRCS
    src/public/scatter/StatusCode.h
    src/public/scatter/ScatterCore.h

    src/public/scatter/Target.h
    src/event/PostbackTarget.h
    src/event/PostbackTarget.cpp

    src/public/scatter/utility.hpp
    src/public/scatter/unid.h
    src/base/unid.cpp
    src/public/scatter/Message.h
    src/chat/Message.cpp


    src/helpers/helpers.h
    src/helpers/helpers.cpp
    src/helpers/base64.cpp
    src/helpers/base64.h
    src/helpers/crypto.hpp

    src/public/scatter/Auth.h
    src/base/auth/Auth.cpp
    src/base/auth/OneOfAuth.cpp
    src/base/auth/OneOfAuth.h
    src/base/auth/AllOfAuth.cpp
    src/base/auth/AllOfAuth.h
    src/base/auth/BasicAuth.cpp
    src/base/auth/BasicAuth.h
    src/base/auth/HeaderAuth.cpp
    src/base/auth/HeaderAuth.h
    src/base/auth/BearerAuth.cpp
    src/base/auth/BearerAuth.h
    src/base/auth/CookieAuth.cpp
    src/base/auth/CookieAuth.h
    src/base/auth/RemoteAuth.cpp
    src/base/auth/RemoteAuth.h

    src/public/scatter/HttpClient.h
    src/web/HttpClient.cpp
    )

add_library(${PROJECT_NAME}_core SHARED ${SCATTER_CORE_LIB_SRCS})
linkdeps(TARGET ${PROJECT_NAME}_core DEPENDENCIES threads boost json fmt toolbox fmt openssl curl date)

if (ENABLE_REDIS_TARGET)
	set(EVENT_REDIS_TARGET_SRC
	    src/event/RedisTarget.cpp
	    src/event/RedisTarget.h)

	add_library(${PROJECT_NAME}_event_redis SHARED ${EVENT_REDIS_TARGET_SRC})
	linkdeps(TARGET ${PROJECT_NAME}_event_redis DEPENDENCIES redis json fmt)
	target_link_libraries(${PROJECT_NAME}_event_redis ${PROJECT_NAME}_core)
endif ()

set(COMMON_LIBS_SRC
    ${PROJECT_LIBS_DIR}/args/cmdline.hpp
    )

set(WS_CLIENT_SRC
    ${PROJECT_LIBS_DIR}/ws/client_ws.hpp
    ${PROJECT_LIBS_DIR}/ws/client_wss.hpp
    )

set(HTTP_COMMON_SRC
    ${PROJECT_LIBS_DIR}/http/crypto.hpp
    ${PROJECT_LIBS_DIR}/http/utility.hpp
    ${PROJECT_LIBS_DIR}/http/status_code.hpp
    )

set(HTTP_CLIENT_SRC
    ${PROJECT_LIBS_DIR}/http/client_http.hpp
    ${PROJECT_LIBS_DIR}/http/client_https.hpp
    )

set(LOCK_FREE_QUEUE_SRC
    ${PROJECT_LIBS_DIR}/concurrentqueue/concurrentqueue.h
    ${PROJECT_LIBS_DIR}/concurrentqueue/blockingconcurrentqueue.h
    )

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/public)
include_directories(${PROJECT_LIBS_DIR}/json/src)
include_directories(${PROJECT_LIBS_DIR})
include_directories(${PROJECT_LIBS_DIR}/ini-parser/include)
include_directories(${PROJECT_LIBS_DIR}/args)
include_directories(${PROJECT_LIBS_DIR}/concurrentqueue)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/helpers)

set(SERVER_EXEC_SRCS
    ${SERVER_SRC}
    ${COMMON_LIBS_SRC})


add_executable(${PROJECT_NAME} src/main.cpp ${SERVER_EXEC_SRCS})
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_core)

# required for openssl, it has undefined symbol `dlclose`
find_library(DL_LIBRARIES NAMES dl)
if (NOT DL_LIBRARIES)
	message(FATAL_ERROR "libdl not found!")
endif ()
target_link_libraries(${PROJECT_NAME} ${DL_LIBRARIES})

linkdeps(TARGET ${PROJECT_NAME} DEPENDENCIES all)
if (WITH_BENCHMARK)
	add_executable(bench
	               src/benchmark/main.cpp
	               ${COMMON_LIBS_SRC}
	               ${WS_COMMON_SRC} ${WS_CLIENT_SRC})

	target_include_directories(bench PUBLIC ${PROJECT_LIBS_DIR}/ws)
	linkdeps(TARGET bench DEPENDENCIES all)
endif ()

if (WITH_TEST)
	include(cmakes/testing.cmake)
endif ()


if ("$ENV{USER}" STREQUAL "root")
	include(cmakes/install.cmake)
else ()
	message(STATUS "INSTALL:
	To enable install command,
	please run script as root user,
	because binaries and configs must be installed into the system dirs.
	")
endif ()

