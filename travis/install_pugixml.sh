#!/bin/bash

wget http://github.com/zeux/pugixml/releases/download/v1.7/pugixml-1.7.tar.gz && tar -xzf pugixml-1.7.tar.gz
cd pugixml-1.7/scripts && cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=/usr
make && sudo make install
cd $TRAVIS_BUILD_DIR
