#ifndef __RTL9607C_REGS_H
#define __RTL9607C_REGS_H

/* GIC */
#define GIC_BASE_ADDR		0x1bdc0000
#define GIC_BASE_SIZE		(128 * 1024)

/*
 * DWAPB UART
 */
#define BSP_UART0_VADDR		0xb8002000
#define BSP_UART0_BAUD		115200
#define BSP_UART0_FREQ		(BSP_PLL_FREQ)
#define BSP_UART0_THR		(BSP_UART0_VADDR + 0x00)
#define BSP_UART0_LSR		(BSP_UART0_VADDR + 0x14)
#define BSP_UART0_USR		(BSP_UART0_VADDR + 0x7c)
#define BSP_UART0_FCR		(BSP_UART0_VADDR + 0x08)
#define BSP_UART0_PADDR		0x18002000
#define BSP_UART0_PSIZE		0x100

/* PCIe */
#define BSP_PCIE_RC_CFG		0xbb000000UL
#define BSP_PCIE_EP_CFG		0xb9010000UL

/* FREQ */
#define BSP_PLL_FREQ		25000000	   /* 25 MHz */
#define BSP_CPU_FREQ		(2 * BSP_PLL_FREQ) /* 50 MHz */

#ifndef MIPS_CPU_IRQ_BASE
#define MIPS_CPU_IRQ_BASE 0
#endif

#define BSP_IRQ_GIC		(MIPS_CPU_IRQ_BASE + 2)  /* GIC IRQ */
#define BSP_IRQ_IPI_RESCHED	(MIPS_CPU_IRQ_BASE + 3)
#define BSP_IRQ_IPI_CALL	(MIPS_CPU_IRQ_BASE + 4)
#define BSP_IRQ_PERFCOUNT	(MIPS_CPU_IRQ_BASE + 6)
#define BSP_IRQ_TIMER		(MIPS_CPU_IRQ_BASE + 7)  /* external timer */
#define BSP_IRQ_COMPARE		(MIPS_CPU_IRQ_BASE + 7)  /* local timer */


#define BSP_IP_SEL          0xb8000600
#define NEW_BSP_IP_SEL      0xb800063c

/* Register access macro */
#ifndef REG32
#define REG32(reg)	(*(volatile unsigned int   *)(reg))
#endif
#ifndef REG16
#define REG16(reg)	(*(volatile unsigned short *)(reg))
#endif
#ifndef REG08
#define REG08(reg)	(*(volatile unsigned char  *)(reg))
#endif

/* SRAM DRAM control registers */

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

extern unsigned int BSP_MHZ;
extern unsigned int BSP_SYSCLK;

#endif /* __RTL9607C_REGS_H */
