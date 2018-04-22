message(STATUS "INSTALL: available")

set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VENDOR "Eduard Maximovich")
set(CPACK_PACKAGE_CONTACT "edward.vstock@gmail.com")
set(CPACK_SYSTEM_NAME "linux-${PROJECT_ARCH}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "WebSocket message server")

install(
	TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION bin
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
include(FeatureSummary)

option(DEBIAN_SYSTEMD_SERVICE "Enable systemd service for Debian system" OFF)
if (IS_REDHAT)
	set(SYSTEMD_SERVICE_PATH "/usr/lib/systemd/system")

	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bin/wsserver.service ${CMAKE_BINARY_DIR}/bin/wsserver.service)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bin/install.sh ${CMAKE_BINARY_DIR}/bin/install.sh)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bin/uninstall.sh ${CMAKE_BINARY_DIR}/bin/uninstall.sh)

	install(
		FILES ${CMAKE_BINARY_DIR}/bin/wsserver.service
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
	set(CPACK_RPM_PACKAGE_URL "https://github.com/edwardstock/wsserver")
	set(CPACK_RPM_PACKAGE_GROUP "System Environment/Daemons")
	set(CPACK_RPM_PACKAGE_REQUIRES "openssl >= 1.0.0, libcurl >= 7.29.0")

elseif (IS_DEBIAN)
	set(SYSTEMD_SERVICE_PATH "/lib/systemd/system")
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bin/wsserver.service ${CMAKE_BINARY_DIR}/bin/wsserver.service)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bin/install.sh ${CMAKE_BINARY_DIR}/bin/install.sh)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bin/uninstall.sh ${CMAKE_BINARY_DIR}/bin/uninstall.sh)
	install(
		FILES ${CMAKE_BINARY_DIR}/bin/wsserver.service
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
	message(STATUS "Install target on this system is not supported")
endif ()

include(CPack)