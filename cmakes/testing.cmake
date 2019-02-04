set(PROJECT_NAME_TEST wstest)
set(GTEST_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/googletest/googletest)

include_directories(${GTEST_SOURCE_DIR}/include ${GTEST_SOURCE_DIR})
add_subdirectory(${GTEST_SOURCE_DIR})
if (WIN32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x")
elseif (APPLE)
	add_definitions(-DGTEST_USE_OWN_TR1_TUPLE)
	add_definitions(-D__GLIBCXX__)
endif ()

#add_executable(${PROJECT_NAME_TEST} ${SERVER_EXEC_SRCS}
##               tests/base/TestAuth.cpp
#
#               )
target_compile_options(${PROJECT_NAME_TEST} PUBLIC -Wno-unused-parameter)

linkdeps(${PROJECT_NAME_TEST})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(${PROJECT_NAME_TEST} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(${PROJECT_NAME_TEST} gtest gtest_main)

#include(cmakes/CodeCoverage.cmake)
#append_coverage_compiler_flags()

set(COVERAGE_TARGET_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/server)

set(COVERAGE_EXCLUDES
    '${CMAKE_CURRENT_SOURCE_DIR}/src/benchmark'
    '${CMAKE_CURRENT_SOURCE_DIR}/libs/*'
    '${CMAKE_CURRENT_SOURCE_DIR}/tests/*'
    'boost/*'
    '/usr/include/*'
    '/usr/local/*'
    'v1'
    )

#setup_target_for_coverage(
#	NAME ws_coverage
#	EXECUTABLE ${PROJECT_NAME_TEST}
#	DEPENDENCIES ${PROJECT_NAME_TEST}
#)

