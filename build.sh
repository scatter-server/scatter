#!/usr/bin/env bash

git clone --recursive https://github.com/edwardstock/toolboxpp.git toolbox
mkdir -p toolbox/build
cd toolbox/build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --target toolboxpp
cmake --build . --target install
cd ../../

mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --target wsserver
cmake --build . --target install