.PHONY: uboot kernel kernel_config install_host_deps
	uboot_clean kernel_clean clean

ARCH=arm
SCRIPTS_DIR=scripts
LEARN_LINUX_PROJ_ROOT=$(shell pwd)

# toolchain
CROSS_COMPILER_PATH=$(LEARN_LINUX_PROJ_ROOT)/tools/gcc-linaro-7.3.1-2018.05-x86_64_arm-linux-gnueabihf/bin
CROSS_COMPILER_PREFX=$(CROSS_COMPILER_PATH)/arm-linux-gnueabihf- 

# src dirs
UBOOT_SRC_DIR=uboot
KERNEL_SRC_DIR=kernel

# build dirs
BUILD_ROOT_DIR=$(LEARN_LINUX_PROJ_ROOT)/build
UBOOT_BUILD_DIR=$(BUILD_ROOT_DIR)/uboot
KERNEL_BUILD_DIR=$(BUILD_ROOT_DIR)/kernel

# out dirs
UBOOT_BINARIES_DIR=$(LEARN_LINUX_PROJ_ROOT)/out/uboot
KERNEL_BINARIES_DIR=$(LEARN_LINUX_PROJ_ROOT)/out/kernel

# BBB dtb
DTB=am335x-boneblack.dtb

# tftp
TFTP_DIR=/var/lib/tftpboot/

all: uboot kernel

uboot: uboot_clean
	$(MAKE) -C $(UBOOT_SRC_DIR)                      \
		CROSS_COMPILE=$(CROSS_COMPILER_PREFX) \
		ARCH=$(ARCH)                          \
		O=$(UBOOT_BUILD_DIR)                  \
		am335x_boneblack_vboot_defconfig

	$(MAKE) -C $(UBOOT_SRC_DIR)                   \
		CROSS_COMPILE=$(CROSS_COMPILER_PREFX) \
		ARCH=$(ARCH)                          \
		O=$(UBOOT_BUILD_DIR)

	cp $(UBOOT_BUILD_DIR)/MLO $(UBOOT_BUILD_DIR)/u-boot* \
		--target-directory=$(UBOOT_BINARIES_DIR)

uboot_clean:
	$(MAKE) -C $(UBOOT_SRC_DIR)                   \
		CROSS_COMPILE=$(CROSS_COMPILER_PREFX) \
		ARCH=$(ARCH)                          \
		O=$(UBOOT_BUILD_DIR)                  \
		distclean

kernel_config:
	cp $(KERNEL_BINARIES_DIR)/.config --target-directory=$(KERNEL_BUILD_DIR)

	$(MAKE) -C $(KERNEL_SRC_DIR)                  \
		CROSS_COMPILE=$(CROSS_COMPILER_PREFX) \
		ARCH=$(ARCH)                          \
		O=$(KERNEL_BUILD_DIR)                 \
		menuconfig

	cp $(KERNEL_BUILD_DIR)/.config --target-directory=$(KERNEL_BINARIES_DIR)

kernel: kernel_clean
	cp $(KERNEL_BINARIES_DIR)/.config --target-directory=$(KERNEL_BUILD_DIR)

	$(MAKE) -j 8 -C $(KERNEL_SRC_DIR)             \
		CROSS_COMPILE=$(CROSS_COMPILER_PREFX) \
		ARCH=$(ARCH)                          \
		O=$(KERNEL_BUILD_DIR)

	cp $(KERNEL_BUILD_DIR)/arch/$(ARCH)/boot/Image            \
		$(KERNEL_BUILD_DIR)/arch/$(ARCH)/boot/zImage      \
		$(KERNEL_BUILD_DIR)/arch/$(ARCH)/boot/dts/$(DTB)  \
		$(KERNEL_BUILD_DIR)/vmlinux                       \
		$(KERNEL_BUILD_DIR)/.config                       \
		--target-directory=$(KERNEL_BINARIES_DIR)

	sudo cp $(KERNEL_BINARIES_DIR)/zImage                     \
		$(KERNEL_BINARIES_DIR)/$(DTB)                     \
		--target-directory=$(TFTP_DIR)

kernel_clean:
	$(MAKE) -C $(KERNEL_SRC_DIR)                  \
		CROSS_COMPILE=$(CROSS_COMPILER_PREFX) \
		ARCH=$(ARCH)                          \
		O=$(KERNEL_BUILD_DIR)                 \
		distclean

clean: uboot_clean kernel_clean

install_host_deps:
	@./$(SCRIPTS_DIR)/install_host_deps.sh
