include(${PROJECT_ROOT}/cmakes/FindLinuxPlatform.cmake)
include(FeatureSummary)

if (NOT INSTALL_PREFIX)
	set(CMAKE_INSTALL_PREFIX "/usr" CACHE INTERNAL "Prefix prepended to install directories" FORCE)
else ()
	set(CMAKE_INSTALL_PREFIX "${INSTALL_PREFIX}" CACHE INTERNAL "Prefix prepended to install directories" FORCE)
endif ()

message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")

set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VENDOR "Eduard Maximovich")
set(CPACK_PACKAGE_CONTACT "edward.vstock@gmail.com")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/scatter-server/scatter")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Simple, standalone, websocket high-performance message server, written in C++")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_ROOT}/LICENSE")

if (NOT "${BUILD_OS}" STREQUAL "")
	set(CPACK_SYSTEM_NAME "linux-${BUILD_OS}-${PROJECT_ARCH}")
else ()
	set(CPACK_SYSTEM_NAME "linux-${PROJECT_ARCH}")
endif ()


install(
	TARGETS ${PROJECT_NAME} ${PROJECT_NAME}_core
	RUNTIME DESTINATION bin
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	PUBLIC_HEADER DESTINATION include/scatter
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE #0755
	GROUP_READ GROUP_EXECUTE
	WORLD_READ WORLD_EXECUTE
)

if ("$ENV{USER}" STREQUAL "root")
	install(
		FILES ${PROJECT_ROOT}/bin/config.json DESTINATION /etc/scatter/
		PERMISSIONS OWNER_READ OWNER_WRITE #0644
		GROUP_READ
		WORLD_READ
	)
endif ()

if (IS_REDHAT)
	set(SYSTEMD_SERVICE_PATH "/usr/lib/systemd/system")

	configure_file(${PROJECT_ROOT}/bin/scatter.service ${CMAKE_BINARY_DIR}/bin/scatter.service)
	configure_file(${PROJECT_ROOT}/bin/install.sh ${CMAKE_BINARY_DIR}/bin/install.sh)
	configure_file(${PROJECT_ROOT}/bin/uninstall.sh ${CMAKE_BINARY_DIR}/bin/uninstall.sh)

	install(
		FILES ${CMAKE_BINARY_DIR}/bin/scatter.service
		DESTINATION ${SYSTEMD_SERVICE_PATH}
		PERMISSIONS OWNER_READ OWNER_WRITE #0644
		GROUP_READ
		WORLD_READ
	)

	install(CODE "execute_process(COMMAND ${CMAKE_BINARY_DIR}/bin/install.sh)")

	add_custom_target(
		uninstall
		COMMAND /bin/bash ${CMAKE_BINARY_DIR}/bin/uninstall.sh
	)

	set(CPACK_GENERATOR "RPM")
	set(CPACK_RPM_PACKAGE_ARCHITECTURE "${PROJECT_ARCH}")
	set(CPACK_RPM_PACKAGE_LICENSE "Apache-2.0")
	set(CPACK_RPM_PACKAGE_URL "https://github.com/scatter-server/scatter")
	set(CPACK_RPM_PACKAGE_GROUP "System Environment/Daemons")
	set(CPACK_RPM_PACKAGE_REQUIRES "openssl >= 1.0.0, libcurl >= 7.29.0")

elseif (IS_DEBIAN)
	set(SYSTEMD_SERVICE_PATH "/lib/systemd/system")
	configure_file(${PROJECT_ROOT}/bin/scatter.service ${CMAKE_BINARY_DIR}/bin/scatter.service)
	configure_file(${PROJECT_ROOT}/bin/install.sh ${CMAKE_BINARY_DIR}/bin/install.sh)
	configure_file(${PROJECT_ROOT}/bin/uninstall.sh ${CMAKE_BINARY_DIR}/bin/uninstall.sh)
	install(
		FILES ${CMAKE_BINARY_DIR}/bin/scatter.service
		DESTINATION ${SYSTEMD_SERVICE_PATH}
		PERMISSIONS OWNER_READ OWNER_WRITE #0644
		GROUP_READ
		WORLD_READ
	)

	install(CODE "execute_process(COMMAND ${CMAKE_BINARY_DIR}/bin/install.sh)")

	add_custom_target(
		uninstall
		COMMAND /bin/bash ${CMAKE_BINARY_DIR}/bin/uninstall.sh
	)

	set(CPACK_GENERATOR "DEB")
	set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${PROJECT_ARCH})
	set(CPACK_DEBIAN_PACKAGE_DEPENDS "libcurl4-openssl-dev (>=7.26.0)")
	set(CPACK_DEBIAN_PACKAGE_SECTION "Network")
	if (CMAKE_BUILD_TYPE STREQUAL "Debug")
		set(CPACK_DEB_PACKAGE_DEBUG ON)
	endif ()

else ()
	message(WARNING "Packaging on this system is not supported")
endif ()

include(CPack)