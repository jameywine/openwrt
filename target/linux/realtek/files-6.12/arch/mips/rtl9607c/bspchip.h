/*
 * Realtek Semiconductor Corp.
 *
 * bsp/bspchip.h:
 *   bsp chip address and IRQ mapping file
 *
 * Copyright (C) 2006-2015 Tony Wu (tonywu@realtek.com)
 */

#ifndef _BSPCHIP_H_
#define _BSPCHIP_H_

#include <linux/version.h>

#include <asm/mach-generic/irq.h>
#include "irq.h"

/*
 * SPRAM
 */
#define BSP_ISPRAM_BASE		0x0
#define BSP_DSPRAM_BASE		0x0

/*
 * GCMP Specific definitions
 */
#define GCMP_BASE_ADDR		0x1fbf8000
#define GCMP_BASE_SIZE		(256 * 1024)

/*
 * GIC Specific definitions
 */
#define GIC_BASE_ADDR		0x1bdc0000
#define GIC_BASE_SIZE		(128 * 1024)

/*
 * CPC Specific defiitions
 */
#define CPC_BASE_ADDR		0x1bde0000
#define CPC_BASE_SIZE		(24 * 1024)

/*
 * FREQ
 */
#define BSP_PLL_FREQ		25000000	   /* 25 MHz */
#define BSP_CPU_FREQ		(2 * BSP_PLL_FREQ) /* 50 MHz */

/*
 * IRQ Mapping
 *
 * We have two mappings:
 *
 * 1. UMP with ICTL
 * 2. SMP with GIC
 */
#define BSP_IRQ_GIC		(MIPS_CPU_IRQ_BASE + 2)  /* GIC IRQ */
#define BSP_IRQ_IPI_RESCHED	(MIPS_CPU_IRQ_BASE + 3)
#define BSP_IRQ_IPI_CALL	(MIPS_CPU_IRQ_BASE + 4)
#define BSP_IRQ_PERFCOUNT	(MIPS_CPU_IRQ_BASE + 6)
#define BSP_IRQ_TIMER		(MIPS_CPU_IRQ_BASE + 7)  /* external timer */
#define BSP_IRQ_COMPARE		(MIPS_CPU_IRQ_BASE + 7)  /* local timer */

#if defined(CONFIG_IRQ_GIC) || defined(CONFIG_MIPS_GIC)

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0)
#include <asm/mips-cps.h>
#else
#include <linux/irqchip/mips-gic.h>
#endif

#define MIPS_GIC_LOCAL_INTRS	GIC_NUM_LOCAL_INTRS

/* to be removed */
#define BSP_IRQ_UART		(MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + 2)
#define BSP_IRQ_WDT		(MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + 3)
#define BSP_IRQ_RTC		(MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + 4)
#define BSP_IRQ_PMU		(MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + 5)
#define BSP_IRQ_DMAC		(MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + 6)
#define BSP_IRQ_PCIE		(MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + 7)
#define BSP_IRQ_XHCI		(MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + 8)
#define BSP_IRQ_GMAC		(MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + 9)

#elif defined(CONFIG_IRQ_ICTL)
#include <linux/irqchip/irq-dw-ictl.h>

/* ICTL base = 16 */
#define BSP_IRQ_ICTL_BASE	DW_ICTL_BASE
#define BSP_IRQ_WDT		(BSP_IRQ_ICTL_BASE + 3)
#define BSP_IRQ_RTC		(BSP_IRQ_ICTL_BASE + 4)
#define BSP_IRQ_PMU		(BSP_IRQ_ICTL_BASE + 5)
#define BSP_IRQ_DMAC		(BSP_IRQ_ICTL_BASE + 6)
#define BSP_IRQ_PCIE		(BSP_IRQ_ICTL_BASE + 7)
#define BSP_IRQ_XHCI		(BSP_IRQ_ICTL_BASE + 8)
#define BSP_IRQ_GMAC		(BSP_IRQ_ICTL_BASE + 9)

#endif


#define BSP_IP_SEL          0xb8000600UL
#define NEW_BSP_IP_SEL      0xb800063cUL
/*
 * DWAPB UART
 */
//#define BSP_UART0_VADDR		0xbfb03000UL
#define BSP_UART0_VADDR		0xb8002000UL
//#define BSP_UART0_BAUD		57600
#define BSP_UART0_BAUD		115200
#define BSP_UART0_FREQ		(BSP_PLL_FREQ)
#define BSP_UART0_THR		(BSP_UART0_VADDR + 0x00)
#define BSP_UART0_LSR		(BSP_UART0_VADDR + 0x14)
#define BSP_UART0_USR		(BSP_UART0_VADDR + 0x7c)
#define BSP_UART0_FCR		(BSP_UART0_VADDR + 0x08)
//#define BSP_UART0_PADDR		0x1fb03000UL
#define BSP_UART0_PADDR		0x18002000UL
#define BSP_UART0_PSIZE		0x100

#define BSP_UART1_VADDR		0xbfb02e00UL
#define BSP_UART1_BAUD		57600
#define BSP_UART1_FREQ		(BSP_PLL_FREQ)
#define BSP_UART1_USR		(BSP_UART1_VADDR + 0x7c)
#define BSP_UART1_FCR		(BSP_UART1_VADDR + 0x08)
#define BSP_UART1_PADDR		0x1fb02e00UL
#define BSP_UART1_PSIZE		0x100

/*
 * DWAPB Timer
 */
#define BSP_TIMER_FREQ		(BSP_PLL_FREQ)
#define BSP_TIMER_VADDR		0xbfb01000UL
#define BSP_TIMER_TLCR		(BSP_TIMER_VADDR + 0x00)
#define BSP_TIMER_TCVR		(BSP_TIMER_VADDR + 0x04)
#define BSP_TIMER_TCR		(BSP_TIMER_VADDR + 0x08)
#define BSP_TIMER_EOI		(BSP_TIMER_VADDR + 0x0c)


