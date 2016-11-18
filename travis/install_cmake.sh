#!/bin/bash

wget http://www.cmake.org/files/v3.3/cmake-3.3.2.tar.gz
tar xf cmake-3.3.2.tar.gz
cd cmake-3.3.2
./configure
make
sudo make install
cd $TRAVIS_BUILD_DIR
