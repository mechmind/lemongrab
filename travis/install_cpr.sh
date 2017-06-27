#!/bin/bash

wget https://github.com/whoshuu/cpr/archive/master.zip && unzip master.zip
mkdir cpr-build && cd cpr-build && cmake -DUSE_SYSTEM_CURL=ON -DBUILD_CPR_TESTS=OFF ../cpr-master/
make
sudo cp -R lib/* /usr/lib/
sudo cp -R ../cpr-master/include/cpr /usr/include/
sed -i -e '14s/cpr.h/cpr\/cpr.h/' cpr-config.cmake
sudo mkdir -p /usr/share/cpr
sudo cp cpr-config.cmake /usr/share/cpr/
cd $TRAVIS_BUILD_DIR
