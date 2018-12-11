cmake_minimum_required(VERSION 3.0)
project(scatter
        VERSION 2.0.0
        LANGUAGES CXX
        )

set(CMAKE_PROJECT_VERSION 2.0.0)
set(CMAKE_PROJECT_VERSION_MAJOR 2)
set(CMAKE_PROJECT_VERSION_MINOR 0)
set(CMAKE_PROJECT_VERSION_PATCH 0)
set(PROJECT_HOMEPAGE_URL "https://github.com/scatter")
set(PROJECT_DESCRIPTION "Free and Open Source WebSocket standalone message server")


set(CMAKE_CXX_STANDARD 14)


### Sources
set(SERVER_SRC
    ServerCore.h

    chat/ChatServer.h
    chat/ChatServer.cpp
    chat/ConnectionStorage.cpp
    chat/ConnectionStorage.h
    chat/Statistics.cpp
    chat/Statistics.h

    restapi/RestServer.cpp
    restapi/RestServer.h
    restapi/ChatRestServer.cpp
    restapi/ChatRestServer.h

    event/EventNotifier.h
    event/EventNotifier.cpp

    base/Settings.hpp
    base/StandaloneService.h
    base/BaseServer.h
    base/ServerStarter.h
    base/ServerStarter.cpp
    base/SocketLayerWrapper.hpp
    base/ws/WebsocketServer.hpp
    base/http/HttpServer.hpp
    )

set(SCATTER_CORE_LIB_SRCS
    event/PostbackTarget.h
    event/PostbackTarget.cpp
    base/unid.cpp
    chat/Message.cpp
    helpers/helpers.h
    helpers/helpers.cpp
    helpers/base64.cpp
    helpers/base64.h
    helpers/crypto.hpp
    base/auth/Auth.cpp
    base/auth/OneOfAuth.cpp
    base/auth/OneOfAuth.h
    base/auth/AllOfAuth.cpp
    base/auth/AllOfAuth.h
    base/auth/BasicAuth.cpp
    base/auth/BasicAuth.h
    base/auth/HeaderAuth.cpp
    base/auth/HeaderAuth.h
    base/auth/BearerAuth.cpp
    base/auth/BearerAuth.h
    base/auth/CookieAuth.cpp
    base/auth/CookieAuth.h
    base/auth/RemoteAuth.cpp
    base/auth/RemoteAuth.h
    web/HttpClient.cpp
    )

set(SCATTER_CORE_LIB_SRCS
    ${SCATTER_CORE_LIB_SRCS}
    ${PUBLIC_HEADERS})

add_library(${PROJECT_NAME}_core SHARED ${SCATTER_CORE_LIB_SRCS})
target_compile_options(${PROJECT_NAME}_core PUBLIC -fPIC)
linkdeps(TARGET ${PROJECT_NAME}_core DEPENDENCIES threads boost json fmt toolbox fmt openssl curl date)

set(COMMON_LIBS_SRC ${PROJECT_LIBS_DIR}/args/cmdline.hpp)
set(WS_CLIENT_SRC ${PROJECT_LIBS_DIR}/ws/client_ws.hpp ${PROJECT_LIBS_DIR}/ws/client_wss.hpp)
set(HTTP_COMMON_SRC ${PROJECT_LIBS_DIR}/http/crypto.hpp ${PROJECT_LIBS_DIR}/http/utility.hpp ${PROJECT_LIBS_DIR}/http/status_code.hpp)
set(HTTP_CLIENT_SRC ${PROJECT_LIBS_DIR}/http/client_http.hpp ${PROJECT_LIBS_DIR}/http/client_https.hpp)
set(LOCK_FREE_QUEUE_SRC ${PROJECT_LIBS_DIR}/concurrentqueue/concurrentqueue.h ${PROJECT_LIBS_DIR}/concurrentqueue/blockingconcurrentqueue.h)

include_directories(${PROJECT_LIBS_DIR}/json/src)
include_directories(${PROJECT_LIBS_DIR})
include_directories(${PROJECT_LIBS_DIR}/ini-parser/include)
include_directories(${PROJECT_LIBS_DIR}/args)
include_directories(${PROJECT_LIBS_DIR}/concurrentqueue)
include_directories(helpers)

set(SERVER_EXEC_SRCS
    ${SERVER_SRC}
    ${COMMON_LIBS_SRC})


add_executable(${PROJECT_NAME} main.cpp ${SERVER_EXEC_SRCS})
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADERS}")

if (APPLE)
	set_target_properties(${PROJECT_NAME} PROPERTIES FRAMEWORK ON)
endif ()

target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_core)

# required for openssl, it has undefined symbol `dlclose`
find_library(DL_LIBRARIES NAMES dl)
if (NOT DL_LIBRARIES)
	message(FATAL_ERROR "libdl not found!")
endif ()
target_link_libraries(${PROJECT_NAME} ${DL_LIBRARIES})

linkdeps(TARGET ${PROJECT_NAME} DEPENDENCIES threads boost openssl toolbox json fmt)


#if (WITH_BENCHMARK)
#	add_executable(bench
#	               benchmark/main.cpp
#	               ${COMMON_LIBS_SRC}
#	               ${WS_COMMON_SRC} ${WS_CLIENT_SRC})
#
#	target_include_directories(bench PUBLIC ${PROJECT_LIBS_DIR}/ws)
#	linkdeps(TARGET bench DEPENDENCIES all)
#endif ()

#if (WITH_TEST)
#	include(../cmakes/testing.cmake)
#endif ()


if ("$ENV{USER}" STREQUAL "root")
	include(${PROJECT_ROOT}/cmakes/install.cmake)
else ()
	include(${PROJECT_ROOT}/cmakes/install.cmake)
	message(WARNING "Installation can be incomplete, because you are not a root user.")
endif ()

