#!/bin/bash

git pull
cd build
cmake ..
make || exit 1

killall lemongrab
nohup ./lemongrab lemongrab.out lemongrab.err &
