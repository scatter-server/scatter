#!/usr/bin/env bash

echo "Restarting systemctl..."
/usr/bin/systemctl daemon-reload
echo "Enabling systemctl -> wsserver.service"
/usr/bin/systemctl enable wsserver.service

echo ""
echo "Now you just can start service by calling:"
echo "  -> systemctl start wsserver.service"
echo "or"
echo "  -> service wsserver start"