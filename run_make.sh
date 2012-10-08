#!/bin/bash
source ~/Android/env.sh
export KERNEL_DIR=~/Android/PONYO/ponyo_kernel

make ARCH=arm CROSS_COMPILE=$CCOMPILER -j`grep 'processor' /proc/cpuinfo | wc -l`
