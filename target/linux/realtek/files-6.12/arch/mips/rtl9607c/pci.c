// SPDX-License-Identifier: GPL-2.0-only

#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/pci.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/mach-rtl960xc/mach-rtl960xc.h>

extern struct rtl96xx_soc_info soc_info;

#define RTK_PCIE_PHY_VER      "v1.3 (2021.06.09)"
#define RTK_PCIE_PHY_REVC_VER "v007 (2025.03.25)"

#define PADDR(addr)  ((addr) & 0x1FFFFFFF)

#define BSP_PCI_MISC 0xb8000504
#define BSP_PCIE1_D_IO      0xB8C00000
#define BSP_PCIE0_D_IO      0xB8E00000
#define BSP_PCIE1_D_MEM     0xB9000000
#define BSP_PCIE0_D_MEM     0xBA000000
#define BSP_PCIE1_D_CFG0    0xB8B10000
#define BSP_PCIE0_D_CFG0    0xB8B30000
#define BSP_PCIE1_H_CFG     0xB8B00000
#define BSP_PCIE1_H_EXT     0xB8B01000
#define BSP_PCIE0_H_CFG     0xB8B20000
#define BSP_PCIE0_H_EXT     0xB8B21000

#define BSP_ENABLE_PCIE0 (1<<7)
#define BSP_ENABLE_PCIE1 (1<<6)

/* to be removed */
#define CONFIG_GENERIC_RTL86XX_PCIE_SLOT0 1
#define CONFIG_GENERIC_RTL86XX_PCIE_SLOT1 1

#ifndef REG32
#define REG32(reg)	(*(volatile unsigned int   *)(reg))
#endif
#ifndef REG16
#define REG16(reg)	(*(volatile unsigned short *)(reg))
#endif
#ifndef REG08
#define REG08(reg)	(*(volatile unsigned char  *)(reg))
#endif

struct pcie_para {
	u8 reg;
	u16 value;
};

void rtk_pcie_host_shutdown(void);

#define MAX_NUM_DEV  4
#define FL_MDIO_RESET	(1<<0)
#define FL_SET_BAR		(1<<1)
struct rtk_pci_controller {
	void __iomem *devcfg_base;
	void __iomem *hostcfg_base;
	void __iomem *hostext_base;
	spinlock_t lock;
	struct pci_controller controller;
	const char *ver;
	const struct pcie_para *phy_param;
	void (*rst_fn)(struct rtk_pci_controller *, int);
	unsigned long rst_data;
	struct gpio_desc *gpiod;
	u8 irq;
	u8 flags;
	u8 port;
	u8 is_linked;
	u8 bus_addr;
};

static inline struct rtk_pci_controller * pci_bus_to_rtk_controller(struct pci_bus *bus)
{
	struct pci_controller *hose;

	hose = (struct pci_controller *) bus->sysdata;
	return container_of(hose, struct rtk_pci_controller, controller);
}

static void rst_by_gpio(struct rtk_pci_controller *ctrl, int action)
{

	switch(action) {
	case 0:
		gpiod_set_raw_value(ctrl->gpiod, 0);
		break;
	case 1:
		gpiod_set_raw_value(ctrl->gpiod, 1);
		break;
	}
}

