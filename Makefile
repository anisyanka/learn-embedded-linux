.PHONY: uboot kernel kernel_config install_host_deps  \
	my_modules my_modules_clean                   \
	$(MY_MODULES_NAMES) $(MY_MODULES_CLEAN_NAMES) \
	dtb

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
DTS_DIR=$(LEARN_LINUX_PROJ_ROOT)/$(KERNEL_SRC_DIR)/arch/$(ARCH)/boot/dts
DTS=am335x-boneblack-custom.dts
DTB=am335x-boneblack-custom.dtb

# tftp
TFTP_DIR=/var/lib/tftpboot/

# modues install and build path
TARGET_NFS_ROOT_DIR=$(LEARN_LINUX_PROJ_ROOT)/nfsroot
TARGET_MOD_DIR_FOR_BUILD=$(TARGET_NFS_ROOT_DIR)/lib/modules/5.5.0-rc6/build
TARGET_MY_MOD_INSTALL_PATH=$(TARGET_NFS_ROOT_DIR)/root/modules

# my modules
MY_MOD_SRC_ROOT_DIR=$(LEARN_LINUX_PROJ_ROOT)/modules

KO_END=ko
CLEAN_END=clean

MY_MOD_HELLO_NAME=hello_bbb
MY_MOD_RTC_NAME=rtc_ds3231
MY_MOD_STR_NAME=strmodule

MY_MODULES_NAMES=$(MY_MOD_HELLO_NAME).$(KO_END) \
		$(MY_MOD_RTC_NAME).$(KO_END) \
		$(MY_MOD_STR_NAME).$(KO_END) \

MY_MODULES_CLEAN_NAMES=	$(MY_MOD_HELLO_NAME).$(CLEAN_END) \
			$(MY_MOD_RTC_NAME).$(CLEAN_END) \
			$(MY_MOD_STR_NAME).$(CLEAN_END) \

all: uboot kernel my_modules

uboot:
	$(MAKE) -C $(UBOOT_SRC_DIR)                   \
		CROSS_COMPILE=$(CROSS_COMPILER_PREFX) \
		ARCH=$(ARCH)                          \
		O=$(UBOOT_BUILD_DIR)                  \
		distclean

	$(MAKE) -C $(UBOOT_SRC_DIR)                   \
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

kernel_config:
	cp $(KERNEL_BINARIES_DIR)/.config --target-directory=$(KERNEL_BUILD_DIR)

	$(MAKE) -C $(KERNEL_SRC_DIR)                  \
		CROSS_COMPILE=$(CROSS_COMPILER_PREFX) \
		ARCH=$(ARCH)                          \
		O=$(KERNEL_BUILD_DIR)                 \
		menuconfig

	cp $(KERNEL_BUILD_DIR)/.config --target-directory=$(KERNEL_BINARIES_DIR)

dtb:
	$(MAKE) -C $(KERNEL_SRC_DIR)              \
		CROSS_COMPILE=$(CROSS_COMPILER_PREFX) \
		ARCH=$(ARCH)                          \
		O=$(KERNEL_BUILD_DIR) dtbs

	cp $(KERNEL_BUILD_DIR)/arch/$(ARCH)/boot/dts/$(DTB) $(KERNEL_BINARIES_DIR)
	sudo cp $(KERNEL_BINARIES_DIR)/$(DTB) $(TFTP_DIR)

kernel:
	$(MAKE) -C $(KERNEL_SRC_DIR)                  \
		CROSS_COMPILE=$(CROSS_COMPILER_PREFX) \
		ARCH=$(ARCH)                          \
		O=$(KERNEL_BUILD_DIR)                 \
		distclean

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

	$(MAKE) -C $(KERNEL_SRC_DIR)                    \
		CROSS_COMPILE=$(CROSS_COMPILER_PREFX)   \
		ARCH=$(ARCH)                            \
		INSTALL_MOD_PATH=$(TARGET_NFS_ROOT_DIR) \
		O=$(KERNEL_BUILD_DIR) modules_install

# Build all my modules
my_modules: $(MY_MODULES_NAMES)
my_modules_clean: $(MY_MODULES_CLEAN_NAMES)

%.ko:
	@echo "\n\n------- Start build $@ module -------"
	$(MAKE) -C $(MY_MOD_SRC_ROOT_DIR)/$(patsubst %.ko,%,$@) \
		CROSS_COMPILE=$(CROSS_COMPILER_PREFX)           \
		ARCH=$(ARCH)                                    \
		BUILD_DIR=$(TARGET_MOD_DIR_FOR_BUILD) all

	sudo cp $(MY_MOD_SRC_ROOT_DIR)/$(patsubst %.ko,%,$@)/$@             \
		$(MY_MOD_SRC_ROOT_DIR)/$(patsubst %.ko,%,$@)/Module.symvers \
		$(MY_MOD_SRC_ROOT_DIR)/$(patsubst %.ko,%,$@)/modules.order  \
		--target-directory=$(TARGET_MY_MOD_INSTALL_PATH)/$(patsubst %.ko,%,$@)

%.clean:
	@echo "\n\n------- Start $@ -------"
	$(MAKE) -C $(MY_MOD_SRC_ROOT_DIR)/$(patsubst %.clean,%,$@) \
		CROSS_COMPILE=$(CROSS_COMPILER_PREFX)           \
		ARCH=$(ARCH)                                    \
		BUILD_DIR=$(TARGET_MOD_DIR_FOR_BUILD) clean

# Dependencies for right building wigh help different PCs
install_host_deps:
	@./$(SCRIPTS_DIR)/install_host_deps.sh
