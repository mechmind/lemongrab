#!/bin/bash

git pull
make clean
make

killall lemongrab
nohup ./lemongrab >lemongrab.out 2>lemongrab.err &