static int rtk_pcie_read(struct pci_bus *bus, unsigned int devfn, int where, int size, unsigned int *val)
{
	struct rtk_pci_controller *ctrl = pci_bus_to_rtk_controller(bus);
	unsigned long flags;
	void __iomem *base;
	u32 data;

	if (ctrl->bus_addr==0xff)
		ctrl->bus_addr = bus->number;

	//printk("pcix-r: %p,%d.%d.%x %x,%x+%4x\n",bus,bus->number,bus->primary,devfn, size, base, where);

	if (bus->number!=ctrl->bus_addr)
		return PCIBIOS_DEVICE_NOT_FOUND;

	//if (PCI_FUNC(devfn))
	//	return PCIBIOS_DEVICE_NOT_FOUND;

	switch (PCI_SLOT(devfn)) {
	case 0:
		base = ctrl->hostcfg_base;
		break;
	case 1:
		base = ctrl->devcfg_base;
		break;
	default:
		return PCIBIOS_DEVICE_NOT_FOUND;
	}

	spin_lock_irqsave(&ctrl->lock, flags);
	REG32(ctrl->hostext_base+0xC) = PCI_FUNC(devfn);
	switch (size) {
	case 1:
		data = REG08(base + where);
		break;
	case 2:
		data = REG16(base + where);
		break;
	case 4:
		data = REG32(base + where);
		break;
	default:
		spin_unlock_irqrestore(&ctrl->lock, flags);
		return PCIBIOS_BAD_REGISTER_NUMBER;
	}
	spin_unlock_irqrestore(&ctrl->lock, flags);

	//printk("pcix-r: %p,%d.%d.%x %x,%x+%4x->%x\n",bus,bus->number,bus->primary,devfn, size, base, where, data);

	*val = data;

	return PCIBIOS_SUCCESSFUL;
}

static int rtk_pcie_write(struct pci_bus *bus, unsigned int devfn, int where, int size, u32 val)
{
	struct rtk_pci_controller *ctrl = pci_bus_to_rtk_controller(bus);
	unsigned long flags;
	void __iomem *base;

	if (ctrl->bus_addr==0xff)
		ctrl->bus_addr = bus->number;

	if (bus->number!=ctrl->bus_addr)
		return PCIBIOS_DEVICE_NOT_FOUND;

	//if (PCI_FUNC(devfn))
	//	return PCIBIOS_DEVICE_NOT_FOUND;

	switch (PCI_SLOT(devfn)) {
	case 0:
		base = ctrl->hostcfg_base;
		break;
	case 1:
		base = ctrl->devcfg_base;
		break;
	default:
		return PCIBIOS_DEVICE_NOT_FOUND;
	}

	//printk("pcix-w: %p,%d.%d.%x %x,%x+%4x<-%x\n",bus,bus->number,bus->primary,devfn, size, base, where, val);
	spin_lock_irqsave(&ctrl->lock, flags);
	REG32(ctrl->hostext_base+0xC) = PCI_FUNC(devfn);
	switch (size) {
	case 1:
		REG08(base + where) = val;
		break;
	case 2:
		REG16(base + where) = val;
		break;
	case 4:
		REG32(base + where) = val;
		break;
	default:
		spin_unlock_irqrestore(&ctrl->lock, flags);
		return PCIBIOS_BAD_REGISTER_NUMBER;
	}
	spin_unlock_irqrestore(&ctrl->lock, flags);
	return PCIBIOS_SUCCESSFUL;
}

static struct pci_ops pcie_ops = {
	.read = rtk_pcie_read,
	.write = rtk_pcie_write
};

#ifdef CONFIG_GENERIC_RTL86XX_PCIE_SLOT0
static struct resource pcie0_io_resource = {
	.name   = "PCIE0 IO",
	.flags  = IORESOURCE_IO,
	.start  = PADDR(BSP_PCIE0_D_IO),
	.end    = PADDR(BSP_PCIE0_D_IO + 0xFFFF)
};

static struct resource pcie0_mem_resource = {
	.name   = "PCIE0 MEM",
	.flags  = IORESOURCE_MEM,
	.start  = PADDR(BSP_PCIE0_D_MEM),
	.end    = PADDR(BSP_PCIE0_D_MEM + 0xFFFFFF)
};

static struct rtk_pci_controller pci0_controller = {
	.devcfg_base	= (void __iomem *)BSP_PCIE0_D_CFG0,
	.hostcfg_base	= (void __iomem *)BSP_PCIE0_H_CFG,
	.hostext_base	= (void __iomem *)BSP_PCIE0_H_EXT,
	.irq			= RTL_PCIE0_IRQ,
	.controller = {
		.pci_ops        = &pcie_ops,
		.mem_resource   = &pcie0_mem_resource,
		.io_resource    = &pcie0_io_resource,
	},
	.ver 			= RTK_PCIE_PHY_VER,
	.rst_fn 		= rst_by_gpio,
	.port			= 0,
	.flags			= FL_MDIO_RESET,
	.bus_addr		= 0xff,
};
#endif

