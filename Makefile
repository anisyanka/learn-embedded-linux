.PHONY: bbb_uboot bbb_kernel install_host_deps clean

SCRIPTS_DIR=scripts

all: install_host_deps bbb_uboot bbb_kernel

bbb_uboot:
	@./$(SCRIPTS_DIR)/build_bbb_uboot.sh
	@./$(SCRIPTS_DIR)/copy_uboot_out.sh

bbb_kernel:
	@./$(SCRIPTS_DIR)/build_bbb_kernel.sh
	@./$(SCRIPTS_DIR)/copy_kernel_out.sh

install_host_deps:
	@./$(SCRIPTS_DIR)/install_host_deps.sh
