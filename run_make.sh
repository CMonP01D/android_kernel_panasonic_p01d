#!/bin/bash
export CCOMPILER=~/Android/PONYO/CM/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi-
#export CCOMPILER=~/Android/PONYO/CM/prebuilt/linux-x86/toolchain/gcc-linaro-4.7-2012.09/bin/arm-eabi-
export KERNEL_DIR=$PWD

make ARCH=arm CROSS_COMPILE=$CCOMPILER -j`grep 'processor' /proc/cpuinfo | wc -l`
