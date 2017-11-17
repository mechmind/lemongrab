#!/bin/bash

git clone https://github.com/foxcpp/Hexicord.git
mkdir hexibuild && cd hexibuild
cmake ../Hexicord && make
sudo make install
cd $TRAVIS_BUILD_DIR
