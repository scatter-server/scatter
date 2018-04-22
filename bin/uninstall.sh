#!/usr/bin/env bash
rm -rf ${CMAKE_INSTALL_PREFIX}/bin/${PROJECT_NAME}
rm -rf /etc/wsserver
/bin/systemctl stop ${PROJECT_NAME}.service
/bin/systemctl disable ${PROJECT_NAME}.service
rm -rf ${SYSTEMD_SERVICE_PATH}/${PROJECT_NAME}.service
/bin/systemctl daemon-reload

echo "Service \"${PROJECT_NAME}.service\" disabled and removed"
