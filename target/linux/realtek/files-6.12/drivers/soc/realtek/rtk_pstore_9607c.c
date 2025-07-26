#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/pstore_ram.h>
#include <linux/slab.h>
#include <asm/cacheflush.h>

#define sram_size 0x2000
extern void* plat_sram_get(void);
extern void* plat_sram_put(void);

static void* swap_buffer;
static void *sram_vaddr;

static struct ramoops_platform_data rtk_ramoops_data = {
        .mem_size       = sram_size,
        .record_size    = sram_size,
        //.console_size   = 128,
        //.ftrace_size    = 128,
        //.pmsg_size		= 128,
};

static struct platform_device rtk_ramoops = {
        .name = "ramoops",
        .dev = {
                .platform_data = &rtk_ramoops_data,
        },
};

static int rtk_pstore_panic_handler(struct notifier_block *this,
                               unsigned long event, void *unused)
{
	/* Enable SRAM */
	/*transfer DRAM=>swap, SRAM on, swap=>SRAM*/
	memcpy(swap_buffer,sram_vaddr,sram_size);
	flush_cache_all();
	sram_vaddr = plat_sram_get();
	memcpy(sram_vaddr,swap_buffer,sram_size);

	printk("\n### [rtk_pstore_panic_handler] Enable SRAM ###\n");
	return NOTIFY_OK;
}

static struct notifier_block rtk_pstore_panic_notifier = {
        .notifier_call  = rtk_pstore_panic_handler,
        .next           = NULL,
        .priority       = 150   /* priority: INT_MAX >= x >= 0 */
};

static __init int rtk_pstore_init(void)
{
	int ret;

	swap_buffer = kzalloc(sram_size,GFP_ATOMIC);
	sram_vaddr = plat_sram_get();

	rtk_ramoops_data.mem_address = (unsigned long)sram_vaddr & ~0xa0000000;
	rtk_ramoops_data.max_reason = KMSG_DUMP_OOPS;
	printk("\n### rtk_ramoops_data.mem_address=%lx ###\n",rtk_ramoops_data.mem_address);

	ret = platform_device_register(&rtk_ramoops);
	if (ret) {
		plat_sram_put();
		printk(KERN_ERR "unable to register platform device\n");
		return ret;
	}

	atomic_notifier_chain_register(&panic_notifier_list,
                                      &rtk_pstore_panic_notifier);

	memcpy(swap_buffer,sram_vaddr,sram_size);
	flush_cache_all();
	memset(sram_vaddr,0xff,sram_size); //After swap data to buffer, clean sram data
	/* disable SRAM */
	plat_sram_put();
	/*  Restore tmp buffer to the addr mapping to SRAM */
	memcpy(sram_vaddr,swap_buffer,sram_size);

	return 0;
}

static void rtk_pstore_exit(void)
{
		platform_device_unregister(&rtk_ramoops);
}

subsys_initcall(rtk_pstore_init);
module_exit(rtk_pstore_exit);


