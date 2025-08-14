// SPDX-License-Identifier: GPL-2.0-only

#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/delay.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/platform_device.h>
#include <asm/mach-rtl960xc/mach-rtl960xc.h>

extern struct rtl96xx_soc_info soc_info;

#define REG32(reg)    (*(volatile unsigned int   *)(reg))

#define RTK_USB_PHY_VER "v1.1 (2021.03.10)"
#define RTK_USB2_PHY_VER_REVC "v3.0 (2022.01.05)"
#define RTK_USB3_PHY_VER_REVC "v4.0 (2025.04.10)"

#define USE_XHCI_VER_3_10	1
#if defined(CONFIG_USB_XHCI_HCD)
static unsigned long BSP_XHCI_BASE	    = 0xB8040000;
static unsigned long BSP_USB3_EXT_REG	= 0xB8140000;
#if !defined(CONFIG_USB_9607C_EHCI2)
static unsigned long BSP_USB2_EXT_REG	= 0xB8140200;
#endif
#endif

#define BSP_XHCI_USB2_PHY_CTRL (BSP_XHCI_BASE + 0xC280)
#define BSP_USB3_MDIO          (BSP_USB3_EXT_REG + 0x0)
#define BSP_USB3_IPCFG         (BSP_USB3_EXT_REG + 0x8)
#if !defined(CONFIG_USB_9607C_EHCI2)
#define BSP_USB2_AUX_REG       (BSP_USB2_EXT_REG + 0xc)
#endif

#define BSP_USB_PHY_CTRL		0xB8000500UL
#define BSP_OHCI_BASE			0xB8020000UL
#define BSP_OHCI_SIZE           0x00000200UL
#define BSP_EHCI_BASE			0xB8021000UL
#define BSP_EHCI_SIZE           0x00000200UL
#define BSP_EHCI_UTMI_CTRL		(BSP_EHCI_BASE + 0xA4)
#define BSP_OHCI2_BASE			0xB8020200UL
#define BSP_EHCI2_BASE			0xB8021200UL
#define BSP_EHCI2_UTMI_CTRL		(BSP_EHCI2_BASE + 0xA4)

struct rtk_u2phy_param {
	u8 reg;
	u8 val;
};

struct rtk_u3phy_param {
	u8 addr;
	u16 val;
};

static const struct rtk_u2phy_param u2phy_data[] = {
	{0xF4, 0x9B},{0xE4, 0x6C},{0xE7, 0x61},{0xF4, 0xBB},{0xE0, 0x21},{0xE0, 0x25},
	{0xF4, 0x9B},{0xE0, 0xE3},{0xE0, 0x63},{0xE0, 0xE3},{0xE0, 0xE3},{0xE1, 0x30},
	{0xE2, 0x0B},{0xE3, 0x8D},{0xE4, 0xE7},{0xE5, 0x19},{0xE6, 0xC0},{0xE7, 0x91},
	{0xF0, 0xFC},{0xF1, 0x8C},{0xF2, 0x00},{0xF3, 0x31},{0xF4, 0x9B},{0xF5, 0x04},
	{0xF6, 0x00},{0xF7, 0x0A},{0xF4, 0xbb},{0xE0, 0x25},{0xE1, 0xcf},{0xE2, 0x00},
	{0xE3, 0x00},{0xE4, 0x00},{0xE5, 0x11},{0xE6, 0x06},{0xE7, 0x66},{0xF4, 0x9b},
};

static const struct rtk_u3phy_param u3phy_data[] = {
	{0x00, 0x400c},{0x01, 0xe058},{0x02, 0x6002},{0x03, 0x2771},{0x04, 0xf000},
	{0x05, 0x230f},{0x06, 0x001f},{0x07, 0x2e00},{0x08, 0x3191},{0x09, 0x121c},
	{0x0a, 0xa601},{0x0b, 0xa905},{0x0c, 0xc000},{0x0d, 0xef1c},{0x0e, 0x2010},
	{0x0f, 0xc020},{0x10, 0x000c},{0x11, 0x4c00},{0x12, 0xfc00},{0x13, 0x0c81},
	{0x14, 0xde01},{0x15, 0x0000},{0x16, 0x0000},{0x17, 0x0000},{0x18, 0x0000},
	{0x19, 0xe004},{0x1a, 0x1260},{0x1b, 0xff06},{0x1c, 0xcb00},{0x1d, 0xa03f},
	{0x1e, 0xc2e0},{0x1f, 0x9009},{0x20, 0xd4ff},{0x21, 0xbb99},{0x22, 0x0057},
	{0x23, 0xab65},{0x24, 0x0800},{0x25, 0x0000},{0x26, 0x040b},{0x27, 0x023f},
	{0x28, 0xf802},{0x29, 0x3080},{0x2a, 0x3082},{0x2b, 0x2078},{0x2c, 0x0000},
	{0x2d, 0x00ff},{0x2e, 0x0000},{0x2f, 0x0000},{0x30, 0x0000},
};