/*
 * Watchdog
 */
#define TC_BASE         0xB8003200
#define BSP_WDTCNTRR          (TC_BASE + 0x60)
#define BSP_WDTINTRR          (BSP_WDTCNTRR + 0x4)
#define BSP_WDTCTRLR          (BSP_WDTCNTRR + 0x8)
	#define WDT_E                         (1<<31)           /* Watchdog enable */
	#define WDT_KICK                      (0x1 << 31)       /* Watchdog kick */
	#define WDT_PH12_TO_MSK               31               /* 11111b */
	#define WDT_PH1_TO_SHIFT              22
	#define WDT_PH2_TO_SHIFT              15
	#define WDT_CLK_SC_SHIFT              29


/*
 * LS GMAC
 */
#define BSP_GMAC_PADDR		0x1b007000UL
#define BSP_GMAC_PSIZE		0x00000400UL

/*
 * LS PCIe
 */
#define BSP_PCIE_RC_CFG		0xbb000000UL
#define BSP_PCIE_EP_CFG		0xb9010000UL
#define BSP_PCIE_IO_PADDR	0x19200000UL
#define BSP_PCIE_IO_PSIZE	0x00200000UL
#define BSP_PCIE_MEM_PADDR	0x19400000UL
#define BSP_PCIE_MEM_PSIZE	0x00c00000UL

/*
 * LS USB3
 */
#define BSP_XHCI_PADDR		0x18000000UL
#define BSP_XHCI_PSIZE		0x00100000UL

/*
 * FLASH
 */
#define BSP_SPIC_VADDR		0xbfb09000UL
#define BSP_FLASH_PADDR		0x1d000000UL
#define BSP_FLASH_PSIZE		0x200000UL

/*
 * LS SMU
 */
#define BSP_SMU_VADDR		0xbb007800UL
#define BSP_SMU_PADDR		0x1b007800UL
#define BSP_SMU_PSIZE		0x00000800UL

#ifdef CONFIG_SDK_FPGA_PLATFORM
	#define MHZ             40 //25
	#define BSP_MHZ			MHZ
	#define BSP_SYSCLK          MHZ * 1000 * 1000
	#if defined(CONFIG_CEVT_R4K)
		/* For R4K on FPGA  */
		#define BSP_CPU0_FREQ           MHZ*1000000     /* 80 MHz */
	#endif
#else
	extern unsigned int BSP_MHZ;
	extern unsigned int BSP_SYSCLK;
#endif

/*
 * Register access macro
 */
#ifndef REG32
#define REG32(reg)	(*(volatile unsigned int   *)(reg))
#endif
#ifndef REG16
#define REG16(reg)	(*(volatile unsigned short *)(reg))
#endif
#ifndef REG08
#define REG08(reg)	(*(volatile unsigned char  *)(reg))
#endif

/*
*	SRAM DRAM control registers
*/
//	CPU0
//	Unmaped Memory Segment Address Register
#define R_C0UMSAR0_BASE		(0xB8001300) /* DRAM UNMAP */
//#ifdef CONFIG_LUNA_USE_SRAM
#define R_C0UMSAR0 			(R_C0UMSAR0_BASE + 0x00)
#define R_C0UMSAR1 			(R_C0UMSAR0_BASE + 0x10)
#define R_C0UMSAR2 			(R_C0UMSAR0_BASE + 0x20)
#define R_C0UMSAR3 			(R_C0UMSAR0_BASE + 0x30)

#define R_C0SRAMSAR0_BASE	(0xB8004000)
#define R_C0SRAMSAR0		(R_C0SRAMSAR0_BASE + 0x00)
#define R_C0SRAMSAR1		(R_C0SRAMSAR0_BASE + 0x10)
#define R_C0SRAMSAR2		(R_C0SRAMSAR0_BASE + 0x20)
#define R_C0SRAMSAR3		(R_C0SRAMSAR0_BASE + 0x30)
//#endif

/* For DRAM controller */
#define C0DCR           (0xB8001004)

/*
 * Logical addresses on OCP buses
 * are partitioned into three zones, Zone 0 ~ Zone 2.
 */
/* ZONE0 */
#define  BSP_CDOR0         0xB8004200
#define  BSP_CDMAR0       (BSP_CDOR0 + 0x04)
/* ZONE1 */
#define  BSP_CDOR1         (BSP_CDOR0 + 0x10)
#define  BSP_CDMAR1       (BSP_CDOR0 + 0x14)
/* ZONE2 */
#define  BSP_CDOR2         (BSP_CDOR0 + 0x20)
#define  BSP_CDMAR2       (BSP_CDOR0 + 0x24)

#if defined(CONFIG_RTK_SOC_RTL8198D) || defined(CONFIG_RTK_SOC_RTL9607C)
#include "bspchip_9607c.h"

#define WRITE_MEM32(addr, val)   (*(volatile unsigned int *)   (addr)) = (val)
#define READ_MEM32(addr)         (*(volatile unsigned int *)   (addr))
#define WRITE_MEM16(addr, val)   (*(volatile unsigned short *) (addr)) = (val)
#define READ_MEM16(addr)         (*(volatile unsigned short *) (addr))
#define WRITE_MEM8(addr, val)    (*(volatile unsigned char *)  (addr)) = (val)
#define READ_MEM8(addr)          (*(volatile unsigned char *)  (addr))
// #else
// #error "err!"
#endif /* defined(CONFIG_RTK_SOC_RTL8198D) */

#endif   /* _BSPCHIP_H */
