# SPDX-License-Identifier: GPL-2.0-only
ARCH:=mips
SUBTARGET:=rtl9607c
CPU_TYPE:=mips32r2
BOARD:=realtek
BOARDNAME:=Realtek MIPS RTL9607C

KERNEL_PATCHVER:=6.12

define Target/Description
	Build firmware images for Realtek RTL9607C based boards.
endef

