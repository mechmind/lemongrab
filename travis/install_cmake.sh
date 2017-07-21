#!/bin/bash

wget https://cmake.org/files/v3.9/cmake-3.9.0-rc1.tar.gz
tar xf cmake-3.9.0-rc1.tar.gz
cd cmake-3.9.0-rc1
./configure
make
sudo make install
cd $TRAVIS_BUILD_DIR
