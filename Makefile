.PHONY: bbb_uboot

SCRIPTS_DIR=scripts

bbb_uboot:
	@./$(SCRIPTS_DIR)/build_bbb_uboot.sh
	@./$(SCRIPTS_DIR)/copy_uboot_bin2out.sh
