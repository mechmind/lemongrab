#!/bin/bash

git pull
make clean
make

killall lemongrab
./lemongrab &
