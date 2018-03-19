message(STATUS "INSTALL: available")
install(
        TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION /usr/sbin
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
    install(
            FILES ${CMAKE_CURRENT_SOURCE_DIR}/bin/rhel/wsserver.service DESTINATION /usr/lib/systemd/system
            PERMISSIONS OWNER_READ OWNER_WRITE #0644
            GROUP_READ
            WORLD_READ
    )

    install(CODE "execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/bin/rhel/install.sh)")

    add_custom_target(
            uninstall
            COMMAND /bin/bash ${CMAKE_CURRENT_SOURCE_DIR}/bin/rhel/uninstall.sh
    )
elseif (IS_DEBIAN)
    #    if (DEBIAN_SYSTEMD_SERVICE)
        install(
                FILES ${CMAKE_CURRENT_SOURCE_DIR}/bin/deb/wsserver.service DESTINATION /lib/systemd/system
                PERMISSIONS OWNER_READ OWNER_WRITE #0644
                GROUP_READ
                WORLD_READ
        )

        install(CODE "execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/bin/deb/install_systemd.sh)")

        add_custom_target(
                uninstall
                COMMAND /bin/bash ${CMAKE_CURRENT_SOURCE_DIR}/bin/deb/uninstall_systemd.sh
        )
    #    else ()
    #        install(
    #                FILES ${CMAKE_CURRENT_SOURCE_DIR}/bin/deb/wsserver DESTINATION /etc/init.d
    #                PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE #0755
    #                GROUP_READ GROUP_EXECUTE
    #                WORLD_READ WORLD_EXECUTE
    #        )
    #        add_custom_target(
    #                uninstall
    #                COMMAND /bin/bash ${CMAKE_CURRENT_SOURCE_DIR}/bin/deb/uninstall_initd.sh
    #        )
    #    endif ()
else ()
    message(STATUS "Install target on this system is not supported")
endif ()