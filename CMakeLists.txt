cmake_minimum_required(VERSION 3.7)
project(wsserver_standalone)

# Configuration
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_COMPILER_FLAGS "-Werror -Wfatal-error -pedantic -Wall -Wextra -Wsign-conversion")
set(CMAKE_VERBOSE_MAKEFILE OFF)

set(COMMON_LIBS_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/json/json.hpp
    )
set(WS_COMMON_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/ws/crypto.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/ws/utility.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/ws/status_code.hpp
    )
set(WS_SERVER_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/ws/server_ws.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/ws/server_wss.hpp
    )

set(WS_CLIENT_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/ws/client_ws.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/ws/client_wss.hpp
    )

set(HTTP_COMMON_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/http/crypto.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/http/utility.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/http/status_code.hpp
    )

set(HTTP_SERVER_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/http/server_http.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/http/server_https.hpp
    )

set(HTTP_CLIENT_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/http/client_http.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/http/client_https.hpp
    )

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/ws)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/json)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/http)

add_subdirectory(src/server)
add_subdirectory(src/benchmark)