static const struct rtk_u2phy_param u2phy_data_revC[] = {
	{0xF4, 0x9B},{0xE0, 0x95},{0xE4, 0x6A},{0xF3, 0x31},{0xF4, 0xBB},{0xE0, 0x26},
	{0xF4, 0xDB},{0xE7, 0x33},{0xF4, 0xBB},{0xE0, 0x26},{0xE0, 0x22},{0xE0, 0x26},
	{0xF4, 0x9B},
};

static const struct rtk_u3phy_param u3phy_data_revC[] = {
	{0x01, 0xac89},{0x06, 0x0019},{0x07, 0x0600},{0x08, 0x3261},{0x09, 0x424c},
	{0x0a, 0xb210},{0x0b, 0xb90d},{0x0d, 0xe718},{0x0e, 0x1010},{0x0f, 0x8d50},
	{0x20, 0x7025},{0x21, 0x88a0},{0x23, 0x0b66},{0x27, 0x01e1},{0x29, 0xf0f2},
	{0x2b, 0x80a8},{0x09, 0x424c},{0x09, 0x404c},{0x09, 0x424c},
};

struct rtk_usb_phy {
	const char *name;
	const char *ver;
	void *priv;
	const void *param_data;
	u16 param_len;
	u8 port;
	u8 vport;

	void (*phy_init)(struct rtk_usb_phy *phy);
	int (*phy_read)(struct rtk_usb_phy *phy, u8 port, u8 reg, u16 *val);
	int (*phy_write)(struct rtk_usb_phy *phy, u8 port, u8 reg, u16 val);
	int (*phy_get_next)(struct rtk_usb_phy *phy, u8 port, int *reg);
};

int rtk_usb_phy_register(struct rtk_usb_phy *phy);

__weak struct proc_dir_entry *realtek_proc;

static void show_usage(void)
{

	printk("	w [reg] [value] : write [value] to [reg] of port1 (value should be 8 bits, reg should be in [E0-E7], [F0-F6])\n");
}

static ssize_t usb_proc_write(struct file * file, const char __user * userbuf, size_t count, loff_t * off)
{
	char buf[32];
	int len;
	u8 reg;
	u16 val;
	struct rtk_usb_phy *phy = pde_data(file_inode(file));

	len = min(sizeof(buf), count);
	if (copy_from_user(buf, userbuf, len))
		return -E2BIG;

	if (strncmp(buf, "help", 4) == 0) {
		show_usage();
	} else if (strncmp(buf, "w ", 2) == 0) {
		if(2==sscanf(buf, "w %hhx %hx", &reg, &val)) {
			//printk("EHCI: write %hhx to phy port %d, phy(%hhx)\n", val, PHY_PORT, reg);
			//ehci_phy_write(PHY_PORT, reg, val);
			if (phy->phy_write(phy, phy->port, reg, val))
				return -EINVAL;
		} else {
			goto ERROR_PARA;
		}
	} else if (strncmp(buf, "r ", 2) == 0) {
		if(1==sscanf(buf, "r %hhx", &reg)) {
			if (phy->phy_read(phy, phy->port, reg, &val))
				return -EINVAL;

			printk("%02x = %02x\n", reg, val);
		}
	} else {
		goto ERROR_PARA;
	}
	return count;
ERROR_PARA:
	printk("error parameter...\n");
	show_usage();
	return -EPERM;
}


#define ITEM_PER_LINE 4
static int rtk_usb_phy_show(struct seq_file *s, void *v)
{
	struct rtk_usb_phy *phy = s->private;
	int reg = -1, n = 0;
	u16 data;

	seq_printf(s, "RTK USB PHY version: %s\n\n", phy->ver);

	while (0==phy->phy_get_next(phy, phy->port, &reg)) {
		n++;
		if (0==phy->phy_read(phy, phy->port, reg, &data))
			seq_printf(s, "%02x: %4x   ", reg, data);
		else
			seq_printf(s, "%02x:  err   ", reg);

		if ((n%ITEM_PER_LINE)==0)
			seq_printf(s, "\n");
	}
	seq_printf(s, "\n");
	return 0;
}

static int rtk_usb_phy_open(struct inode *inode, struct file *file)
{
	return(single_open(file, rtk_usb_phy_show, pde_data(inode)));
}


