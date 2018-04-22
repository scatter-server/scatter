#!/usr/bin/env bash

echo "Restarting systemctl..."
/bin/systemctl daemon-reload
echo "Enabling systemctl -> wsserver.service"
/bin/systemctl enable wsserver.service

echo ""
echo "Now you just can start service by calling:"
echo "  -> systemctl start wsserver.service"
echo "or"
echo "  -> service wsserver start"