#ifdef CONFIG_GENERIC_RTL86XX_PCIE_SLOT1
static struct resource pcie1_io_resource = {
	.name   = "PCIE1 IO",
	.flags  = IORESOURCE_IO,
	.start  = PADDR(BSP_PCIE1_D_IO),
	.end    = PADDR(BSP_PCIE1_D_IO + 0xFFFF)
};

static struct resource pcie1_mem_resource = {
	.name   = "PCIE1 MEM",
	.flags  = IORESOURCE_MEM,
	.start  = PADDR(BSP_PCIE1_D_MEM),
	.end    = PADDR(BSP_PCIE1_D_MEM + 0xFFFFFF)
};

static struct rtk_pci_controller pci1_controller = {
	.devcfg_base	= (void __iomem *)BSP_PCIE1_D_CFG0,
	.hostcfg_base	= (void __iomem *)BSP_PCIE1_H_CFG,
	.hostext_base	= (void __iomem *)BSP_PCIE1_H_EXT,
	.irq			= RTL_PCIE1_IRQ,
	.controller = {
		.pci_ops        = &pcie_ops,
		.mem_resource   = &pcie1_mem_resource,
		.io_resource    = &pcie1_io_resource,
	},
	.ver 			= RTK_PCIE_PHY_VER,
	.rst_fn 		= rst_by_gpio,
	.port			= 1,
	.flags			= FL_MDIO_RESET,
	.bus_addr		= 0xff,
};
#endif

#define RETRY_COUNT 3
static int rtk_pcie_reset(struct rtk_pci_controller *p)
{
	u32 tmp;
	int bits = (p->port) ? (1<<21) : (1<<24|1<<30);
	const struct pcie_para *phy;
	int retry = 0;

RETRY:
	// 0. Assert PCIE Device Reset
	p->rst_fn(p, 0);

	// 1. PCIE phy mdio reset
	REG32(BSP_PCI_MISC) = REG32(BSP_PCI_MISC) & (p->port ? ~(1<<14) : ~(1<<15));
	if (p->flags & FL_MDIO_RESET) {
		REG32(BSP_PCI_MISC) &= ~bits;
		mb();
		REG32(BSP_PCI_MISC) |= bits;
	}

	// 2. PCIE MAC reset
	tmp = (p->port) ? BSP_ENABLE_PCIE1 : BSP_ENABLE_PCIE0;
	REG32(BSP_IP_SEL) &= ~tmp;
	mb();
	REG32(BSP_IP_SEL) |= tmp;
	mdelay(10);

	if (p->flags & FL_MDIO_RESET) {
		REG32(BSP_PCI_MISC) |= bits;
	}

	REG32(p->hostext_base+0x8) = 0x1; //bit7 PHY reset=0   bit0 Enable LTSSM=1
	mb();
	REG32(p->hostext_base+0x8) = 0x81;   //bit7 PHY reset=1   bit0 Enable LTSSM=1
	mdelay(50);


	for (phy = p->phy_param; phy->reg != 0xff; phy++) {
		void __iomem *mdiobase = p->hostext_base+0x0;
		REG32(mdiobase) = 	((phy->value & 0xffff) << 16) |
		                    ((phy->reg & 0xff) << 8) |	1;
		mdelay(1);
	}

	mdelay(20); //TPERST#-CLK min 100us

	// PCIE Device Reset
	p->rst_fn(p, 1);

	// wait for LinkUP
	tmp = 10;
	while(--tmp) {
		if((REG32(p->hostcfg_base + 0x0728)&0x1f)==0x11)
			break;

		mdelay(10);
	}

	if (tmp == 0) {
		printk("Warning!! Port %d PCIE Link Failed, State=0x%x retry=%d\n", p->port, REG32(p->hostcfg_base + 0x0728), retry);

		if (retry < RETRY_COUNT) {
			retry ++;
			goto RETRY;
		}

		return 0;
	}

	mdelay(100); // allow time for CR

	// 8. Set BAR
	if (p->flags & FL_SET_BAR) {
		REG32(p->devcfg_base + 0x10) = p->port ? 0x18e00001 : 0x18c00001;
		REG32(p->devcfg_base + 0x18) = p->port ? 0x1a000004 : 0x19000004;
		REG32(p->devcfg_base + 0x04) = 0x00180007;
	}

	REG32(p->hostcfg_base+ 0x04) = 0x00100007;

	// Enable PCIE host
	REG32(p->hostcfg_base + 0x04) = 0x00100007;
	REG08(p->hostcfg_base + 0x78) = (REG08(p->hostcfg_base + 0x78) & (~0xE0)) | 0;  // Set MAX_PAYLOAD_SIZE to 128B

	REG32(p->hostcfg_base + 0x80C) |= (1<<17);
	do {
		u16 status;
		const char *str;
		mdelay(1);
		status = REG16(p->hostcfg_base + 0x82);

		switch (status & 0xf) {
		case 1:
			str = "2.5GHz";
			break;
		case 2:
			str = "5.0Ghz";
			break;
		default:
			str = "Unknown";
			break;
		}
		printk("Port%d Link@%s\n", p->port, str);
	} while (0);

	return 1;
}

