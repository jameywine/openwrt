// SPDX-License-Identifier: GPL-2.0-only

/* Setup for the Realtek RTL9607C SoC */

#include <linux/init.h>
#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include <linux/clk.h>
#include <linux/of_fdt.h>
#include <linux/delay.h>
#include <linux/memblock.h>
#include <asm/prom.h>
#include <asm/time.h>
#include <asm/reboot.h>
#include <asm/fw/fw.h>
#include <asm/mips-cps.h>
#include <linux/irqchip.h>

#include <asm/mach-rtl960xc/mach-rtl960xc.h>

extern struct rtl96xx_soc_info soc_info;

// __weak void force_stop_wlan_hw(void) {
//         printk("%s(%d): empty\n",__func__,__LINE__);
// }

// __weak void rtk_pcie_host_shutdown(void) {
// }

extern void PCIE_reset_pin(int *reset);
extern void PCIE1_reset_pin(int *reset);
// static void plat_pcie_shutdown(void) {
// 	__maybe_unused int gpio_rst;
//
// 	/* Enable BTM on Lx1/2 to prevent possible system hang */
// 	soc_w32(0xf0000000, 0x5240);
// 	soc_w32(0xf0000000, 0x5260);
//
// 	//rtk_pcie_host_shutdown();
// 	mdelay(1);
// }

static void plat_machine_restart(char *command)
{
	local_irq_disable();
	printk("System restart...\n");
	//force_stop_wlan_hw();
	//plat_pcie_shutdown();

	while (1) {
		soc_w32(1 << 31, 0x3268);
		mdelay(100);
	}
}

static void plat_machine_halt(void)
{
	printk("System halted.\n");
	//force_stop_wlan_hw();
	//plat_pcie_shutdown();
	while(1);
}

// #define STICK_REGISTER 0xb800121c
// static const u32 reason_mapping[] = { 0xb, 0xa, 0xc, 0xd, 0x9 };
// static void __07c_set_reboot_reason(int reason) {
// 	if ((reason >= 0) && (reason) < ARRAY_SIZE(reason_mapping)) {
// 		writel(reason_mapping[reason], (void *)STICK_REGISTER);
// 	} else {
// 		printk("Unsupported reason %d\n", reason);
// 	}
// }
//
// static int __07c_get_reboot_reason(void) {
// 	int i;
// 	u32 val = readl((void *)STICK_REGISTER);
// 	for (i=0; i<ARRAY_SIZE(reason_mapping); i++) {
// 		if (reason_mapping[i]==val)
// 			return i;
// 	}
// 	return -1;
// }
// void rtk_soc_reboot_reason_set(int reason) {
// 	if (mips_gcr_base) {
// 		__07c_set_reboot_reason(reason);
// 	} else {
// 	}
// }
// EXPORT_SYMBOL(rtk_soc_reboot_reason_set);

// int rtk_soc_reboot_reason_get(void) {
// 	if (mips_gcr_base) {
// 		return __07c_get_reboot_reason();
// 	} else {
// 		return -1;
// 	}
// }

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

	void *dtb;

	dtb = get_fdt();
	if (!dtb)
		panic("no dtb found");

	/*
	 * Load the builtin devicetree. This causes the chosen node to be
	 * parsed resulting in our memory appearing
	 */
	__dt_setup_arch(dtb);
}

void __init plat_time_init(void)
{
	of_clk_init(NULL);
	timer_probe();
}

unsigned int get_c0_compare_int(void)
{
	return gic_get_c0_compare_int();
}

int get_c0_perfcount_int(void)
{
	return gic_get_c0_perfcount_int();
}
EXPORT_SYMBOL_GPL(get_c0_perfcount_int);

void __init arch_init_irq(void)
{
	irqchip_init();
}

