/*
 * Realtek Semiconductor Corp.
 *
 * bsp/prom.c
 *     bsp early initialization code
 *
 * Copyright (C) 2006-2015 Tony Wu (tonywu@realtek.com)
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/memblock.h>
#include <asm/addrspace.h>
#include <asm/bootinfo.h>
#include <asm/prom.h>

#include <asm/fw/fw.h>
#include "rtl9607c_zones.h"
#include "rtl9607c_regs.h"

#include <asm/mips-cm.h>
GCR_ACCESSOR_RW(64, 0x010, control)

static void disable_sram(void) {
	u32 tmp;

	pr_info("Disable SRAM PLL...");

	tmp = read_gcr_control();
	tmp = (tmp & ~(1<<16)) | (1<<5); // syncdis = 1, synctl = 0;
	write_gcr_control(tmp);

	REG32(0xb8000204) &= ~1; // SRAM CLK off
	pr_info("done\n");
}

extern void prom_printf(char *fmt, ...);

/*
 * Clear the setting for
 * CPU0 Unmapped Memory Segment Address Register 0 ~ 3
 *
 */
static void inline prom_dram_unmap_clear(void){
	int i;

	for(i=0; i<4; i++){
		REG32(R_C0UMSAR0_BASE + i*0x10) = 0x0;
  }
}

void reset_sram(void){
	REG32(R_C0UMSAR0) = 0x0;
	REG32(R_C0UMSAR1) = 0x0;
	REG32(R_C0UMSAR2) = 0x0;
	REG32(R_C0UMSAR3) = 0x0;
	REG32(R_C0SRAMSAR0) = 0x0;
	REG32(R_C0SRAMSAR1) = 0x0;
	REG32(R_C0SRAMSAR2) = 0x0;
	REG32(R_C0SRAMSAR3) = 0x0;
}

/***********************************************
 * Manually set up the mem map.
 * When DRAM >= 512MB
 * 1. With DSP/IPC arch
 *   a) Not add "mem=MEMEORY_SIZE" in the kernel cmdline
 *   b) Select CONFIG_HIGHMEM in kernel option
 *   c) Set up the mem offset for DSP, IPC
 *   d) Set up the mem offset for PBO
 *
 *
 * 2. Single image( Without DSP arch)
 *   a) Not add "mem=MEMEORY_SIZE" in the kernel cmdline
 *   b) Add "highmem=xxxM" in the kernel cmdline
 *      e,q: highmem=256M
 *   c) Select CONFIG_HIGHMEM in kernel option
 *
 * Note
 *   1) The PBO offset must between 0~256MB
 *   2) The CONFIG_LUNA_PBO_DL_DRAM_OFFSET or
 *      CONFIG_LUNA_PBO_UL_DRAM_OFFSET
 *      must be the end of CPU0 MEM Size
 *   3) The DSP/IPC should be put at the end of DRAM!
 ***********************************************/
static inline void bsp_add_highmem(void){
#if defined(CONFIG_RTL8686_CPU_MEM_SIZE) && (CONFIG_RTL8686_CPU_MEM_SIZE > ZONE1_SIZE)
#if !defined(CONFIG_HIGHMEM)
#warning "*******************************************************************"
#warning "****  CONFIG_RTL8686_CPU_MEM_SIZE > 256MB !                 *******"
#warning "****  You should select CONFIG_HIGHMEM to support HIGHMEM.  *******"
#warning "*******************************************************************"
#endif
if(CONFIG_RTL8686_CPU_MEM_SIZE > ZONE1_SIZE){
    REG32(BSP_CDOR2) = ZONE2_OFF;
    REG32(BSP_CDMAR2) = (CONFIG_RTL8686_CPU_MEM_SIZE - ZONE1_SIZE) - 1;
    memblock_add(ZONE2_BASE, (CONFIG_RTL8686_CPU_MEM_SIZE - ZONE1_SIZE));
}
#else
/* No DSP arch                        */
/* We must get mem size from highmem=??   */
     char *ptr;
     char *ptr_mem;
     char *endptr;	/* local pointer to end of parsed string */
     unsigned long mem_para = 0;

     ptr = strstr(arcs_cmdline, "highmem=");
     if(ptr){
        ptr_mem= ptr+8;
        prom_printf("arcs_cmdline=%s, %p, %p\n", arcs_cmdline, arcs_cmdline, ptr);
        mem_para = simple_strtoull(ptr_mem, &endptr, 0);//MB
	if(mem_para > 0){
	   mem_para = mem_para << 20;//MB->Byte
	   prom_printf("mem_para=0x%08x\n", mem_para);
           REG32(BSP_CDOR2) = ZONE2_OFF;
           REG32(BSP_CDMAR2) = mem_para - 1;
           memblock_add(ZONE2_BASE, mem_para );
	}

     }
#endif
}
/* Add Linux Memory Zone : Normal
 * call <memblock_add> to register boot_mem_map
 * memblock_add(base, size, type);
 * type: BOOT_MEM_RAM, BOOT_MEM_ROM_DATA or BOOT_MEM_RESERVED
 */
