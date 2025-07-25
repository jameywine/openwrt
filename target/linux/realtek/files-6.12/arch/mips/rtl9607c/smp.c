/*
 * Realtek Semiconductor Corp.
 *
 * bsp/smp.c
 *     bsp SMP initialization code
 *
 * Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
 */
#include <linux/smp.h>
#include <linux/interrupt.h>

#include <asm/smp-ops.h>

#ifdef CONFIG_MIPS_CM
#include <asm/mips-cm.h>
#include <asm/mips-cpc.h>
#endif

/*
 * Called in kernel/smp-*.c to do secondary CPU initialization.
 *
 * All platform-specific initialization should be implemented in
 * this function.
 *
 * Known SMP callers are:
 *     kernel/smp-mt.c
 *     kernel/smp-cmp.c
 *
 * This function is called by secondary CPUs.
 */
void plat_smp_init_secondary(void)
{
	change_c0_status(ST0_IM, 0xff00);
}

#ifdef CONFIG_MIPS_CM
phys_addr_t mips_cpc_default_phys_base(void)
{
	return CPC_BASE_ADDR;
}
#endif


#define L2_BYPASS_MODE                      (1<<12)

#define PRINTK_L2(fmt, ...)   printk("[L2_DEBUG:(%s,%d)]" fmt, __FUNCTION__ ,__LINE__,##__VA_ARGS__)
static inline void bsp_scache_config(void){
	PRINTK_L2(" - L2_Bypass: %s\n",  read_c0_config2() & L2_BYPASS_MODE ? "Enable" : "Disable");
}
extern void init_l2_cache(void);
static void inline l2_cache_bypass(void){
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
	    PRINTK_L2("SCache LineSize= %d (B)\n", (1<<(tmp+1)) );
	    init_l2_cache();
#else
	 /* Kernel not to use L2cache, so force the l2cache into bypass mode.*/
	 /* L2B : bit 12 of config2 */

     l2_cache_bypass();
#endif
	}
}

/*
 * Called in bsp/setup.c to initialize SMP operations
 *
 * Depends on SMP type, plat_smp_init calls corresponding
 * SMP operation initializer in arch/mips/kernel
 *
 * Known SMP types are:
 *     MIPS_CMP
 *     MIPS_MT_SMP
 *     MIPS_CMP + MIPS_MT_SMP: 1004K (use MIPS_CMP)
 */
void __init plat_smp_init(void)
{
#if defined(CONFIG_MIPS_CM)
	if ((0==mips_cm_probe()) && (0==mips_cpc_probe()))
		bsp_setup_scache();

#elif defined(CONFIG_MIPS_CMP)
	gcmp_probe(GCMP_BASE_ADDR, GCMP_BASE_SIZE);
#endif

	if (!register_cps_smp_ops())
		return;

	// if (!register_cmp_smp_ops())
	// 	return;

	if (!register_vsmp_smp_ops())
		return;
}
