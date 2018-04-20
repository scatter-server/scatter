#!/usr/bin/env bash
echo "Restarting systemctl..."
/usr/bin/systemctl daemon-reload
echo "Enabling systemctl -> ${PROJECT_NAME}.service"
/usr/bin/systemctl enable "${PROJECT_NAME}.service"

echo ""
echo "Now you just can start service by calling:"
echo "  -> systemctl start ${PROJECT_NAME}.service"
echo "or"
echo "  -> service ${PROJECT_NAME} start"