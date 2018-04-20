#!/usr/bin/env bash
rm -rf ${CMAKE_INSTALL_PREFIX}/bin/${PROJECT_NAME}
rm -rf /etc/wsserver
/usr/bin/systemctl stop wsserver.service
/usr/bin/systemctl disable wsserver.service
rm -rf /usr/lib/systemd/system/wsserver.service
/usr/bin/systemctl daemon-reload

echo "Service disabled and removed"
