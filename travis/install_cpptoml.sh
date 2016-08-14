#!/bin/bash

wget https://raw.githubusercontent.com/skystrife/cpptoml/master/include/cpptoml.h
sudo mv cpptoml.h /usr/include/
cd $TRAVIS_BUILD_DIR
