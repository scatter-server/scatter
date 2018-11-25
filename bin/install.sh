#!/usr/bin/env bash

echo "Restarting systemctl..."
/bin/systemctl daemon-reload
echo "Enabling systemctl -> scatter.service"
/bin/systemctl enable scatter.service

echo ""
echo "Now you just can start service by calling:"
echo "  -> systemctl start scatter.service"
echo "or"
echo "  -> service scatter start"