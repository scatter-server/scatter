#!/usr/bin/env bash
rm -rf /usr/sbin/${PROJECT_NAME}
rm -rf /etc/wsserver
/bin/systemctl stop wsserver.service
/bin/systemctl disable wsserver.service
rm -rf /lib/systemd/system/wsserver.service
/bin/systemctl daemon-reload

echo "Service disabled and removed"
