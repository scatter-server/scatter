#!/usr/bin/env bash

git clone --recursive https://github.com/edwardstock/toolboxpp.git toolbox
mkdir -p toolbox/build
cd toolbox/build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
cmake --build . --target install
cd ../../

mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DWITH_TEST=ON
cmake --build .
cmake --build . --target install