void rtk_pcie_host_shutdown(void)
{
	struct rtk_pci_controller *p;

	//assert device reset for host shutdown
#ifdef CONFIG_GENERIC_RTL86XX_PCIE_SLOT0
	p = &pci0_controller;
	p->rst_fn(p, 0);
	printk("[%s] reset device using GPIO %d\n", __func__, (int)p->rst_data);
#endif

#ifdef CONFIG_GENERIC_RTL86XX_PCIE_SLOT1
	p = &pci1_controller;
	p->rst_fn(p, 0);
	printk("[%s] reset device using GPIO %d\n", __func__, (int)p->rst_data);
#endif
}

int pcibios_plat_dev_init(struct pci_dev *dev)
{
	//printk("%s(%d)\n",__func__,__LINE__);
	printk("  devfn:%x vend:%x dev:%x cls:%x pin:%x\n",dev->devfn,dev->vendor,dev->device,dev->class,dev->pin);
	return 0;
}

int pcibios_map_irq(const struct pci_dev *dev, u8 slot, u8 pin)
{
	struct rtk_pci_controller *ctrl = pci_bus_to_rtk_controller(dev->bus);
	struct irq_desc *d = irq_to_desc(ctrl->irq);
	if (d)
		irqd_set_trigger_type(&d->irq_data, IRQ_TYPE_LEVEL_HIGH);

	return ctrl->irq;
}