static const struct proc_ops proc_ops_rtk_usb_phy = {
	.proc_open  	= rtk_usb_phy_open,
	.proc_write 	= usb_proc_write,
	.proc_read		= seq_read,
	.proc_lseek		= seq_lseek,
	.proc_release	= single_release,
};

int rtk_usb_phy_register(struct rtk_usb_phy *phy)
{

	struct proc_dir_entry *e;

	printk("register USB PHY: %s, RTK USB PHY version: %s\n", phy->name, phy->ver);

	if (phy->phy_init)
		phy->phy_init(phy);

	e = proc_create_data(phy->name, S_IRUGO | S_IWUSR, realtek_proc, &proc_ops_rtk_usb_phy, phy);
	if (!e) {
		printk("Failed to register USB_PHY %s\n", phy->name);
		return -EINVAL;
	}
	return 0;
}


#define BSP_USB3_PHY_CTRL		0xB8000508
#define USB3_CAP_MASK ((1<<27)|(1<<28))
struct phy_regs {
	u32 ehci_utmi_reg;
	u32 usb2_ext_reg;
};
#define PADDR(addr)  ((addr) & 0x1FFFFFFF)
static u64 bsp_usb_dmamask = 0xFFFFFFFFUL;
/* USB Host Controller */
#ifdef CONFIG_USB_OHCI_HCD
static struct resource bsp_usb_ohci_resource[] = {
	[0] = {
		.start = PADDR(BSP_OHCI_BASE),
		.end   = PADDR(BSP_OHCI_BASE) + BSP_OHCI_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = RTL_USB_2_IRQ,
		.end   = RTL_USB_2_IRQ,
		.flags = IORESOURCE_IRQ,
	}
};

static struct platform_device bsp_usb_ohci_device = {
	//.name          = "rtl86xx-ohci",
	.name          = "ohci-platform",
	.id            = 0,
	.num_resources = ARRAY_SIZE(bsp_usb_ohci_resource),
	.resource      = bsp_usb_ohci_resource,
	.dev           = {
		.dma_mask = &bsp_usb_dmamask,
		.coherent_dma_mask = 0xFFFFFFFFUL,
	}
};

static struct resource bsp_usb_ohci2_resource[] = {
	[0] = {
		.start = PADDR(BSP_OHCI2_BASE),
		.end   = PADDR(BSP_OHCI2_BASE) + BSP_OHCI_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = RTL_USBHOSTP2_IRQ,
		.end   = RTL_USBHOSTP2_IRQ,
		.flags = IORESOURCE_IRQ,
	}
};

static struct platform_device bsp_usb_ohci2_device = {
	//.name          = "rtl86xx-ohci",
	.name          = "ohci-platform",

	.id            = 1,
	.num_resources = ARRAY_SIZE(bsp_usb_ohci2_resource),
	.resource      = bsp_usb_ohci2_resource,
	.dev           = {
		.dma_mask = &bsp_usb_dmamask,
		.coherent_dma_mask = 0xFFFFFFFFUL,
	}
};
#endif
#ifdef CONFIG_USB_EHCI_HCD
static struct resource bsp_usb_ehci_resource[] = {
	[0] = {
		.start = PADDR(BSP_EHCI_BASE),
		.end   = PADDR(BSP_EHCI_BASE) + BSP_EHCI_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = RTL_USB_2_IRQ,
		.end   = RTL_USB_2_IRQ,
		.flags = IORESOURCE_IRQ,
	}
};



static struct platform_device bsp_usb_ehci_device = {
	//.name          = "rtl86xx-ehci",
	.name          = "ehci-platform",
	.id            = 0,
	.num_resources = ARRAY_SIZE(bsp_usb_ehci_resource),
	.resource      = bsp_usb_ehci_resource,
	.dev           = {
		.dma_mask = &bsp_usb_dmamask,
		.coherent_dma_mask = 0xFFFFFFFFUL,
	}
};


static struct resource bsp_usb_ehci2_resource[] = {
	[0] = {
		.start = PADDR(BSP_EHCI2_BASE),
		.end   = PADDR(BSP_EHCI2_BASE) + BSP_EHCI_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = RTL_USBHOSTP2_IRQ,
		.end   = RTL_USBHOSTP2_IRQ,
		.flags = IORESOURCE_IRQ,
	}
};

