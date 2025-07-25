/*
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 *
 *  Copyright (C) 2015, Tony Wu <tonywu@realtek.com>
 */

#include <linux/io.h>
#include <linux/serial_reg.h>
#include <asm/addrspace.h>

#include "rtl9607c_regs.h"

static __iomem void *uart_base = (__iomem void *) KSEG1ADDR(BSP_UART0_PADDR);

static inline void uart_w32(u32 val, unsigned reg)
{
	__raw_writel(val, uart_base + (reg << 2));
}

static inline u32 uart_r32(unsigned reg)
{
	return __raw_readl(uart_base + (reg << 2));
}

void early_uart_init(void)
{
	uart_w32(0x80, UART_LCR);
	uart_w32(0x0, UART_IER);
	/*
	 *  baud rate = (serial clock freq) / (16 * divisor)
	 *  baud rate = 57600
	 *  serial clock freq = 25MHz
	 */
	uart_w32(0x1b, UART_TX);
	uart_w32(0x3, UART_LCR);
}

#define REG8(reg)               (*(volatile unsigned char *)((unsigned int)reg))
#define BSP_TxCHAR_AVAIL	0x00
#define BSP_TXRST		0x04
#define BSP_CHAR_TRIGGER_14	0xC0
#define BSP_LSR_THRE		0x20
#define _UART_THR		BSP_UART0_THR
#define _UART_FCR		BSP_UART0_FCR
#define _UART_LSR		BSP_UART0_LSR

void prom_putchar(unsigned char c)
{
	unsigned int busy_cnt = 0;

#ifdef CONFIG_RTL867X_NETLOG
	 netlog_emit_char(c);
#endif

	do {
		/* Prevent Hanging */
		if (busy_cnt++ >= 30000) {
			/* Reset Tx FIFO */
			REG8(_UART_FCR) = BSP_TXRST | BSP_CHAR_TRIGGER_14;
			return ;
		}
	} while ((REG8(_UART_LSR) & BSP_LSR_THRE) == BSP_TxCHAR_AVAIL);

	/* Send Character */
	REG8(_UART_THR) = c;
}
