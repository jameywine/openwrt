# SPDX-License-Identifier: GPL-2.0
#
# Realtek SoC drivers
#

obj-y += rtk_proc.o reboot_reason.o
obj-$(CONFIG_NET) += netdev_reset.o
ifeq ($(CONFIG_RTK_PROC_GPIO_FILES),y)
obj-y += rtk_gpio.o
obj-y += pushbutton.o led-generic.o
obj-$(CONFIG_WPS_LED) += led-wps.o
obj-$(CONFIG_E8B) += led-power.o
obj-$(CONFIG_RTL8672_SW_USB_LED)    += led-usb.o
obj-$(CONFIG_RTL9607C) += soc_rtl960xc.o
endif
obj-$(CONFIG_RTL9607C) += board_rtl960xc.o

ifdef CONFIG_PSTORE_RAM
obj-$(CONFIG_RTL9607C) += rtk_pstore_9607c.o sram.o
endif
obj-$(CONFIG_GPIO_RESET_CONTROLLER) += vbus_chain.o
