#!/usr/bin/env bash
rm -rf ${CMAKE_INSTALL_PREFIX}/bin/${PROJECT_NAME}
rm -rf /etc/scatter
/usr/bin/systemctl stop scatter.service
/usr/bin/systemctl disable scatter.service
rm -rf /lib/systemd/system/scatter.service
/usr/bin/systemctl daemon-reload

echo "Service disabled and removed"
