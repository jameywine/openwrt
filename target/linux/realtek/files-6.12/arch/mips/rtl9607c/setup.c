/*
 * Realtek Semiconductor Corp.
 *
 * bsp/setup.c
 *     bsp interrult initialization and handler code
 *
 * Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
 */
#include <linux/of_fdt.h>
#include <linux/delay.h>
#include <linux/memblock.h>
#include <asm/prom.h>
#include <asm/time.h>
#include <asm/reboot.h>
#include <asm/fw/fw.h>
//#include <bspgpio.h>
#include "rtl9607c_regs.h"

extern char __dtb_start[];

__weak void force_stop_wlan_hw(void) {
        printk("%s(%d): empty\n",__func__,__LINE__);
}

__weak void rtk_pcie_host_shutdown(void) {
}

__weak void rtk_nand_reset(void) {
        printk("%s(%d): empty\n",__func__,__LINE__);
}

extern void PCIE_reset_pin(int *reset);
extern void PCIE1_reset_pin(int *reset);
static void plat_pcie_shutdown(void) {
	__maybe_unused int gpio_rst;

	/* Enable BTM on Lx1/2 to prevent possible system hang */
	REG32(0xb8005240) = 0xf0000000;
	REG32(0xb8005260) = 0xf0000000;

	rtk_pcie_host_shutdown();
#if 0
	#ifdef CONFIG_USE_PCIE_SLOT_0
	PCIE_reset_pin(&gpio_rst);
	gpioClear(gpio_rst);
	#endif
	#ifdef CONFIG_USE_PCIE_SLOT_1
	PCIE1_reset_pin(&gpio_rst);
	gpioClear(gpio_rst);
	#endif
#endif
	mdelay(1);
}

static void plat_machine_restart(char *command)
{
	local_irq_disable();
	printk("System restart...\n");
	force_stop_wlan_hw();
	plat_pcie_shutdown();
	rtk_nand_reset();

	while (1) {
		#if 0
		if (mips_gcr_base)
			REG32(0xBB000108) |= (1 << 7);
		else
			REG32(0xBB0000E0) |= (1 << 7);
		#endif
		REG32(0xb8003268) = (1<<31);
		mdelay(100);
	}
}

static void plat_machine_halt(void)
{
	printk("System halted.\n");
	force_stop_wlan_hw();
	plat_pcie_shutdown();
	rtk_nand_reset();
	while(1);
}

#define STICK_REGISTER 0xb800121c
static const u32 reason_mapping[] = { 0xb, 0xa, 0xc, 0xd, 0x9 };
static void __07c_set_reboot_reason(int reason) {
	if ((reason >= 0) && (reason) < ARRAY_SIZE(reason_mapping)) {
		writel(reason_mapping[reason], (void *)STICK_REGISTER);
	} else {
		printk("Unsupported reason %d\n", reason);
	}
}

static int __07c_get_reboot_reason(void) {
	int i;
	u32 val = readl((void *)STICK_REGISTER);
	for (i=0; i<ARRAY_SIZE(reason_mapping); i++) {
		if (reason_mapping[i]==val)
			return i;
	}
	return -1;
}
void rtk_soc_reboot_reason_set(int reason) {
	if (mips_gcr_base) {
		__07c_set_reboot_reason(reason);
	} else {
	}
}
EXPORT_SYMBOL(rtk_soc_reboot_reason_set);

int rtk_soc_reboot_reason_get(void) {
	if (mips_gcr_base) {
		return __07c_get_reboot_reason();
	} else {
		return -1;
	}
}

/* callback function */
void __init plat_mem_setup(void)
{
	/* define io/mem region */
	ioport_resource.start = 0x14000000;
	ioport_resource.end = 0x1fffffff;

	iomem_resource.start = 0x14000000;
	iomem_resource.end = 0x1fffffff;

	/* set reset vectors */
	_machine_restart = plat_machine_restart;
	_machine_halt = plat_machine_halt;
	pm_power_off = plat_machine_halt;

#ifdef CONFIG_BUILTIN_DTB
	/*
	 * Load the builtin devicetree. This causes the chosen node to be
	 * parsed resulting in our memory appearing
	 */
	__dt_setup_arch(&__dtb_start);
#else
	/*
	 * U-Boot will pass $a0 and $a1 to kernel. $a0 is the magic value, -2,
	 * to check whether $a1 is the dtb address or not.
	 */
	if (fw_arg0 == -2)
		__dt_setup_arch((void *)KSEG0ADDR(fw_arg1));
#endif

#if	defined(CONFIG_RTL8686_SWITCH)
	do {
		extern void rtk_core_init(void);
		rtk_core_init();
	} while (0);
#endif
}
