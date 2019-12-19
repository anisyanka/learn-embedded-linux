#!/bin/bash

export ARCH=arm
export LEARN_LINUX_PROJ_ROOT=`pwd`

# toolchain
export CROSS_COMPILER_PATH=$LEARN_LINUX_PROJ_ROOT/tools/gcc-linaro-7.3.1-2018.05-x86_64_arm-linux-gnueabihf/bin
export CROSS_COMPILER_PREFX=$CROSS_COMPILER_PATH/arm-linux-gnueabihf-

# src dirs
export UBOOT_SRC_DIR=uboot
export KERNEL_SRC_DIR=kernel

# build dirs
export BUILD_ROOT_DIR=$LEARN_LINUX_PROJ_ROOT/build
export UBOOT_BUILD_DIR=$BUILD_ROOT_DIR/uboot
export KERNEL_BUILD_DIR=$BUILD_ROOT_DIR/kernel

# out dirs
export UBOOT_BINARIES_DIR=$LEARN_LINUX_PROJ_ROOT/out/uboot
export KERNEL_BINARIES_DIR=$LEARN_LINUX_PROJ_ROOT/out/kernel
