#!/bin/bash

CMAKE_BIN=`which cmake | tr '\n' ' '`
CMAKE_VERS_MAJOR=`${CMAKE_BIN} --version | grep version | awk '{print $3}' | awk -F '.' '{print $1}'`

if [ "${CMAKE_VERS_MAJOR}" -lt 3 ]
then
	echo "CMake version must be >= 3.0.0"
	exit 1
fi

git clone --recursive https://github.com/edwardstock/toolboxpp.git toolbox
mkdir -p toolbox/build
cd toolbox/build
sudo ${CMAKE_BIN} .. -DCMAKE_BUILD_TYPE=Release
${CMAKE_BIN} --build . --target toolboxpp
sudo ${CMAKE_BIN} --build . --target install
cd ../../

mkdir -p build && cd build
sudo ${CMAKE_BIN} .. -DCMAKE_BUILD_TYPE=Release
${CMAKE_BIN} --build . --target wsserver
sudo ${CMAKE_BIN} --build . --target install