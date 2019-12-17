.PHONY: bbb_uboot install_host_deps

SCRIPTS_DIR=scripts

bbb_uboot:
	@./$(SCRIPTS_DIR)/build_bbb_uboot.sh
	@./$(SCRIPTS_DIR)/copy_uboot_bin2out.sh

install_host_deps:
	@./$(SCRIPTS_DIR)/install_host_deps.sh