static void bsp_add_normal(unsigned long normal_size){

    if (normal_size <= ZONE1_SIZE){
        memblock_add(0, normal_size);
     }
     else{
       /*Don't memblock_add with position within 256MB~511MB ( For CPU Logical Address space)*/
       /* The range is reserved for Device's MMIO mapping */
        memblock_add(0, ZONE1_SIZE);
     }
#ifdef CONFIG_RTW_MEMPOOL
	rtw_mempool_zero_region(0,(normal_size <= ZONE1_SIZE ? normal_size : ZONE1_SIZE));
#endif
}

void __init prom_meminit(void)
{
	char *ptr;
	unsigned int memsize;
#ifdef CONFIG_LUNA_RESERVE_DRAM_FOR_PBO
	u_long base;
#endif

	prom_dram_unmap_clear();

	/* Check the command line first for a memsize directive */
	ptr = strstr(arcs_cmdline, "mem=");

	if (ptr)
	   memsize = memparse(ptr + 4, &ptr);
	else {
        /* No memsize in command line, add a default memory region */
#ifdef CONFIG_LUNA_RESERVE_DRAM_FOR_PBO
	   memsize = min(CONFIG_LUNA_PBO_DL_DRAM_OFFSET, CONFIG_LUNA_PBO_UL_DRAM_OFFSET);
#elif defined(CONFIG_RTL8686_CPU_MEM_SIZE)
	   memsize = CONFIG_RTL8686_CPU_MEM_SIZE;
#else
	   memsize = 0x04000000;  /* Default to 64MB */
#endif
	}

#ifdef CONFIG_RTL8686_CPU_MEM_SIZE
    if (memsize > CONFIG_RTL8686_CPU_MEM_SIZE)
        memsize = CONFIG_RTL8686_CPU_MEM_SIZE;
#endif /* #ifdef CONFIG_RTL8686_CPU_MEM_SIZE */

#ifdef CONFIG_LUNA_RESERVE_DRAM_FOR_PBO
    if (memsize > CONFIG_LUNA_PBO_DL_DRAM_OFFSET)
        memsize = CONFIG_LUNA_PBO_DL_DRAM_OFFSET;
    if (memsize > CONFIG_LUNA_PBO_UL_DRAM_OFFSET)
        memsize = CONFIG_LUNA_PBO_UL_DRAM_OFFSET;
#endif /* #ifdef CONFIG_LUNA_RESERVE_DRAM_FOR_PBO */

      bsp_add_normal(memsize);

#ifdef CONFIG_LUNA_RESERVE_DRAM_FOR_PBO
    /* It is assumed that UL or DL is put on the end of DRAM and the other is put
       on the end of the previous bank to utilze the strength of para-bank accees
       E.g., For 32MB DDR2, it has 4 banks(8MB for each).
             Assuming that DL and UL use 2MB and 2MB respectively, if DL is put
             on 30MB(end of the DRAM), UL will be put on 22MB(end of the 3rd bank).
    */
    if(CONFIG_LUNA_PBO_DL_DRAM_OFFSET > CONFIG_LUNA_PBO_UL_DRAM_OFFSET) {
        base = CONFIG_LUNA_PBO_UL_DRAM_OFFSET + CONFIG_LUNA_PBO_UL_DRAM_SIZE;
        memsize = CONFIG_LUNA_PBO_DL_DRAM_OFFSET - base;
    } else {
        base = CONFIG_LUNA_PBO_DL_DRAM_OFFSET + CONFIG_LUNA_PBO_DL_DRAM_SIZE;
        memsize = CONFIG_LUNA_PBO_UL_DRAM_OFFSET - base;
    }
    //prom_printf("base=0x%08x, mem_size=0x%08x\n", base, mem_size);
    memblock_add(base, memsize);
#endif /* #ifdef CONFIG_LUNA_RESERVE_DRAM_FOR_PBO */

    /* If there is "mem" in UBoot bootargs, arcs_cmdline will be overwritten       . It is processed at
       "early_para_mem():linux-2.6.x/arch/rlx/kernel/setup.c"
       and memblock_add() is called again to update memory region 0 */

     /* For 512MB or above,
     * For Apollo Memory Mem Map
     * ZONE_NORMAL :  0~256  (MB)@Physical
     * ZONE_HIGH   :  256~xx (MB)@Physical     <----> ((256+256)~ xx+256)(MB)@CPU local AddrSpace
     */
    bsp_add_highmem();
}

