#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/dma-direct.h>
#include <linux/init.h>
#include "rtl9607c_zones.h"

#ifdef DEBUG
#define DEBUG_PRINT(fmt, args...) printk(KERN_INFO fmt, ## args)
#else
#define DEBUG_PRINT(fmt, args...)
#endif

dma_addr_t phys_to_dma(struct device *dev, phys_addr_t paddr)
{
	dma_addr_t ret;

	if(paddr >= ZONE2_BASE) {
		ret = (paddr - ZONE2_PHY);
        DEBUG_PRINT("[%s-%d]paddr=0x%08x=>daddr=0x%08x\n", __func__, __LINE__, paddr, ret);
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
        DEBUG_PRINT("[%s-%d]daddr=0x%08x=>paddr=0x%08x\n", __func__, __LINE__, daddr,ret);
    } else {
        ret = daddr;
    }

	return ret;
}
