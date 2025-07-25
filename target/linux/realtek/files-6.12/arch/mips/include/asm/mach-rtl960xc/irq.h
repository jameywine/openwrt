/*
 * Realtek Semiconductor Corp.
 *
 * rtl8198d/irq.h:
 *     rtl8198d IRQ mapping file
 *
 */
#ifndef __ASM_MIPS_IA_MACH_REALTEK_IRQ_H
#define __ASM_MIPS_IA_MACH_REALTEK_IRQ_H

#if defined(CONFIG_MIPS_GIC)
#include <asm/mips-cps.h>
#include <asm/mips-gic.h>
#define MIPS_GIC_LOCAL_INTRS	GIC_NUM_LOCAL_INTRS
#endif /*  defined(CONFIG_IRQ_GIC) || defined(CONFIG_MIPS_GIC) */

#ifndef NR_IRQS
#define NR_IRQS 128
#endif

#ifndef MIPS_CPU_IRQ_BASE
#define MIPS_CPU_IRQ_BASE 0
#endif

#ifndef MIPS_GIC_IRQ_BASE
#define MIPS_GIC_IRQ_BASE (MIPS_CPU_IRQ_BASE + 16)
#endif

#ifndef MIPS_GIC_LOCAL_INTRS
#define MIPS_GIC_LOCAL_INTRS 7
#endif

#include <asm/mach-generic/irq.h>

/***************************************************
 *  GIC Interrupt External Source
 * ************************************************/
/* ApolloPro Spec
Index i	Interrupt Source	Description
0.		TC0	Timer/Counter #0 interrupt
1.		TC1	Timer/Counter #1 interrupt
2.		TC2	Timer/Counter #2 interrupt
3.		TC3	Timer/Counter #3 interrupt
4.		TC4	Timer/Counter #4 interrupt
5.		TC5	Timer/Counter #5 interrupt
6.		TC6	Timer/Counter #6 interrupt
7.		TC7	Timer/Counter #7 interrupt
8.		TC8	Timer/Counter #8 interrupt
9.		PCM0	PCM0 interrupt
10.		PCM1	PCM1 interrupt
11.		VOIPXSI	VOIP XSI interrupt
12.		VOIPSPI	SPI for VoIP SLIC interrupt
13.		VOIPACC	VOIPACC interrupt
14.		FFTACC	FFTACC interrupt
15.		SWCORE	Switch Core interrupt
16.		GMAC0	GMAC#0 interrupt
17.		GMAC1	GMAC#1 interrupt
18.		GMAC2	GMAC#2 interrupt
19.		EXT_GPIO0	Ext. GPIO 0 interrupt
20.		EXT_GPIO1	Ext. GPIO 1 interrupt
21.		GPIO_ABCD	GPIO_ABCD interrupt
22.		GPIO_EFGH	GPIO_EFGH interrupt
23.		UART0 	UART0 interrupt
24.		UART1	UART1 interrupt
25.		UART2	UART2 interrupt
26.		UART3	UART3 interrupt
27.		TC0_DELAY_INT	Delayed interrupt with Timer/Counter 0
28.		TC1_DELAY_INT	Delayed interrupt with Timer/Counter 1
29.		TC2_DELAY_INT	Delayed interrupt with Timer/Counter 2
30.		TC3_DELAY_INT	Delayed interrupt with Timer/Counter 3
31.		TC4_DELAY_INT	Delayed interrupt with Timer/Counter 4
32.		TC5_DELAY_INT	Delayed interrupt with Timer/Counter 5
33.		WDT_PH1TO	Watchdog Timer Phase 1 Timeout interrupt
34.		WDT_PH2TO	Watchdog Timer Phase 2 Timeout interrupt
35.		OCP0TO	OCP0 bus time-out interrupt
36.		OCP1TO	OCP1 bus time-out interrupt
37.		LXPBOTO	LX PBO bus time-out interrupt
38.		LXMTO	LX Master 0/1/2/3 time-out interrupt
39.		LXSTO	LX Slave 0/1/2/3/P time-out interrupt
40.		OCP0_ILA	OCP0 illegal addr. interrupt
41.		OCP1_ILA	OCP1 illegal addr. interrupt
42.		PONNIC	PONNIC interrupt
43.		PONNIC_DS	PONNIC_DS interrupt
44.		BTG	BTG/GDMA interrupts from LX0/1/2/3 & PBO_D(U)S_R(W)
45.		CPI_P1	Cross-Processor interrupt P1
46.		CPI_P2	Cross-Processor interrupt P2
47.		CPI_P3	Cross-Processor interrupt P3
48.		CPI_P4	Cross-Processor interrupt P4
49.		PCIE0	PCIE0 interrupt
50.		PCIE1	PCIE1 interrupt
51.		USB_2	USB 2.0 interrupt
52.		USB_23	USB 2/3 interrupt
53.		P_NAND	Parallel NAND flash DMA interrupt
54.		SPI_NAND	SPI-NAND DMA interrupt
55.		ECC	ECC Controller interrupt

*/
/************************************************************************
* GIC External IRQ Source Num
* Physical layout
************************************************************************/
#define GIC_EXT_TC0         0
#define GIC_EXT_TC1         1
#define GIC_EXT_TC2         2
#define GIC_EXT_TC3         3
#define GIC_EXT_TC4         4
#define GIC_EXT_TC5         5
#define GIC_EXT_TC6         6
#define GIC_EXT_TC7         7
#define GIC_EXT_TC8         8
#define GIC_EXT_PCM0        9

