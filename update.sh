#!/bin/bash

git pull
cd build
cmake .. -DGLOOXPATH=~/gloox-1.0.15/
make || exit 1

killall lemongrab
LD_PRELOAD=~/gloox-1.0.15/src/.libs/libgloox.so nohup ./lemongrab lemongrab.out lemongrab.err &
