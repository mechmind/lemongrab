#!/bin/bash

wget https://github.com/whoshuu/cpr/archive/1.2.0.tar.gz && tar -xzf 1.2.0.tar.gz
mkdir cpr-build && cd cpr-build && cmake -DUSE_SYSTEM_CURL=ON -DBUILD_CPR_TESTS=OFF ../cpr-1.2.0/
make
sudo cp -R lib/* /usr/lib/
sudo mkdir -p /usr/include/cpr
sudo cp -R ../cpr-1.2.0/include/* /usr/include/cpr
cd $TRAVIS_BUILD_DIR