static struct platform_device bsp_usb_ehci2_device = {
	//.name          = "rtl86xx-ehci",
	.name          = "ehci-platform",
	.id            = 1,
	.num_resources = ARRAY_SIZE(bsp_usb_ehci2_resource),
	.resource      = bsp_usb_ehci2_resource,
	.dev           = {
		.dma_mask = &bsp_usb_dmamask,
		.coherent_dma_mask = 0xFFFFFFFFUL,
	}
};
#endif
#ifdef CONFIG_USB_XHCI_HCD
static struct resource bsp_xhci_resource[] = {
	{
		//.start = PADDR(BSP_XHCI_BASE),
		//.end = PADDR(BSP_XHCI_BASE) +0x0000EFFF,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = RTL_USB_23_IRQ,
		.end = RTL_USB_23_IRQ,
		.flags = IORESOURCE_IRQ,
	}
};

struct platform_device bsp_xhci_device = {
	.name = "xhci-hcd",
	//.name = "rtl86xx-xhci",
	.id = -1,
	.num_resources = ARRAY_SIZE(bsp_xhci_resource),
	.resource = bsp_xhci_resource,
	.dev = {
		.dma_mask = &bsp_usb_dmamask,
		.coherent_dma_mask = 0xffffffffUL
	}
};
#endif

static struct platform_device *bsp_usb_devs0[] __initdata = {
#ifdef CONFIG_USB_OHCI_HCD
	&bsp_usb_ohci_device,
#endif
#ifdef CONFIG_USB_EHCI_HCD
	&bsp_usb_ehci_device,
#endif
};

static struct platform_device *bsp_usb_devs2[] __maybe_unused __initdata = {
#ifdef CONFIG_USB_OHCI_HCD
	&bsp_usb_ohci2_device,
#endif
#ifdef CONFIG_USB_EHCI_HCD
	&bsp_usb_ehci2_device,
#endif
};
#if !defined(CONFIG_USB_9607C_EHCI2)
#ifdef CONFIG_USB_XHCI_HCD
static struct platform_device *bsp_usb_devs1[] __initdata = {
	&bsp_xhci_device,
};
#endif
#endif

/* ============== EHCI ============ */
#define LOOP_LIMIT 100000
#define UTMI_NOP   (1<<12)
#define UTMI_LOAD  (0<<12)
#define BUILD_CMD(vp,vctrl) (((vp)&0xf)<<13)|(((vctrl)&0xf)<<8)
#define UTMI_BUSY(x) (x&(1<<17))
static u32 utmi_wait(struct rtk_usb_phy *phy)
{
	u32 __reg, __c = 0;
	struct phy_regs *p = phy->priv;
	u32 utmi_reg = p->ehci_utmi_reg;
	mdelay(1);
	do {
		__c++;
		__reg = le32_to_cpu(REG32(utmi_reg));
		if (unlikely(__c>LOOP_LIMIT)) {
			printk("utmi_wait timeout\n");
			return 0;
		}
	} while (UTMI_BUSY(__reg));
	return __reg;
}

static void utmi_set(struct rtk_usb_phy *phy, u32 v)
{
	struct phy_regs *p = phy->priv;
	u32 utmi_reg = p->ehci_utmi_reg;
	utmi_wait(phy);
	REG32(utmi_reg) = cpu_to_le32(v);
}

static u32 utmi_get(struct rtk_usb_phy *phy)
{
	u32 reg;
	reg = utmi_wait(phy);
	return reg;
}

static int ehci_phy_get(struct rtk_usb_phy *phy, u8 port, u8 reg, u8 *data)
{

	// send [3:0]
	utmi_set(phy, BUILD_CMD(phy->vport,reg&0xf)|UTMI_NOP);
	utmi_set(phy, BUILD_CMD(phy->vport,reg&0xf)|UTMI_LOAD);
	utmi_set(phy, BUILD_CMD(phy->vport,reg&0xf)|UTMI_NOP);

	// send [7:4]
	utmi_set(phy, BUILD_CMD(phy->vport,reg>>4)|UTMI_NOP);
	utmi_set(phy, BUILD_CMD(phy->vport,reg>>4)|UTMI_LOAD);
	utmi_set(phy, BUILD_CMD(phy->vport,reg>>4)|UTMI_NOP);

	*data = utmi_get(phy) & 0xff;

	return 0;
}


static int ehci_is_valid_param(u8 port, u8 reg)
{
	//int i;

	if (port > 2)
		return 0;

	if (((reg >= 0xe0) && (reg <= 0xe7)) ||
	        ((reg >= 0xf0) && (reg <= 0xf7)) ||
	        ((reg >= 0xb0) && (reg <= 0xb7)))
		return 1;

	return 0;
}

