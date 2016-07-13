#!/bin/bash

git pull
cd build
cmake .. -DGLOOX_INCLUDE_DIR=~/gloox-1.0.15/src -DGLOOX_LIBRARY=~/gloox-1.0.15/src/.libs/libgloox.so \
    -DCPR_INCLUDE_DIR=~/cpr-1.2.0/include -DCPR_LIBRARY=~/cpr-1.2.0/build/lib/libcpr.a
make || exit 1

killall lemongrab
LD_PRELOAD=~/gloox-1.0.15/src/.libs/libgloox.so nohup ./lemongrab lemongrab.out lemongrab.err &
