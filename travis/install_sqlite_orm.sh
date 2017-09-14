#!/bin/bash

sudo mkdir -p /usr/include/sqlite_orm/
cd /usr/include/sqlite_orm/
sudo wget https://cdn.rawgit.com/fnc12/sqlite_orm/master/include/sqlite_orm/sqlite_orm.h
# sudo patch -p3 < $TRAVIS_BUILD_DIR/travis/0001-sqliteorm-escape-update.patch

cd $TRAVIS_BUILD_DIR
