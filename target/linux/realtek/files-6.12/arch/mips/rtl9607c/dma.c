// SPDX-License-Identifier: GPL-2.0-only

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/dma-direct.h>
#include <linux/init.h>
#include <asm/mach-rtl960xc/mach-rtl960xc.h>

dma_addr_t phys_to_dma(struct device *dev, phys_addr_t paddr)
{
	dma_addr_t ret;

	if(paddr >= ZONE2_BASE) {
		ret = (paddr - ZONE2_PHY);
        pr_debug("[%s-%d]paddr=0x%08x=>daddr=0x%08x\n", __func__, __LINE__, paddr, ret);
    } else {
        ret = paddr;
    }

	return ret;
}

phys_addr_t dma_to_phys(struct device *dev, dma_addr_t daddr)
{
	phys_addr_t ret;
	if(daddr >= ZONE2_PHY) {
		ret = (daddr + ZONE2_PHY);
        pr_debug("[%s-%d]daddr=0x%08x=>paddr=0x%08x\n", __func__, __LINE__, daddr,ret);
    } else {
        ret = daddr;
    }

	return ret;
}
