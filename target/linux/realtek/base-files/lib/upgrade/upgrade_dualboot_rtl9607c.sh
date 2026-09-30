# TODO comments about whole mechanism
# - sw_tryactive
# - sw_active
# - sw_commit

rtl9607c_get_spare_part() {
	local active="$(fw_printenv -n sw_active)"
	if [[ "$active" == 0 ]]; then
		echo 1
	else
		echo 0
	fi
}

rtl9607c_set_boot_vol() {
	fw_setenv sw_tryactive "$1"
}

platform_do_upgrade_dualboot_rtl9607c() {
	local spare_num="$(rtl9607c_get_spare_part)"
	CI_KERNPART="ubi_k$spare_num"
	CI_ROOTPART="ubi_r$spare_num"
	nand_do_flash_file "$1" || nand_do_upgrade_failed
	rtl9607c_set_boot_vol "$spare_num"
	nand_do_upgrade_success
}