static int __ehci_phy_read(struct rtk_usb_phy *phy, u8 port, u8 reg, u16 *val)
{
	int ret = 0;
	u8 data;

	//printk("%s(%d): %02x %02x %d\n",__func__,__LINE__,port,reg,ehci_is_valid_param(port, reg));
	if (!ehci_is_valid_param(port, reg))
		return -EINVAL;

	if (!((reg >= 0xb0) && (reg <= 0xb7)))
		reg -= 0x20;

	ehci_phy_get(phy, port, reg, &data);

	*val = data;

	return ret;
}

static int __ehci_phy_write(struct rtk_usb_phy *phy, u8 port, u8 reg, u16 val)
{
	struct phy_regs *p = phy->priv;
	u32 usb2_aux_reg = p->usb2_ext_reg+0xc;

	if (!ehci_is_valid_param(port, reg))
		return -EINVAL;

	//if (port)
	//	REG32(BSP_USB_PHY_CTRL) = (REG32(BSP_USB_PHY_CTRL) & ~0xff0000) | ((val & 0xff) << 16);
	//else
	//	REG32(BSP_USB_PHY_CTRL) = (REG32(BSP_USB_PHY_CTRL) & ~0xff) | (val & 0xff);
	REG32(usb2_aux_reg) = (val)&0xff;

	// send [3:0]
	utmi_set(phy, BUILD_CMD(phy->vport,reg&0xf)|UTMI_NOP);
	utmi_set(phy, BUILD_CMD(phy->vport,reg&0xf)|UTMI_LOAD);
	utmi_set(phy, BUILD_CMD(phy->vport,reg&0xf)|UTMI_NOP);

	// send [7:4]
	utmi_set(phy, BUILD_CMD(phy->vport,reg>>4)|UTMI_NOP);
	utmi_set(phy, BUILD_CMD(phy->vport,reg>>4)|UTMI_LOAD);
	utmi_set(phy, BUILD_CMD(phy->vport,reg>>4)|UTMI_NOP);

	return 0;

}

static int __ehci_phy_get_next(struct rtk_usb_phy *phy, u8 port, int *reg)
{
	if (port > 2)
		return -EINVAL;

	if (*reg < 0)
		*reg = 0xe0;
	else if ((*reg >= 0xe0) && (*reg <= 0xe6))
		*reg += 1;
	else if (*reg == 0xe7)
		*reg = 0xf0;
	else if ((*reg >= 0xf0) && (*reg <= 0xf6))
		*reg += 1;
	else
		return -EINVAL;

	return 0;
}

static void __ehci_phy_init(struct rtk_usb_phy *phy)
{

	struct phy_regs *p = phy->priv;
	const struct rtk_u2phy_param *u2phy = (struct rtk_u2phy_param *)phy->param_data;
	int i;

	REG32(p->usb2_ext_reg + 0x10) &= ~(1<<5); /* disable force-host-disconnect */

	mdelay(10); /* allow IP to startup */

	for (i=0; i<phy->param_len;i++) {
		__ehci_phy_write(phy, phy->port,u2phy[i].reg,u2phy[i].val);
	}
}

static struct phy_regs ehci_regs = {
	.ehci_utmi_reg = BSP_EHCI_UTMI_CTRL,
	.usb2_ext_reg = 0xB8140200,
};
static struct rtk_usb_phy ehci_phy = {
	.name = "ehci-phy",
	.ver = RTK_USB_PHY_VER,
	.port = 0,
	.vport = 1,
	.param_data = (void *)u2phy_data,
	.param_len = ARRAY_SIZE(u2phy_data),
	.priv = &ehci_regs,
	.phy_init = __ehci_phy_init,
	.phy_read = __ehci_phy_read,
	.phy_write = __ehci_phy_write,
	.phy_get_next = __ehci_phy_get_next,
};


static struct phy_regs ehci2_regs = {
	.ehci_utmi_reg = BSP_EHCI2_UTMI_CTRL,
	.usb2_ext_reg = 0xB8140300,
};

static struct rtk_usb_phy ehci2_phy = {
	.name = "ehci2-phy",
	.ver = RTK_USB_PHY_VER,
	.port = 2,
	.vport = 1,
	.param_data = (void *)u2phy_data,
	.param_len = ARRAY_SIZE(u2phy_data),
	.priv = &ehci2_regs,
	.phy_init = __ehci_phy_init,
	.phy_read = __ehci_phy_read,
	.phy_write = __ehci_phy_write,
	.phy_get_next = __ehci_phy_get_next,
};

/* ============== XHCI U2 ============ */
#define U3UTMI_READY_MASK ((1<<24)|(1<<23))
#define U3UTMI_READY(x) ((x&U3UTMI_READY_MASK)==(1<<24))
#define U3UTMI_NEWREQ (1<<25)
// add another wrapper, since HOST can be in big-endian mode (BSP_USB3_IPCFG)
// Use le32_ func if IPCFG is little-endian mode.

