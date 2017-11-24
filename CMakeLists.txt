cmake_minimum_required(VERSION 3.7)
project(wsserver)


# Configuration
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_COMPILER_FLAGS "-Werror -Wfatal-error -pedantic -Wall -Wextra -Wsign-conversion -g -rdynamic")
set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_ROOT_MODULE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

if (MSVC)
	add_compile_options(/W1)
endif ()

include(cmakes/sources_sets.cmake)
include(cmakes/dependencies.cmake)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/ws)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/json/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/http)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/ini-parser/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/args)

add_definitions(-DWS_BIN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/bin")

# Sources
set(SERVER_SRC
    src/server/main.cpp
    src/server/chat/ChatMessageServer.h
    src/server/chat/ChatMessageServer.cpp
    src/server/defs.h
    src/server/chat/Message.h
    src/server/chat/Message.cpp
    src/server/threadsafe.hpp
    src/server/restapi/RestServer.cpp
    src/server/restapi/RestServer.h
    src/server/restapi/ChatRestServer.cpp
    src/server/restapi/ChatRestServer.h
    src/server/helpers/helpers.h
    src/server/helpers/helpers.cpp
    src/server/web/HttpClient.cpp
    src/server/web/HttpClient.h
    src/server/web/QueryBuilder.hpp
    src/server/event/EventNotifier.h
    src/server/event/PostbackTarget.cpp
    src/server/event/PostbackTarget.h
    src/server/event/EventConfig.h
    src/server/helpers/base64.cpp
    src/server/helpers/base64.h
    src/server/StandaloneService.h
    src/server/event/EventNotifier.cpp
    src/server/ServerStarter.cpp
    src/server/ServerStarter.h)

add_executable(${PROJECT_NAME}
               ${SERVER_SRC}
               ${COMMON_LIBS_SRC}
               ${WS_SERVER_SRC} ${WS_COMMON_SRC}
               ${HTTP_SERVER_SRC} ${HTTP_CLIENT_SRC} ${HTTP_COMMON_SRC}
               )

linkdeps(${PROJECT_NAME})

add_subdirectory(src/benchmark)


if ($ENV{USER} STREQUAL "root")
	message(STATUS "INSTALL: available")
	install(
		TARGETS ${PROJECT_NAME}
		RUNTIME
		DESTINATION /usr/sbin
		CONFIGURATIONS Release
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE #0755
		GROUP_READ GROUP_EXECUTE
		WORLD_READ WORLD_EXECUTE
	)

	install(
		FILES ${CMAKE_CURRENT_SOURCE_DIR}/bin/config.json DESTINATION /etc/wsserver/
		PERMISSIONS OWNER_READ OWNER_WRITE #0644
		GROUP_READ
		WORLD_READ

	)

	include(cmakes/FindLinuxPlatform.cmake)
	if (IS_REDHAT)
		install(
			bin/rhel/wsserver.service DESTINATION /usr/lib/systemd/system
			PERMISSIONS OWNER_READ OWNER_WRITE #0644
			GROUP_READ
			WORLD_READ
		)

		execute_process(
			COMMAND /usr/bin/systemctl daemon-reload
			COMMAND /usr/bin/systemctl enable wsserver.service
			OUTPUT_VARIABLE RHEL_ENABLE_SERVICE_OUTPUT
		)
		message(STATUS ${RHEL_ENABLE_SERVICE_OUTPUT})
		message(STATUS "Now you just can start service by calling: systemctl start wsserver.service")

		add_custom_target(
			uninstall
			COMMAND /bin/bash ${CMAKE_CURRENT_SOURCE_DIR}/bin/scripts/rhel_uninstall.sh
		)

	elseif (IS_DEBIAN)
		install(
			bin/deb/wsserver DESTINATION /etc/init.d
			PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE #0755
			GROUP_READ GROUP_EXECUTE
			WORLD_READ WORLD_EXECUTE
		)
	else ()
		message(STATUS "Install target on this system is not supported")
	endif ()
else ()
	message(STATUS "INSTALL:
	To enable install command,
	please run script as root user,
	because binaries and configs must be installed in system dirs.
	If you not trust me,
	than you can just take built binaries and put wherever you want,
	just don't forget about config.json file.
	")
endif ()



