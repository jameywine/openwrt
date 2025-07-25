/*
 * Realtek Semiconductor Corp.
 *
 * bsp/irq.c
 *     platform interrupt init and handler code
 *     using device tree framework
 *
 * Copyright (C) 2006-2015 Tony Wu (tonywu@realtek.com)
 */
#include <linux/irqchip.h>

#include "irq-ipi.c"

//void __init plat_irq_init(void)
void __init arch_init_irq(void)
{
	irqchip_init();
	plat_ipi_init();
}
