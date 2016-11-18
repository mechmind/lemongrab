#!/bin/bash

wget https://github.com/open-source-parsers/jsoncpp/archive/1.6.5.tar.gz && tar -xzf 1.6.5.tar.gz
cd jsoncpp-1.6.5 && mkdir build && cd build && cmake -DCMAKE_INSTALL_PREFIX=/usr ..
make && sudo make install
gem install coveralls-lcov
cd /usr/src/gtest && sudo cmake .
sudo make
sudo mv libg* /usr/lib/
cd $TRAVIS_BUILD_DIR