#define GIC_EXT_PCM1        10
#define GIC_EXT_VOIPXSI     11
#define GIC_EXT_VOIPSPI     12
#define GIC_EXT_VOIPACC     13
#define GIC_EXT_FFTACC      14
#define GIC_EXT_SWCORE      15
#define GIC_EXT_RESERVE16   16
#define GIC_EXT_USBHOSTP2   17
#define GIC_EXT_GPIO_JKMN   18
#define GIC_EXT_RESERVE19   19

#define GIC_EXT_RESERVE20   20
#define GIC_EXT_GPIO_ABCD   21
#define GIC_EXT_GPIO_EFGH   22
#define GIC_EXT_UART0       23
#define GIC_EXT_UART1       24
#define GIC_EXT_UART2       25
#define GIC_EXT_UART3       26
#define GIC_EXT_RESERVE27   27
#define GIC_EXT_RESERVE28   28
#define GIC_EXT_RESERVE29   29

#define GIC_EXT_RESERVE30     30
#define GIC_EXT_RESERVE31     31
#define GIC_EXT_RESERVE32     32
#define GIC_EXT_WDT_PH1TO     33
#define GIC_EXT_WDT_PH2TO     34
#define GIC_EXT_NAND          35
#define GIC_EXT_INT0_GMAC0    36
#define GIC_EXT_INT1_GMAC0    37
#define GIC_EXT_INT0_GMAC1    38
#define GIC_EXT_INT1_GMAC1    39

#define GIC_EXT_INT0_GMAC2      40
#define GIC_EXT_INT1_GMAC2      41
#define GIC_EXT_PONNIC          42
#define GIC_EXT_PONNIC_DS       43
#define GIC_EXT_DBG_LX_BUS_IP   44
#define GIC_EXT_CPI_P1          45
#define GIC_EXT_CPI_P2          46
#define GIC_EXT_CPI_P3          47
#define GIC_EXT_CPI_P4          48
#define GIC_EXT_PCIE_11AC       49

#define GIC_EXT_PCIE_11N           50
#define GIC_EXT_USB_2              51
#define GIC_EXT_USB_23             52
#define GIC_EXT_ECC                53
#define GIC_EXT_DSP_TO_CPU_IE      54
#define GIC_EXT_CPU_TO_DSP_IE      55
/* Reserve 56 ~ 63, Usedd for SMP'S IPI ,4 CPUs */