static const struct pcie_para pcie0_phy_params_revB[] __initconst = {
	{ 0x00, 0x4008 },{ 0x01, 0xa812 },{ 0x02, 0x6042 },{ 0x04, 0x5000 },
	{ 0x05, 0x230a },{ 0x06, 0x0011 },{ 0x09, 0x520c },{ 0x0a, 0xc670 },
	{ 0x0b, 0xb905 },{ 0x0d, 0xef16 },{ 0x0e, 0x0000 },{ 0x20, 0x9499 },
	{ 0x21, 0x66aa },{ 0x27, 0x011a },
	{ 0x09, 0x500c },{ 0x09, 0x520c },

	{ 0x40, 0x4008 },{ 0x41, 0xa811 },{ 0x42, 0x6042 },{ 0x44, 0x5000 },
	{ 0x45, 0x230a },{ 0x46, 0x0011 },{ 0x4a, 0xc670 },{ 0x4b, 0xb905 },
	{ 0x4d, 0xef16 },{ 0x4e, 0x0000 },{ 0x4f, 0x000c },{ 0x5b, 0xaea1 },
	{ 0x5e, 0x28eb },{ 0x60, 0x94aa },{ 0x61, 0x88ff },{ 0x62, 0x0093 },
	{ 0x67, 0x011a },{ 0x6f, 0x65bd },{ 0x49, 0x500c },{ 0x49, 0x520c },
#if 0 // GEN1
	{ 0x00, 0x8a50 }, { 0x02, 0x26f9 }, { 0x03, 0x6bcd }, { 0x06, 0x104a },
	{ 0x09, 0x6307 },
	{ 0x0b, 0x0009 }, { 0x0c, 0x0800 }, { 0x20, 0x0105 }, { 0x21, 0x1000 },
#endif
	{ 0xff, 0xffff}
};
static const struct pcie_para pcie1_phy_params_revB[] __initconst = {
	{ 0x00, 0x8a50 }, { 0x02, 0x26f9 }, { 0x03, 0x6bcd }, { 0x04, 0x8049 },
	{ 0x06, 0x1088 }, { 0x07, 0x52b3 }, { 0x08, 0x5285 }, { 0x09, 0x6300 },
	{ 0x0b, 0x0009 }, { 0x0c, 0x0800 }, { 0x0e, 0x0093 }, { 0x20, 0x0105 },
	{ 0x21, 0x1000 },
	{ 0xff, 0xffff}
};

static const struct pcie_para pcie0_phy_params_revC[] __initconst = {
	{ 0x01, 0xa852 },{ 0x06, 0x0017 },{ 0x08, 0x3591 },{ 0x09, 0x520c },
	{ 0x0a, 0xf670 },{ 0x0b, 0xa90d },{ 0x0d, 0xe720 },{ 0x0e, 0x1010 },
	{ 0x1c, 0x2001 },{ 0x1e, 0x66eb },{ 0x20, 0xd4a4 },{ 0x21, 0x485a },
	{ 0x23, 0x0b66 },{ 0x24, 0x4f0c },{ 0x29, 0xf0f3 },{ 0x2b, 0xa0a1 },
	{ 0x09, 0x500c },{ 0x09, 0x520c },

	{ 0x41, 0xa849 },{ 0x46, 0x0017 },{ 0x48, 0x3591 },{ 0x49, 0x520c },
	{ 0x4a, 0xf650 },{ 0x4b, 0xa90d },{ 0x4d, 0xe720 },{ 0x4e, 0x1010 },
	{ 0x5c, 0x2001 },{ 0x60, 0xd4a6 },{ 0x61, 0x586a },{ 0x63, 0x0b66 },
	{ 0x69, 0xf0f3 },{ 0x6b, 0xa0a1 },{ 0x6f, 0x5046 },
	{ 0x49, 0x500c },{ 0x49, 0x520c },

	{ 0xff, 0xffff}
};
static const struct pcie_para pcie1_phy_params_revC[] __initconst = {
	{ 0x01, 0xa852 },{ 0x06, 0x0017 },{ 0x08, 0x3591 },{ 0x09, 0x520c },
	{ 0x0a, 0xf670 },{ 0x0b, 0xa90d },{ 0x0d, 0xe720 },{ 0x0e, 0x1010 },
	{ 0x1c, 0x2001 },{ 0x1e, 0x66eb },{ 0x20, 0xd4a4 },{ 0x21, 0x485a },
	{ 0x23, 0x0b66 },{ 0x24, 0x4f0c },{ 0x29, 0xf0f3 },{ 0x2b, 0xa0a1 },
	{ 0x09, 0x500c },{ 0x09, 0x520c },

	{ 0xff, 0xffff}
};

static const struct pcie_para *pcie0_phy_params = pcie0_phy_params_revB;
static const struct pcie_para *pcie1_phy_params = pcie1_phy_params_revB;

