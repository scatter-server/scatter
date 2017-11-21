cmake_minimum_required(VERSION 3.7)
project(wsserver)


# Configuration
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_COMPILER_FLAGS "-Werror -Wfatal-error -pedantic -Wall -Wextra -Wsign-conversion")
set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_ROOT_MODULE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

if (MSVC)
	add_compile_options(/W1)
endif ()

include(cmakes/sources_sets.cmake)
include(cmakes/dependencies.cmake)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/ws)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/json)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/http)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/ini-parser/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/args)

add_definitions(-DWS_BIN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/bin")

# Sources
set(SERVER_SRC
    src/server/main.cpp
    src/server/chat/ChatServer.h
    src/server/chat/ChatServer.cpp
    src/server/defs.h
    src/server/chat/Message.h
    src/server/chat/Message.cpp
    src/server/BlockingQueue.hpp
    src/server/restapi/RestServer.cpp
    src/server/restapi/RestServer.h
    src/server/restapi/ChatRestApi.cpp
    src/server/restapi/ChatRestApi.h
    src/server/helpers/helpers.h
    src/server/helpers/helpers.cpp
    src/server/web/HttpClient.cpp
    src/server/web/HttpClient.h
    src/server/web/QueryBuilder.hpp
    src/server/event/EventNotifier.h src/server/event/PostbackTarget.cpp src/server/event/PostbackTarget.h src/server/event/EventConfig.h)

add_executable(${PROJECT_NAME}
               ${SERVER_SRC}
               ${COMMON_LIBS_SRC}
               ${WS_SERVER_SRC} ${WS_COMMON_SRC}
               ${HTTP_SERVER_SRC} ${HTTP_CLIENT_SRC} ${HTTP_COMMON_SRC}
               )


linkdeps(${PROJECT_NAME})

add_subdirectory(src/benchmark)



