#!/usr/bin/env bash
rm -rf ${CMAKE_INSTALL_PREFIX}/bin/${PROJECT_NAME}
rm -rf /etc/wsserver
/bin/systemctl stop wsserver.service
/bin/systemctl disable wsserver.service
rm -rf /lib/systemd/system/wsserver.service
/bin/systemctl daemon-reload

echo "Service disabled and removed"
