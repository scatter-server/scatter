#!/usr/bin/env bash

/usr/bin/systemctl daemon-reload
/usr/bin/systemctl enable wsserver.service

echo "Now you just can start service by calling:"
echo "  -> systemctl start wsserver.service"
echo "or"
echo "  -> service wsserver start"