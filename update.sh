#!/bin/bash

git pull
cd build
cmake .. -DGLOOXPATH=~/gloox-1.0.15/
make || exit 1

killall lemongrab
nohup ./lemongrab lemongrab.out lemongrab.err &
