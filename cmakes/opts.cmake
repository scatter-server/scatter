
# Project options
option(ENABLE_SSL "Certifacates required" OFF)
option(ENABLE_REDIS_TARGET "Enables event notifier Redis target (queue or pub/sub channel)" ON)

option(WITH_ARCH "Define target compile architecture" OFF)
option(WITH_BENCHMARK "Compile benchmark (dev only)" OFF)
option(WITH_TEST "Compile tests (dev only)" OFF)

option(CMAKE_INSTALL_PREFIX "Install prefix" "/usr")
