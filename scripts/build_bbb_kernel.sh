#!/bin/bash

source ./scripts/global_vars.sh

make -C $KERNEL_SRC_DIR                     \
	CROSS_COMPILE=$CROSS_COMPILER_PREFX \
	ARCH=$ARCH                          \
	O=$KERNEL_BUILD_DIR                 \
	distclean

make -C $KERNEL_SRC_DIR                     \
	CROSS_COMPILE=$CROSS_COMPILER_PREFX \
	ARCH=$ARCH                          \
	O=$KERNEL_BUILD_DIR                 \
	multi_v7_defconfig

make -j 8 -C $KERNEL_SRC_DIR                \
	CROSS_COMPILE=$CROSS_COMPILER_PREFX \
	ARCH=$ARCH                          \
	O=$KERNEL_BUILD_DIR
