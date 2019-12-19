#!/bin/bash

source ./scripts/global_vars.sh

make -C $UBOOT_SRC_DIR                      \
	CROSS_COMPILE=$CROSS_COMPILER_PREFX \
	ARCH=$ARCH                          \
	O=$UBOOT_BUILD_DIR                  \
	distclean

make -C $UBOOT_SRC_DIR                      \
	CROSS_COMPILE=$CROSS_COMPILER_PREFX \
	ARCH=$ARCH                          \
	O=$UBOOT_BUILD_DIR                  \
	am335x_boneblack_vboot_defconfig

make -C $UBOOT_SRC_DIR                      \
	CROSS_COMPILE=$CROSS_COMPILER_PREFX \
	ARCH=$ARCH                          \
	O=$UBOOT_BUILD_DIR
