#!/bin/bash

DOXY_BIN=`which doxygen | tr -d '\n'`
if [ "${DOXY_BIN}" == "" ]
then
	echo "Doxygen binary did not found!"
	exit 1
fi

cd src
ln -s ../README.md README.md
cd ../docs
${DOXY_BIN} wsserver_doc
rm -f README.md
cd ../

echo -e ""
echo "Complete!"
echo "Now just open ${PWD}/docs/html/index.html"

