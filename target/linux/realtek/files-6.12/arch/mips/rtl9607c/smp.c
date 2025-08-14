// SPDX-License-Identifier: GPL-2.0-only

/* SMP initialization code */

#include <linux/smp.h>
#include <linux/interrupt.h>
#include <asm/smp-ops.h>
#include <asm/mips-cps.h>

#include <asm/mach-rtl960xc/mach-rtl960xc.h>

void plat_smp_init(void) __init;

phys_addr_t mips_cpc_default_phys_base(void)
{
	return CPC_BASE_ADDR;
}

#define L2_BYPASS_MODE                      (1<<12)

static inline void bsp_scache_config(void){
	pr_info("L2_Bypass: %s\n",  read_c0_config2() & L2_BYPASS_MODE ? "Enable" : "Disable");
}
extern void init_l2_cache(void);
static inline void l2_cache_bypass(void){
   struct cpuinfo_mips *c = &current_cpu_data;

   write_c0_config2(  read_c0_config2() | L2_BYPASS_MODE );
   /* For setup_scache@c-r4k.c */
   c->scache.flags = MIPS_CACHE_NOT_PRESENT;
}

static void __init bsp_setup_scache(void)
{
	unsigned long config2;
	unsigned int tmp;

	config2 = read_c0_config2();
	tmp = (config2 >> 4) & 0x0f;

	/*if enable l2_bypass mode, L2cache linesize will be 0  */
	/* So We consider that user does not use L2cache and not init it */
	/*if arch not implement L2cache, linesize will always be 0     */

	/* By default, RTK preloader will set the l2cache into Uncache mode.
	 */
	if (0 < tmp && tmp <= 7){ //Scache linesize >0 and <=256 (B)
#if defined(CONFIG_MIPS_CPU_SCACHE)
	    bsp_scache_config();
	    pr_info("SCache LineSize= %d (B)\n", (1<<(tmp+1)) );
	    init_l2_cache();
#else
	 /* Kernel not to use L2cache, so force the l2cache into bypass mode.*/
	 /* L2B : bit 12 of config2 */

     l2_cache_bypass();
#endif
	}
}

/*
 * Depends on SMP type, plat_smp_init calls corresponding
 * SMP operation initializer in arch/mips/kernel
 */
void __init plat_smp_init(void)
{
	if ((0==mips_cm_probe()) && (0==mips_cpc_probe()))
		bsp_setup_scache();

	if (!register_cps_smp_ops())
		return;

	if (!register_vsmp_smp_ops())
		return;
}

