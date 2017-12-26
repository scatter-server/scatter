#!/usr/bin/env bash

git clone --recursive https://github.com/edwardstock/toolboxpp.git toolbox
mkdir -p toolbox/build
cd toolbox/build
sudo cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --target toolboxpp
sudo cmake --build . --target install
cd ../../

mkdir -p build && cd build
sudo cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --target wsserver
sudo cmake --build . --target install