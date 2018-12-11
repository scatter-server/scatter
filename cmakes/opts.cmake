
# Project options
option(ENABLE_REDIS_TARGET "Enables event notifier Redis target (queue or pub/sub channel)" ON)
option(ENABLE_AMQP_TARGET "Build AMQP client to use queue server to receive chat messages" ON)
set(INSTALL_PREFIX "/usr" CACHE STRING "Prefix prepended to install directories")
option(WITH_ARCH "Define target compile architecture" OFF)
option(WITH_BENCHMARK "Compile benchmark (dev only)" OFF)
option(WITH_TEST "Compile tests (dev only)" OFF)