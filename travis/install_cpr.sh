#!/bin/bash

wget https://github.com/whoshuu/cpr/archive/master.zip && unzip master.zip
mkdir cpr-build && cd cpr-build && cmake -DUSE_SYSTEM_CURL=ON -DBUILD_CPR_TESTS=OFF ../cpr-master/
make
sudo cp -R lib/* /usr/lib/
sudo cp -R ../cpr-master/include/cpr /usr/include/
cd $TRAVIS_BUILD_DIR
