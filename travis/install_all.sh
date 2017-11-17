#!/bin/bash

# ./travis/install_cmake.sh || die 1
./travis/install_jsoncpp.sh  || die 1
./travis/install_cpr.sh || die 1
./travis/install_cpptoml.sh || die 1
./travis/install_pugixml.sh || die 1
./travis/install_sqlite_orm.sh || die 1
./travis/install_hexicord.sh || die 1
