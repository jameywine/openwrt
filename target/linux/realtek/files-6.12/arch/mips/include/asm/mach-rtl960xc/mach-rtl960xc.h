/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _MACH_RTL960XC_H_
#define _MACH_RTL960XC_H_

#include <asm/types.h>

/* Register access macro */

#define RTL960XX_SWCORE_BASE	((volatile void *) 0xBB000000)
#define RTL960XX_SOC_BASE       ((volatile void *) 0xB8000000)

#define rtl9607c_r32(reg) readl(reg)
#define rtl9607c_w32(val, reg)	writel(val, reg)
#define rtl9607c_w32_mask(clear, set, reg) rtl9607c_w32((rtl9607c_r32(reg) & ~(clear)) | (set), reg)

#define rtl9607c_r8(reg)		readb(reg)
#define rtl9607c_w8(val, reg)	writeb(val, reg)

#define sw_r32(reg)		readl(RTL960XX_SWCORE_BASE + reg)
#define sw_w32(val, reg)    writel(val, RTL960XX_SWCORE_BASE + reg)
#define sw_w32_mask(clear, set, reg)	sw_w32((sw_r32(reg) & ~(clear)) | (set), reg)

#define soc_r32(reg)		readl(RTL960XX_SOC_BASE + reg)
#define soc_w32(val, reg)    writel(val, RTL960XX_SOC_BASE + reg)
#define soc_w32_mask(clear, set, reg)	soc_w32((soc_r32(reg) & ~(clear)) | (set), reg)

/* SRAM */

#define RTL9607C_ISPRAM_BASE		0x0
#define RTL9607C_DSPRAM_BASE		0x0

/* CPC Specific defiitions */

#define CPC_BASE_ADDR		0x1bde0000
#define CPC_BASE_SIZE		(24 * 1024)

/* Zones */

#define ZONE1_SIZE    0x10000000
#define ZONE2_BASE    0x20000000
#define ZONE2_PHY     0x10000000
#define ZONE2_OFF     (0x80000000 + ZONE2_PHY - ZONE2_BASE)
#define ZONE2_MAX     ((256<<20) -1)

/* Memory Controller */

#define RTL9607C_MC_BASE			(0xB8001000)
#define RTL9607C_MC_MTCR0			(RTL9607C_MC_BASE + 0x04)
#define RTL9607C_MC_MTCR1			(RTL9607C_MC_BASE + 0x08)
#define RTL9607C_MC_PFCR			(RTL9607C_MC_BASE + 0x010)

/* GPIO */

#define RTL9607C_GPIO_PIN_NUM 96
#define RTL9607C_GPIO_INTR_NUM 96
#define RTL9607C_GPIO_BASE ((volatile void *) 0xb8003300)

#define RTL9607C_GPIO_CTRL_ABCD	RTL9607C_GPIO_BASE + 0x0  /*enable gpio function*/
#define RTL9607C_GPIO_CTRL_EFGH	RTL9607C_GPIO_BASE + 0x1c /*enable gpio function*/
#define RTL9607C_GPIO_CTRL_JKMN	RTL9607C_GPIO_BASE + 0x40 /*enable gpio function*/
#define RTL9607C_GPIO_DIR_ABCD	RTL9607C_GPIO_BASE + 0x08 /*configure gpio pin to gpo or gpi*/
#define RTL9607C_GPIO_DIR_EFGH	RTL9607C_GPIO_BASE + 0x24 /*configure gpio pin to gpo or gpi*/
#define RTL9607C_GPIO_DIR_JKMN	RTL9607C_GPIO_BASE + 0x48 /*configure gpio pin to gpo or gpi*/
#define RTL9607C_GPIO_DATA_ABCD	RTL9607C_GPIO_BASE + 0x0c /*datatit for input/output mode*/
#define RTL9607C_GPIO_DATA_EFGH	RTL9607C_GPIO_BASE + 0x28 /*datatit for input/output mode*/
#define RTL9607C_GPIO_DATA_JKMN	RTL9607C_GPIO_BASE + 0x4c /*datatit for input/output mode*/
#define RTL9607C_GPIO_IMS_ABCD	RTL9607C_GPIO_BASE + 0x10 /*interrupt status */
#define RTL9607C_GPIO_IMS_EFGH	RTL9607C_GPIO_BASE + 0x2c /*interrupt status */
#define RTL9607C_GPIO_IMS_JKMN	RTL9607C_GPIO_BASE + 0x50 /*interrupt status */
#define RTL9607C_GPIO_IMR_AB	RTL9607C_GPIO_BASE + 0x14 /*interrupt mask register AB */
#define RTL9607C_GPIO_IMR_CD	RTL9607C_GPIO_BASE + 0x18 /*interrupt mask register CD*/
#define RTL9607C_GPIO_IMR_EF	RTL9607C_GPIO_BASE + 0x30 /*interrupt mask register EF*/
#define RTL9607C_GPIO_IMR_GH	RTL9607C_GPIO_BASE + 0x34 /*interrupt mask register GH*/
#define RTL9607C_GPIO_IMR_JK	RTL9607C_GPIO_BASE + 0x54 /*interrupt mask register EF*/
#define RTL9607C_GPIO_IMR_MN	RTL9607C_GPIO_BASE + 0x58 /*interrupt mask register GH*/
#define RTL9607C_GPIO_CPU0_ABCD	RTL9607C_GPIO_BASE + 0x38 /*configure gpio imr to cpu 0*/
#define RTL9607C_GPIO_CPU1_ABCD	RTL9607C_GPIO_BASE + 0x3c /*configure gpio imr to cpu 1*/

