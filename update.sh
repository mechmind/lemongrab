#!/bin/bash

git pull
#qmake lemon
make clean
make

killall lemongrab
./lemongrab &