#define U3_FROM_HOST(x) (x)
#define U3_TO_HOST(x) (x)

//#define U3_FROM_HOST(x) le32_to_cpu(x)
//#define U3_TO_HOST(x) cpu_to_le32(x)

#if defined(CONFIG_USB_XHCI_HCD)
static void u3_utmi_wait(void)
{
	u32 __reg, __c = 0;
	mdelay(1);
	do {
		__c++;
		__reg = U3_FROM_HOST(REG32(BSP_XHCI_USB2_PHY_CTRL));
		if (unlikely(__c>LOOP_LIMIT)) {
			printk("u3_utmi_wait timeout\n");
			return;
		}
	} while (!U3UTMI_READY(__reg));
}

static void u3_utmi_set(u32 v)
{
	u3_utmi_wait();
	REG32(BSP_XHCI_USB2_PHY_CTRL) = U3_TO_HOST(v);
}

static u32 u3_utmi_get(void)
{
	u32 reg;
	u3_utmi_wait();
	reg = U3_FROM_HOST(REG32(BSP_XHCI_USB2_PHY_CTRL));
	return reg;
}
static int __xhci_u2phy_read(struct rtk_usb_phy *phy, u8 port, u8 reg, u16 *val)
{
	if (!ehci_is_valid_param(port, reg))
		return -EINVAL;

	reg = reg - 0x20;

	u3_utmi_set(((reg&0x0f) << 8) | U3UTMI_NEWREQ);
	u3_utmi_wait();
	u3_utmi_set(((reg&0xf0) << 4) | U3UTMI_NEWREQ);
	u3_utmi_wait();
	u3_utmi_set(                    U3UTMI_NEWREQ);
	u3_utmi_wait();
	*val = (u3_utmi_get() & 0xff);
	return 0;
}

static int __xhci_u2phy_write(struct rtk_usb_phy *phy, u8 port, u8 reg, u16 val)
{
	if (!ehci_is_valid_param(port, reg))
		return -EINVAL;

	if (port)
		REG32(BSP_USB_PHY_CTRL) = (REG32(BSP_USB_PHY_CTRL) & ~0xff0000) | ((val & 0xff) << 16);
	else
		REG32(BSP_USB_PHY_CTRL) = (REG32(BSP_USB_PHY_CTRL) & ~0xff) | (val & 0xff);

	u3_utmi_set(((reg&0x0f) << 8) | U3UTMI_NEWREQ);
	u3_utmi_wait();
	u3_utmi_set(((reg&0xf0) << 4) | U3UTMI_NEWREQ);
	u3_utmi_wait();

	return 0;
}

static void __xhci_u2phy_init(struct rtk_usb_phy *phy)
{
	const struct rtk_u2phy_param *u2phy = (struct rtk_u2phy_param *)phy->param_data;
	int i;

	printk("XHCI U2 Phy Init\n");
	for (i=0; i<phy->param_len;i++) {
		__xhci_u2phy_write(phy, phy->port,u2phy[i].reg,u2phy[i].val);
	}
}

static struct rtk_usb_phy xhci_u2phy = {
	.name = "xhci-u2",
	.ver = RTK_USB_PHY_VER,
	.port = 0,
	.vport = 1,
	.param_data = (void *)u2phy_data,
	.param_len = ARRAY_SIZE(u2phy_data),
	.phy_init = __xhci_u2phy_init,
	.phy_read = __xhci_u2phy_read,
	.phy_write = __xhci_u2phy_write,
	.phy_get_next = __ehci_phy_get_next,
};

/* ============== XHCI U3 ============ */
#define MDIO_READY_MASK ((3<<5)|(1<<4)|(1<<1))
#define MDIO_READY(x) ((x&MDIO_READY_MASK)==(1<<4))
static u32 mdio_wait(void)
{
	u32 __reg, __c = 0;
	mdelay(1);
	do {
		__c++;
		__reg = REG32(BSP_USB3_MDIO);
		if (unlikely(__c>LOOP_LIMIT)) {
			printk("mdio_wait timeout\n");
			return 0;
		}
	} while (!MDIO_READY(__reg));

	return __reg;
}



static int __xhci_u3phy_read(struct rtk_usb_phy *phy, u8 port, u8 reg, u16 *val)
{
	u32 v;
	mdio_wait();
	REG32(BSP_USB3_MDIO) = (reg << 8);
	v = mdio_wait();
	*val = (v>>16);
	return 0;
}