static int __rtk_pci_controller_init(struct rtk_pci_controller *ctrl, const struct pcie_para *phy, const char *gpio_rst_name)
{
	struct device_node *node;
	//enum of_gpio_flags flags;
	int ret;

	node = of_find_node_by_name(NULL, "board_setting");
	if (!node) {
		pr_err("Port%d: board_setting is not found in Device Tree. Initialization is aborted\n",
		       ctrl->port);
		return -ENODEV;
	}

	ret = of_get_named_gpio(node, gpio_rst_name, 0);
	if (ret < 0) {
		pr_err("Port%d: Cannot get device gpio reset pin from Device Tree. Initialization is aborted\n",
		       ctrl->port);
		return ret;
	} else {
		pr_info("Port%d Device gpio reset pin (%d)\n", ctrl->port, ret);
	}
	spin_lock_init(&ctrl->lock);
	ctrl->phy_param = phy;
	ctrl->rst_data  = ret;

	pr_info("PCIe Port%d, PCIe PHY version: %s\n", ctrl->port, ctrl->ver);
	return 0;
}

__weak struct proc_dir_entry *realtek_proc;

static void show_usage(void)
{
	printk("    w [addr] [value] : write [value] to [addr] of phy (value should be 16 bits, addr should be in [0x0-0x2F] or [0x40-0x6F] for Gen 2)\n");
}

