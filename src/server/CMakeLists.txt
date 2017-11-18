cmake_minimum_required(VERSION 3.7)
project(wsserver)

if (MSVC)
	add_compile_options(/W1)
endif ()

# Sources
set(BIN_SRC
    main.cpp
    chat/ChatServer.h chat/ChatServer.cpp
    defs.h
    chat/Message.h chat/Message.cpp
    BlockingQueue.hpp
    restapi/RestServer.cpp
    restapi/RestServer.h
    restapi/ChatRestApi.cpp
    restapi/ChatRestApi.h
    helpers/helpers.h
    helpers/helpers.cpp
    )


add_executable(${PROJECT_NAME}
               ${BIN_SRC}
               ${COMMON_LIBS_SRC}
               ${WS_SERVER_SRC} ${WS_COMMON_SRC}
               ${HTTP_SERVER_SRC} ${HTTP_CLIENT_SRC} ${HTTP_COMMON_SRC}
               )


include(../../cmakes/dependencies.cmake)
linkdeps(${PROJECT_NAME})

add_subdirectory(../../libs/ini-parser PARENT_SCOPE)
add_dependencies(${PROJECT_NAME} iniparser)