static int __xhci_u3phy_write(struct rtk_usb_phy *phy, u8 port, u8 reg, u16 val)
{
	mdio_wait();
	REG32(BSP_USB3_MDIO) = (val << 16) | (reg << 8) | 1;
	return 0;
}

static int __xhci_u3_get_next(struct rtk_usb_phy *phy, u8 port, int *reg)
{
	if (*reg < 0) {
		*reg = 0;
		return 0;
	} else 	if ((*reg >= 0) && (*reg < 0x30)) {
		*reg = *reg + 1;
		return 0;
	}
	return -EINVAL;
}

static void __xhci_u3phy_init(struct rtk_usb_phy *phy)
{
	const struct rtk_u3phy_param *u3phy = (struct rtk_u3phy_param *)phy->param_data;
	int i;

	printk("XHCI U3 Phy Init\n");

	for (i=0; i<phy->param_len;i++) {
		__xhci_u3phy_write(phy, 0,u3phy[i].addr,u3phy[i].val);
	}
}

static struct rtk_usb_phy xhci_u3phy = {
	.name = "xhci-u3",
	.ver = RTK_USB_PHY_VER,
	.port = 0,
	.vport = 1,
	.param_data = (void *)u3phy_data,
	.param_len = ARRAY_SIZE(u3phy_data),
	.phy_init = __xhci_u3phy_init,
	.phy_read = __xhci_u3phy_read,
	.phy_write = __xhci_u3phy_write,
	.phy_get_next = __xhci_u3_get_next,
};
#endif // OCNFIG_USB_XHCI_HCD
extern int rtk_usb_phy_register(struct rtk_usb_phy *phy);


#if !defined(CONFIG_USB_9607C_EHCI2)
static int __init_plat_xhci(void)
{
#if defined(CONFIG_USB_XHCI_HCD)
	u32 reg;
#ifdef USE_XHCI_VER_3_10
	reg = REG32(NEW_BSP_IP_SEL) & ~(7<<29);
	REG32(NEW_BSP_IP_SEL) = reg | (2<<29) | (1<<31) | (1<<26);
	if (REG32(NEW_BSP_IP_SEL) & (1<<31)) {
		printk("XHCI v3.10\n");
		BSP_XHCI_BASE	 = 0xB8060000;
		BSP_USB3_EXT_REG = 0xB8140100;
		BSP_USB2_EXT_REG = 0xB8140300;
	}
#endif
	REG32(BSP_IP_SEL) |= (3<<4) | (1<<29) | (1<<30);
	mdelay(10);

	bsp_xhci_resource[0].start = PADDR(BSP_XHCI_BASE);
	bsp_xhci_resource[0].end   = PADDR(BSP_XHCI_BASE) +0x0000EFFF;

	//printk("XHCI reset...\n");

	REG32(BSP_USB3_IPCFG) &= ~(1<<6); /* host-order */
	REG32(BSP_USB3_IPCFG) |= (0x2); /* wake u2 */
	REG32(BSP_USB3_IPCFG) |= (1<<27); /* disable RX50T */
	mdelay(10);
	REG32(BSP_USB3_IPCFG) &= ~(1<<27); /* put it back RX50T */

#ifndef USE_XHCI_VER_3_10
	// fix default MAC value
	REG32(BSP_XHCI_BASE+0xc108)=U3_TO_HOST(0x21010000);
	REG32(BSP_XHCI_BASE+0xc10c)=U3_TO_HOST(0x21080000);
	REG32(BSP_XHCI_BASE+0xc2c0)=U3_TO_HOST(0x00041202);
#endif

	REG32(BSP_XHCI_BASE+0x430) |= (0x80); /* PORT reset */ //REG32(0xB8040430) |= 1<<31;
	rtk_usb_phy_register(&xhci_u2phy);
	rtk_usb_phy_register(&xhci_u3phy);

	return platform_add_devices(bsp_usb_devs1, ARRAY_SIZE(bsp_usb_devs1));

#else
	return 0;
#endif

}
#endif

static int __init_plat_ehci2(void)
{
#if defined(CONFIG_USB_EHCI_HCD) || defined(CONFIG_USB_OHCI_HCD)
	u32 reg;
	printk("EHCI2 initialization2\n");
	reg = REG32(NEW_BSP_IP_SEL) & ~(7<<29);
	REG32(NEW_BSP_IP_SEL) = reg | (1<<29);
	if ((REG32(NEW_BSP_IP_SEL) >> 29)!=0x1) {
		return -EINVAL;
	}

	REG32(BSP_IP_SEL) |= (1<<30) | (1<<13);
	mdelay(5);

	rtk_usb_phy_register(&ehci2_phy);

	return platform_add_devices(bsp_usb_devs2, ARRAY_SIZE(bsp_usb_devs2));
#else
	return 0;
#endif
}

