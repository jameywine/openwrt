/*
 * Realtek Semiconductor Corp.
 *
 * bsp/timer.c
 *     bsp timer initialization code
 *
 * Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
 */
#include <linux/version.h>
#include <linux/init.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0)
#include <asm/mips-cps.h>
#else
#include <linux/irqchip/mips-gic.h>
#endif

#include "rtl9607c_regs.h"

#ifndef CAUSEF_DC
#define CAUSEF_DC	(_ULCAST_(1)   << 27)
#endif /* ! CAUSEF_DC */

unsigned int get_c0_compare_int(void)
{
#ifdef CONFIG_MIPS_GIC
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,14,0)
	if (gic_present)
#endif
		return gic_get_c0_compare_int();
#endif

	return BSP_IRQ_COMPARE;
}

int get_c0_perfcount_int(void)
{
#ifdef CONFIG_MIPS_GIC
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,14,0)
	if (gic_present)
#endif
		return gic_get_c0_perfcount_int();
#endif

	return BSP_IRQ_COMPARE;
}

EXPORT_SYMBOL_GPL(get_c0_perfcount_int);

#ifdef CONFIG_MIPS_GIC
#ifdef CONFIG_OF
#include <linux/version.h>
#include <linux/of.h>
#include <linux/clk-provider.h>
#include <linux/clocksource.h>
#endif

static void __init mips_gic_time_init(void)
{
#ifdef CONFIG_OF
	of_clk_init(NULL);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,13,0)
	timer_probe();
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(4,4,0)
	clocksource_probe();
#else
	clocksource_of_init();
#endif
#else
	gic_clocksource_init(BSP_CPU_FREQ);
#endif /* CONFIG_OF */
}
#else
static void __init mips_gic_time_init(void)
{
}
#endif /* CONFIG_MIPS_GIC */

#ifdef CONFIG_CEVT_R4K
#include <asm/time.h>

static void __init cevt_r4k_time_init(void)
{
	/* set cp0_compare_irq and cp0_perfcount_irq */
	cp0_compare_irq = get_c0_compare_int();
	cp0_perfcount_irq = get_c0_perfcount_int();

	if (cp0_perfcount_irq == cp0_compare_irq)
		cp0_perfcount_irq = -1;

	mips_hpt_frequency = BSP_CPU_FREQ / 2;

	write_c0_count(0);
	clear_c0_cause(CAUSEF_DC);
}
#else
static void __init cevt_r4k_time_init(void)
{
}
#endif /* CONFIG_CEVT_R4K */

void __init plat_time_init(void)
{
	cevt_r4k_time_init();
	mips_gic_time_init();
}
