project(scatter_event_redis
        VERSION 1.0.0
        DESCRIPTION "Redis event target for Scatter websocket server"
        HOMEPAGE_URL "https://github.com/scatter"
        LANGUAGES CXX)
cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 14)

set(SRC_FILES
    RedisTarget.cpp
    RedisTarget.h)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})
target_compile_options(${PROJECT_NAME} PUBLIC -fPIC)

linkdeps(TARGET ${PROJECT_NAME} DEPENDENCIES redis json fmt)

target_link_libraries(${PROJECT_NAME} scatter_core)

install(
	TARGETS ${PROJECT_NAME}
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE #0755
	GROUP_READ GROUP_EXECUTE
	WORLD_READ WORLD_EXECUTE
)