/************************************************************************
* BSP External IRQ Num
* ADD IRQ BASE Num
* MIPS_GIC_IRQ_BASE = GIC_IRQ_BASE = 16, defined in <include/asm/mach-generic/irq.h>
***********************************************************************/
#define BSP_TC0_IRQ             ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_TC0              )
#define BSP_TC1_IRQ             ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_TC1              )
#define BSP_TC2_IRQ             ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_TC2              )
#define BSP_TC3_IRQ             ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_TC3              )
#define BSP_TC4_IRQ             ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_TC4              )
#define BSP_TC5_IRQ             ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_TC5              )
#define BSP_TC6_IRQ             ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_TC6              )
#define BSP_TC7_IRQ             ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_TC7              )
#define BSP_TC8_IRQ             ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_TC8              )
#define BSP_PCM0_IRQ            ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_PCM0             )
#define BSP_PCM1_IRQ            ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_PCM1             )
#define BSP_VOIPXSI_IRQ         ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_VOIPXSI          )
#define BSP_VOIPSPI_IRQ         ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_VOIPSPI          )
#define BSP_VOIPACC_IRQ         ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_VOIPACC          )
#define BSP_FFTACC_IRQ          ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_FFTACC           )
#define BSP_SWCORE_IRQ          ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_SWCORE           )
#define BSP_RESERVE16_IRQ       ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_RESERVE16        )
#define BSP_USBHOSTP2_IRQ       ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_USBHOSTP2        )
#define BSP_GPIO_JKMN_IRQ       ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_GPIO_JKMN        )
#define BSP_RESERVE19_IRQ       ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_RESERVE19        )
#define BSP_RESERVE20_IRQ       ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_RESERVE20        )
#define BSP_GPIO_ABCD_IRQ       ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_GPIO_ABCD        )
#define BSP_GPIO_EFGH_IRQ       ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_GPIO_EFGH        )
#define BSP_UART0_IRQ           ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_UART0            )
#define BSP_UART1_IRQ           ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_UART1            )
#define BSP_UART2_IRQ           ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_UART2            )
#define BSP_UART3_IRQ           ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_UART3            )
#define BSP_RESERVE27_IRQ       ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_RESERVE27        )
#define BSP_RESERVE28_IRQ       ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_RESERVE28        )
#define BSP_RESERVE29_IRQ       ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_RESERVE29        )
#define BSP_RESERVE30_IRQ       ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_RESERVE30        )
#define BSP_RESERVE31_IRQ       ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_RESERVE31        )
#define BSP_RESERVE32_IRQ       ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_RESERVE32        )
#define BSP_WDT_PH1TO_IRQ       ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_WDT_PH1TO        )
#define BSP_WDT_PH2TO_IRQ       ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_WDT_PH2TO        )
#define BSP_NAND_IRQ            ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_NAND             )
#define BSP_INT0_GMAC0_IRQ      ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_INT0_GMAC0       )
#define BSP_INT1_GMAC0_IRQ      ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_INT1_GMAC0       )
#define BSP_INT0_GMAC1_IRQ      ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_INT0_GMAC1       )
#define BSP_INT1_GMAC1_IRQ      ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_INT1_GMAC1       )
#define BSP_INT0_GMAC2_IRQ      ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_INT0_GMAC2       )
#define BSP_INT1_GMAC2_IRQ      ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_INT1_GMAC2       )
#define BSP_PONNIC_IRQ          ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_PONNIC           )
#define BSP_PONNIC_DS_IRQ       ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_PONNIC_DS        )
#define BSP_DBG_LX_BUS_IP_IRQ   ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_DBG_LX_BUS_IP    )
#define BSP_CPI_P1_IRQ          ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_CPI_P1           )
#define BSP_CPI_P2_IRQ          ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_CPI_P2           )
#define BSP_CPI_P3_IRQ          ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_CPI_P3           )
#define BSP_CPI_P4_IRQ          ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_CPI_P4           )
#define BSP_PCIE_11AC_IRQ       ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_PCIE_11AC        )
#define BSP_PCIE_11N_IRQ        ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_PCIE_11N         )
#define BSP_USB_2_IRQ           ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_USB_2            )
#define BSP_USB_23_IRQ          ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_USB_23           )
#define BSP_ECC_IRQ             ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_ECC              )
#define BSP_DSP_TO_CPU_IE_IRQ   ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_DSP_TO_CPU_IE    )
#define BSP_CPU_TO_DSP_IE_IRQ   ( MIPS_GIC_IRQ_BASE + MIPS_GIC_LOCAL_INTRS + GIC_EXT_CPU_TO_DSP_IE    )

#endif /* __ASM_MIPS_IA_MACH_REALTEK_IRQ_H */