/* Reset */

#define RTL9607C_SOFTWARE_RST (0x108)

/* MDIO via Realtek's SMI interface */

#define RTL9607C_SMI_INDRT_ACCESS_CTRL_0	(0x230B8)
#define RTL9607C_SMI_INDRT_ACCESS_CTRL_1	(0x230BC)
#define RTL9607C_SMI_INDRT_ACCESS_CTRL_2	(0x230C0)
#define RTL9607C_SMI_INDRT_ACCESS_CTRL_3	(0x230C4)
#define RTL9607C_SMI_INDRT_ACCESS_BC_PHYID_CTRL (0x230C8)
#define RTL9607C_SMI_INDRT_ACCESS_MMD_CTRL (0x230CC)

/* Switch interrupts */

#define RTL9607C_INTR_CTRL	(0x1D000)
#define RTL9607C_INTR_IMR	(0x1100)
#define RTL9607C_INTR_IMS	(0x1104)

/* Basic SoC Features */

#define RTL9607C_CPU_PORT	9

/* IO Access Layer */

#define SWCORE_PHYS_BASE    0x1B000000
#define SOC_PHYS_BASE       0x18000000

#define SWCORE_MEM_SIZE     0x2000000
#define SOC_MEM_SIZE        0x5000
#define SRAM_MEM_SIZE       0x8000
#define DMA_MEM_SIZE        0x100000

/* Definition chip attribute flags - bit-wise */
#define CHIP_AFLAG_PCI      (0x1 << 0)
#define CHIP_AFLAG_LEXRA    (0x1 << 1)

/* Definition of chip and family IDs */
#define RTL9607C_CHIP_ID        (0x96070001)
#define RTL9607_FAMILY_ID		(0x9607)

#define RTL960XC_MODEL_NAME_INFO		(0x10000)
#define RTL960XX_CHIP_INFO (0x10004)

#define RTL9607C_CHIP_SUBTYPE_INFO	(0x23024)

/* Chip Subtypes for RTL9607C */

#define RTL9607C_CHIP_SUB_TYPE_RTL9607CP        0x13
#define RTL9607C_CHIP_SUB_TYPE_RTL9607EP        0x11
#define RTL9607C_CHIP_SUB_TYPE_RTL9603CT        0x00
#define RTL9607C_CHIP_SUB_TYPE_RTL9607C_VA5     0x15
#define RTL9607C_CHIP_SUB_TYPE_RTL9607C_VA6     0x18
#define RTL9607C_CHIP_SUB_TYPE_RTL9607C_VA7     0x1c
#define RTL9607C_CHIP_SUB_TYPE_RTL9607E_VA5     0x14
#define RTL9607C_CHIP_SUB_TYPE_RTL9603C_VA4     0x01
#define RTL9607C_CHIP_SUB_TYPE_RTL9603C_VA5     0x04
#define RTL9607C_CHIP_SUB_TYPE_RTL9603C_VA6     0x08
#define RTL9607C_CHIP_SUB_TYPE_RTL9603CW_VA6    0x19
#define RTL9607C_CHIP_SUB_TYPE_RTL9603CW_VA7    0x1d
#define RTL9607C_CHIP_SUB_TYPE_RTL9603CP        0x10
#define RTL9607C_CHIP_SUB_TYPE_RTL9603CE        0x09
#define RTL9607C_CHIP_SUB_TYPE_RTL8198D_VE5     0x16
#define RTL9607C_CHIP_SUB_TYPE_RTL8198D_VE6     0x1a
#define RTL9607C_CHIP_SUB_TYPE_RTL8198D_VE7     0x1e
#define RTL9607C_CHIP_SUB_TYPE_RTL9606C_VA5     0x17
#define RTL9607C_CHIP_SUB_TYPE_RTL9606C_VA6     0x1b
#define RTL9607C_CHIP_SUB_TYPE_RTL9606C_VA7     0x1f

/* System MISC Control Register */

#define BSP_IP_SEL          0xb8000600UL
#define NEW_BSP_IP_SEL      0xb800063cUL

/* Some IRQ controller definitions */

#define RTL_USB_23_IRQ		75
#define RTL_USB_2_IRQ		74
#define RTL_PCIE1_IRQ		73
#define RTL_PCIE0_IRQ		72
#define RTL_USBHOSTP2_IRQ	40

struct rtl96xx_soc_info {
	unsigned char *name;
	unsigned int id;
	unsigned int family;
	unsigned int subtype;
	unsigned char *compatible;
	volatile void *sw_base;
	volatile void *icu_base;
	int cpu_port;
};

/* Function prototypes */

uint32_t pll_ocp_freq_mhz(void);

#endif /* _MACH_RTL960XC_H_ */
