
include(FindLinuxPlatform.cmake)

set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${PROJECT_ARCH})
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libssl-dev, libcurl4-openssl-dev, libboost1.63-all-dev")

include(CPack)