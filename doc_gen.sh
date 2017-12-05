#!/usr/bin/env bash
cd src
ln -s ../README.md README.md
cd ../docs
doxygen wsserver_doc
rm -f README.md
cd ../

echo -e ""
echo "Complete!"
PWD=$(pwd)
echo "Now just open ${PWD}/docs/html/index.html"

