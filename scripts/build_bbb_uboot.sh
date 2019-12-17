#!/bin/bash

ARCH=arm
UBOOT_ROOT=uboot
LEARN_LINUX_PROJ_ROOT=`pwd`
BUILD_ROOT=$LEARN_LINUX_PROJ_ROOT/build/uboot

CROSS_COMPILE_PATH=$LEARN_LINUX_PROJ_ROOT/tools/gcc-linaro-7.3.1-2018.05-x86_64_arm-linux-gnueabihf/bin
CROSS_COMPILE=$CROSS_COMPILE_PATH/arm-linux-gnueabihf-

make -C $UBOOT_ROOT                  \
	CROSS_COMPILE=$CROSS_COMPILE \
	ARCH=$ARCH                   \
	O=$BUILD_ROOT                \
	distclean

make -C $UBOOT_ROOT                  \
	CROSS_COMPILE=$CROSS_COMPILE \
	ARCH=$ARCH                   \
	O=$BUILD_ROOT                \
	am335x_boneblack_vboot_defconfig

make -C $UBOOT_ROOT                  \
	CROSS_COMPILE=$CROSS_COMPILE \
	ARCH=$ARCH                   \
	O=$BUILD_ROOT
