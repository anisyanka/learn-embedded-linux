#!/bin/bash

source ./scripts/global_vars.sh

cp $UBOOT_BUILD_DIR/MLO $UBOOT_BUILD_DIR/u-boot* \
	--target-directory=$UBOOT_BINARIES_DIR