static ssize_t rtk_pcie_phy_proc_write(struct file * file, const char __user * userbuf, size_t count, loff_t * off)
{
	char buf[32];
	int len;
	u8 addr;
	u16 val;
	struct rtk_pci_controller *p = pde_data(file_inode(file));
	void __iomem *mdiobase = p->hostext_base+0x0;

	len = min(sizeof(buf), count);
	if (copy_from_user(buf, userbuf, len))
		return -E2BIG;

	if (strncmp(buf, "help", 4) == 0) {
		show_usage();
	} else if (strncmp(buf, "w ", 2) == 0) {
		if(2 == sscanf(buf, "w %hhx %hx", &addr, &val)) {
			if (p->port == 0) {
				if ((addr > 0x2f && addr < 0x40) || addr > 0x6f)
					return -EINVAL;
			} else {
				if (addr > 0x2f)
					return -EINVAL;
			}

			REG32(mdiobase) = ((val & 0xffff) << 16) |
			                  ((addr & 0xff) << 8) | 1;
			mdelay(1);
		} else {
			goto ERROR_PARA;
		}
	} else if (strncmp(buf, "r ", 2) == 0) {
		if(1 == sscanf(buf, "r %hhx", &addr)) {
			if (p->port == 0) {
				if ((addr > 0x2f && addr < 0x40) || addr > 0x6f)
					return -EINVAL;
			} else {
				if (addr > 0x2f)
					return -EINVAL;
			}

			REG32(mdiobase) = ((addr & 0xff) << 8);
			mdelay(1);
			val = (REG32(mdiobase) >> 16 & 0xffff);

			printk("%02x = %04x\n", addr, val);
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

static int rtk_pcie_phy_show(struct seq_file *s, void *v)
{
	struct rtk_pci_controller *p = s->private;
	void __iomem *mdiobase = p->hostext_base+0x0;
	int addr, range = 0x2f;
	u16 val;

	if (soc_info.family==RTL9607_FAMILY_ID) {
		range = 0x35;
	}

	seq_printf(s, "PCIe PHY ver: %s\n", p->ver);

	if (p->port == 0)
		seq_printf(s, "Gen 1:  \tGen 2:\n");
	else
		seq_printf(s, "Gen 1:\n");

	for (addr = 0x0; addr <= range; addr++) {
		REG32(mdiobase) = ((addr & 0xff) << 8);
		mdelay(1);
		val = (REG32(mdiobase) >> 16 & 0xffff);
		mdelay(1);
		if (p->port == 0) {
			seq_printf(s, "%02x: %04x\t", addr, val);
			REG32(mdiobase) = (((addr+0x40) & 0xff) << 8);
			mdelay(1);
			val = (REG32(mdiobase) >> 16 & 0xffff);
			mdelay(1);
			seq_printf(s, "%02x: %04x\n", addr+0x40, val);
		} else {
			seq_printf(s, "%02x: %04x\n", addr, val);
		}
	}

	return 0;
}

static int rtk_pcie_phy_open(struct inode *inode, struct file *file)
{
	return(single_open(file, rtk_pcie_phy_show, pde_data(inode)));
}


static const struct proc_ops proc_ops_rtk_pcie_phy = {
	.proc_open  		= rtk_pcie_phy_open,
	.proc_write 		= rtk_pcie_phy_proc_write,
	.proc_read		= seq_read,
	.proc_lseek		= seq_lseek,
	.proc_release	= single_release,
};

static int __init rtk_pcie_init(void)
{
	struct rtk_pci_controller *p;
	struct proc_dir_entry *e;
	int ret;

	printk("RTK-PCI Init...\n");

	if (soc_info.family==RTL9607_FAMILY_ID) {
		printk("Using revC phy\n");
		pcie0_phy_params = pcie0_phy_params_revC;
		pcie1_phy_params = pcie1_phy_params_revC;
		#ifdef CONFIG_GENERIC_RTL86XX_PCIE_SLOT0
		pci0_controller.ver = RTK_PCIE_PHY_REVC_VER;
		#endif
		#ifdef CONFIG_GENERIC_RTL86XX_PCIE_SLOT0
		pci1_controller.ver = RTK_PCIE_PHY_REVC_VER;
		#endif
	}

#ifdef CONFIG_GENERIC_RTL86XX_PCIE_SLOT0
	p = &pci0_controller;
	ret = __rtk_pci_controller_init(p, pcie0_phy_params, "pci0_gpio_rst");
	if (ret < 0)
		return ret;

	/*gpio request*/
	ret = gpio_request_one(p->rst_data, GPIOF_OUT_INIT_LOW, "pci0-gpio-rst");
	if (ret) {
		pr_err("[%s] gpio request failed (%d)\n", __func__, ret);
		return ret;
	}
	p->gpiod = gpio_to_desc(p->rst_data);

	p->is_linked = rtk_pcie_reset(p);
	printk(" Port%d link=%d irq=%d\n",p->port,p->is_linked,p->irq);
	if (p->is_linked)
		register_pci_controller(&(p->controller));

	e = proc_create_data("pcie0", S_IRUGO | S_IWUSR, realtek_proc, &proc_ops_rtk_pcie_phy, p);
	if (!e) {
		printk("Failed to create procfs for pcie %d\n", p->port);
		return -EINVAL;
	}
#endif

#ifdef CONFIG_GENERIC_RTL86XX_PCIE_SLOT1
	p = &pci1_controller;
	ret = __rtk_pci_controller_init(p, pcie1_phy_params, "pci1_gpio_rst");
	if (ret < 0)
		return ret;

	/*gpio request*/
	ret = gpio_request_one(p->rst_data, GPIOF_OUT_INIT_LOW, "pci1-gpio-rst");
	if (ret) {
		pr_err("[%s] gpio request failed (%d)\n", __func__, ret);
		return ret;
	}
	p->gpiod = gpio_to_desc(p->rst_data);

	p->is_linked = rtk_pcie_reset(p);
	printk(" Port%d link=%d\n",p->port,p->is_linked);
	if (p->is_linked)
		register_pci_controller(&p->controller);

	e = proc_create_data("pcie1", S_IRUGO | S_IWUSR, realtek_proc, &proc_ops_rtk_pcie_phy, p);
	if (!e) {
		printk("Failed to create procfs for pcie %d\n", p->port);
		return -EINVAL;
	}
#endif


	printk("RTK-PCI Done\n");

	return 0;
}

late_initcall(rtk_pcie_init);
