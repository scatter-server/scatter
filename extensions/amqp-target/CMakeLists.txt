project(scatter_event_amqp
        VERSION 1.0.0
        DESCRIPTION "AMQP Protocol (rabbitmq) event target for Scatter websocket server"
        HOMEPAGE_URL "https://github.com/scatter"
        LANGUAGES CXX)

cmake_minimum_required(VERSION 3.2)
set(CMAKE_CXX_STANDARD 14)

set(SRC_FILES
    AMQPTarget.cpp
    AMQPTarget.h
    ScatterBoostAsioHandler.h)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})
target_compile_options(${PROJECT_NAME} PUBLIC -fPIC)
linkdeps(TARGET ${PROJECT_NAME} DEPENDENCIES amqp json fmt openssl)

target_link_libraries(${PROJECT_NAME} scatter_core)
target_link_libraries(${PROJECT_NAME} ${CMAKE_DL_LIBS})

install(
	TARGETS ${PROJECT_NAME}
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE #0755
	GROUP_READ GROUP_EXECUTE
	WORLD_READ WORLD_EXECUTE
)