static int __init bsp_usb_init_9607c(void)
{
	int ret = 0;
#if defined(CONFIG_USB_EHCI_HCD) || defined(CONFIG_USB_OHCI_HCD)
	REG32(BSP_IP_SEL) |= (1<<3) | (1<<31);
	mdelay(5);
	ehci_phy.port = 1;
	rtk_usb_phy_register(&ehci_phy);
#endif

	ret = platform_add_devices(bsp_usb_devs0, ARRAY_SIZE(bsp_usb_devs0));
	if (ret < 0) {
		printk("ERROR: unable to add EHCI\n");
		return ret;
	}

#if defined(CONFIG_USB_9607C_EHCI2)
#ifndef CONFIG_EXTERNAL_PHY_POLLING
	REG32(BSP_USB3_PHY_CTRL) &= ~(USB3_CAP_MASK);
#endif
	ret = __init_plat_ehci2();
	if (ret < 0) {
		printk("ERROR: unable to add EHCI2\n");
		return ret;
	}
#else // EHCI2
	ret = __init_plat_xhci();
	if (ret < 0) {
		printk("ERROR: unable to add XHCI\n");
		return ret;
	}
#endif // EHCI2

	return 0;
}

static int __init bsp_usb_init_9603c(void)
{
	int ret = 0;
	REG32(BSP_USB3_PHY_CTRL) &= ~(USB3_CAP_MASK);
	ret = __init_plat_ehci2();

	if (ret < 0) {
		printk("ERROR: unable to add EHCI\n");
		return ret;
	}

	return 0;
}

// __weak int bsp_usb_init_9603cvd(void)
// {
// 	printk("%s(%d): \n",__func__,__LINE__);
// 	return -ENODEV;
// }

static int __init bsp_usb_init(void)
{
	if (soc_info.family == RTL9607_FAMILY_ID) {
		printk("USB: Using revC phy\n");
		#if defined(CONFIG_USB_XHCI_HCD)
		xhci_u3phy.ver = RTK_USB3_PHY_VER_REVC;
		xhci_u3phy.param_data = u3phy_data_revC;
		xhci_u3phy.param_len = ARRAY_SIZE(u3phy_data_revC);
		xhci_u2phy.ver = RTK_USB2_PHY_VER_REVC;
		xhci_u2phy.param_data = u2phy_data_revC;
		xhci_u2phy.param_len = ARRAY_SIZE(u2phy_data_revC);
		#endif
		ehci_phy.ver = RTK_USB2_PHY_VER_REVC;
		ehci_phy.param_data = u2phy_data_revC;
		ehci_phy.param_len = ARRAY_SIZE(u2phy_data_revC);
		ehci2_phy.ver = RTK_USB2_PHY_VER_REVC;
		ehci2_phy.param_data = u2phy_data_revC;
		ehci2_phy.param_len = ARRAY_SIZE(u2phy_data_revC);

		switch(soc_info.subtype) {
		case RTL9607C_CHIP_SUB_TYPE_RTL9603CT:
		case RTL9607C_CHIP_SUB_TYPE_RTL9603C_VA4:
		case RTL9607C_CHIP_SUB_TYPE_RTL9603C_VA5:
		case RTL9607C_CHIP_SUB_TYPE_RTL9603C_VA6:
			return bsp_usb_init_9603c();
		default:
			/*
			 * RTL9607C_CHIP_SUB_TYPE_RTL9607CP
			 * RTL9607C_CHIP_SUB_TYPE_RTL9607EP
			 * RTL9607C_CHIP_SUB_TYPE_RTL9607C_VA5
			 * RTL9607C_CHIP_SUB_TYPE_RTL9607C_VA6
			 * RTL9607C_CHIP_SUB_TYPE_RTL9607C_VA7
			 * RTL9607C_CHIP_SUB_TYPE_RTL9607E_VA5
			 * RTL9607C_CHIP_SUB_TYPE_RTL9603CP
			 * RTL9607C_CHIP_SUB_TYPE_RTL8198D_VE5
			 * RTL9607C_CHIP_SUB_TYPE_RTL8198D_VE6
			 * RTL9607C_CHIP_SUB_TYPE_RTL8198D_VE7
			 * And any other RTL9607C sub type
			 */
			return bsp_usb_init_9607c();
		}
	}

	pr_info("USB not initialized, %x\n",soc_info.id);
	return -ENXIO;
}

module_init(bsp_usb_init);

MODULE_LICENSE("GPL");
