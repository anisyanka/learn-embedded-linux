.PHONY: uboot kernel kernel_config install_host_deps clean

ARCH=arm
SCRIPTS_DIR=scripts
LEARN_LINUX_PROJ_ROOT=$(shell pwd)

# toolchain
CROSS_COMPILER_PATH=$(LEARN_LINUX_PROJ_ROOT)/embedded_system_parts/tools/gcc-linaro-7.3.1-2018.05-x86_64_arm-linux-gnueabihf/bin
CROSS_COMPILER_PREFX=$(CROSS_COMPILER_PATH)/arm-linux-gnueabihf- 

# src dirs
UBOOT_SRC_DIR=embedded_system_parts/uboot
KERNEL_SRC_DIR=embedded_system_parts/kernel

# build dirs
BUILD_ROOT_DIR=$(LEARN_LINUX_PROJ_ROOT)/build
UBOOT_BUILD_DIR=$(BUILD_ROOT_DIR)/uboot
KERNEL_BUILD_DIR=$(BUILD_ROOT_DIR)/kernel

# out dirs
UBOOT_BINARIES_DIR=$(LEARN_LINUX_PROJ_ROOT)/out/uboot
KERNEL_BINARIES_DIR=$(LEARN_LINUX_PROJ_ROOT)/out/kernel

# BBB dtb
DTB=am335x-boneblack.dtb

all: uboot kernel

uboot:
	$(MAKE) -C $(UBOOT_SRC_DIR)                   \
		CROSS_COMPILE=$(CROSS_COMPILER_PREFX) \
		ARCH=$(ARCH)                          \
		O=$(UBOOT_BUILD_DIR)                  \
		distclean

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

kernel:
	$(MAKE) -C $(KERNEL_SRC_DIR)                  \
		CROSS_COMPILE=$(CROSS_COMPILER_PREFX) \
		ARCH=$(ARCH)                          \
		O=$(KERNEL_BUILD_DIR)                 \
		distclean

	# after this .config file will place in the `KERNEL_BUILD_DIR`
	$(MAKE) -C $(KERNEL_SRC_DIR)                  \
		CROSS_COMPILE=$(CROSS_COMPILER_PREFX) \
		ARCH=$(ARCH)                          \
		O=$(KERNEL_BUILD_DIR)                 \
		multi_v7_defconfig

	$(MAKE) -C $(KERNEL_SRC_DIR)             \
		CROSS_COMPILE=$(CROSS_COMPILER_PREFX) \
		ARCH=$(ARCH)                          \
		O=$(KERNEL_BUILD_DIR)

	cp $(KERNEL_BUILD_DIR)/arch/$(ARCH)/boot/Image            \
		$(KERNEL_BUILD_DIR)/arch/$(ARCH)/boot/zImage      \
		$(KERNEL_BUILD_DIR)/arch/$(ARCH)/boot/dts/$(DTB)  \
		$(KERNEL_BUILD_DIR)/vmlinux                       \
		$(KERNEL_BUILD_DIR)/.config                       \
		--target-directory=$(KERNEL_BINARIES_DIR)

clean:
	$(MAKE) -C $(UBOOT_SRC_DIR)                   \
		CROSS_COMPILE=$(CROSS_COMPILER_PREFX) \
		ARCH=$(ARCH)                          \
		O=$(UBOOT_BUILD_DIR)                  \
		distclean

	$(MAKE) -C $(KERNEL_SRC_DIR)                  \
		CROSS_COMPILE=$(CROSS_COMPILER_PREFX) \
		ARCH=$(ARCH)                          \
		O=$(KERNEL_BUILD_DIR)                 \
		distclean

kernel_config:
	$(MAKE) -C $(KERNEL_SRC_DIR)             \
		CROSS_COMPILE=$(CROSS_COMPILER_PREFX) \
		ARCH=$(ARCH)                          \
		O=$(KERNEL_BUILD_DIR)                 \
		menuconfig

	cp $(KERNEL_BUILD_DIR)/.config --target-directory=$(KERNEL_BINARIES_DIR)

install_host_deps:
	@./$(SCRIPTS_DIR)/install_host_deps.sh
