#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/delay.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/slab.h>
//#include <dal/rtl9607c/dal_rtl9607c_switch.h>
#include "bspchip.h"
#include "bsp_automem.h"
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>

/* These functions are for 9607C/CP only */

static const int sram_size = 0x2000;
static dma_addr_t sram_map_paddr;
static unsigned long sram_map_vaddr;
#define SRAM_PLL_BASE 0xb8000204
static u32 sram_state_save_enable(void) {
	u32 old;
	old = REG32(SRAM_PLL_BASE) & 0x1;
	REG32(SRAM_PLL_BASE) |= 1;
	mdelay(1);
	return old;
}
static void sram_state_restore(u32 flags) {
	REG32(SRAM_PLL_BASE) = (REG32(SRAM_PLL_BASE) & 0xfffffffe) | flags;
	mdelay(1);
}
static void __setup_sram(void) {
	REG32(0xb8004018) = 0x8000;
	REG32(0xb8004014) = REG32(0xb8001314) = 0x6;
	REG32(0xb8004010) = REG32(0xb8001310) = sram_map_paddr | 1;
}
static void __teardown_sram(void) {
	REG32(0xb8004010) = REG32(0xb8001310) = 0;
}
static atomic_t sram_ref = ATOMIC_INIT(0);
static u32 sram_state;

static u64 soc_sram_dmamask = DMA_BIT_MASK(32);
static struct platform_device sram_dev = {
	.name           = "soc_sram",
	.id             = 0,
	.dev = {
		.dma_mask               = &soc_sram_dmamask,
		.coherent_dma_mask      = DMA_BIT_MASK(32),
	},
	.num_resources  = 0,
};


static void sram_addr_init(void){
	platform_device_register(&sram_dev);
	sram_map_vaddr = dma_alloc_coherent(&sram_dev.dev, sram_size,&sram_map_paddr, GFP_KERNEL);
}
void *plat_sram_get(void) {
	if (atomic_inc_return(&sram_ref) == 1) {
		__setup_sram();
		sram_state = sram_state_save_enable();
	}
	return (void *)sram_map_vaddr;
}
void plat_sram_put(void) {
	if (atomic_dec_and_test(&sram_ref)) {
		__teardown_sram();
		sram_state_restore(sram_state);
	}
}
unsigned int plat_sram_size(void) {
	return sram_size;
}
EXPORT_SYMBOL(plat_sram_get);
EXPORT_SYMBOL(plat_sram_put);
EXPORT_SYMBOL(plat_sram_size);
#define SRAM_TEST 0
#if SRAM_TEST
static u32 nv_counter;
static __maybe_unused int get_test_value(void *addr, u32 *ret) {
       int i;
       u32 val, *p = (u32 *)addr;
       for (i=0; i<(sram_size/sizeof(u32)); i++) {
               if (i!=0) {
                       if (val != p[i])
                               return -1;
               } else
                       val = p[i];
       }
       *ret = val;
       return 0;
}
static __maybe_unused void set_test_value(void *addr, u32 val) {
       int i;
       u32 *p = (u32 *)addr;
       for (i=0; i<(sram_size/sizeof(u32)); i++) {
               p[i] = val;
       }
}
int load_nv_counter(void) {
    int ret;
	void *s;
	s = plat_sram_get();
    ret = get_test_value((void*)s,&nv_counter);
    if (ret) {
            nv_counter = 1;
            printk("%s(%d): cold boot? set nv = %d\n", __func__,__LINE__,nv_counter);
    } else {
            printk("%s(%d): nv %d = %d\n", __func__,__LINE__, nv_counter, nv_counter+1);
            nv_counter++;
    }
	plat_sram_put();
    return ret;
}
void store_nv_counter(void) {
	void *s;
	s = plat_sram_get();
    set_test_value((void *)s, nv_counter);
    //printk("Save NV=%d\n", nv_counter);
	plat_sram_put();
}
#endif

static __init int sram_init(void) {
     void *ptr;
     sram_addr_init();
     ptr = plat_sram_get();
     printk("SRAM init@0x%px\n",ptr);
     plat_sram_put();
     return 0;
}

arch_initcall(sram_init);

