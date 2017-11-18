project(wss_benchmark)

add_executable(wss_benchmark
               main.cpp
               ${COMMON_LIBS_SRC}
               ${WS_COMMON_SRC} ${WS_CLIENT_SRC})

include(../../cmakes/dependencies.cmake)
linkdeps(${PROJECT_NAME})