const char *get_system_type(void)
{
	return "RTL9607C";
}

unsigned int BSP_MHZ, BSP_SYSCLK;
static unsigned int pll_sys_LX_freq_mhz(void)
{
	unsigned int reg_val, LX_freq;

	reg_val = REG32(0xBB01F054);
	reg_val &= (0xf);
	LX_freq = 1000/(reg_val + 5);
	BSP_MHZ = LX_freq;
	BSP_SYSCLK = BSP_MHZ*1000*1000;
	//prom_printf("##### LX_freq = %d #####\n",BSP_MHZ);

	return LX_freq;
}

uint32_t
pll_ocp_freq_mhz(void)
{
    unsigned int ocp_pll_ctrl0 = REG32(0xB8000200);
    unsigned int ocp_pll_ctrl3 = REG32(0xB800020C);
    unsigned int oc0_cmugcr    = REG32(0xB8000380);
    //printk("ocp_pll_ctrl0 = 0x%X, ocp_pll_ctrl3 = 0x%X,oc0_cmugcr = 0x%X \n", ocp_pll_ctrl0, ocp_pll_ctrl3,oc0_cmugcr);

    uint32_t cpu_freq_sel0 = (ocp_pll_ctrl0>>16) & ((1<<6)-1);
    uint32_t en_DIV2_cpu0  = (ocp_pll_ctrl3>>18) & 1;
    uint32_t cmu_mode      = (oc0_cmugcr) & ((1<<2)-1);
    uint32_t freq_div      = (oc0_cmugcr>>4) & ((1<<3)-1);

    uint32_t cpu_mhz = ((cpu_freq_sel0+2)*50)/(1<<en_DIV2_cpu0);
    if (0 != cmu_mode) {
        cpu_mhz /= (1<<freq_div);
    }
    printk("CPU clock is %u\n", cpu_mhz );
    return cpu_mhz;
}

void __init prom_free_prom_memory(void)
{
	return;
}

/* Do basic initialization */
void __init prom_init(void)
{
	extern void plat_smp_init(void);
	extern void early_uart_init(void);

	fw_init_cmdline();

#ifndef CONFIG_SDK_FPGA_PLATFORM
	pll_sys_LX_freq_mhz();
#endif

#ifdef CONFIG_SERIAL_8250_CONSOLE
	if (!strstr(arcs_cmdline, "console=")) {
		strlcat(arcs_cmdline, " console=ttyS0,115200",
			COMMAND_LINE_SIZE);
	}
#endif

#ifdef CONFIG_RTK_SOC_RTL8198D
	mips_set_machine_name("Realtek Semiconductor RTL8198D");
#else
	mips_set_machine_name("Realtek Semiconductor RTL9607C");
#endif

#ifdef CONFIG_EARLY_PRINTK
	//early_uart_init();
#endif

#ifdef CONFIG_SMP
	plat_smp_init();
#endif
	if (mips_gcr_base)
		disable_sram();
}
