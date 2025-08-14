// SPDX-License-Identifier: GPL-2.0-only

/* Early intialization code for the Realtek RTL9607C SoC */

#include <asm/bootinfo.h>
#include <asm/prom.h>
#include <asm/mips-cps.h>
#include <asm/fw/fw.h>

#include <asm/mach-rtl960xc/mach-rtl960xc.h>

struct rtl96xx_soc_info soc_info;
const void *fdt;

GCR_ACCESSOR_RW(64, 0x010, control)
static void disable_sram(void) {
	u32 tmp;

	pr_info("Disable SRAM PLL...");

	tmp = read_gcr_control();
	tmp = (tmp & ~(1<<16)) | (1<<5); // syncdis = 1, synctl = 0;
	write_gcr_control(tmp);

	soc_w32(soc_r32(0x204) & ~1, 0x204); // SRAM CLK off
	pr_info("done\n");
}

void __init device_tree_init(void)
{
	if (!fdt_check_header(&__appended_dtb)) {
		fdt = &__appended_dtb;
		pr_info("Using appended Device Tree.\n");
	}
	initial_boot_params = (void *)fdt;
	unflatten_and_copy_device_tree();
}
const char *get_system_type(void)
{
	return soc_info.name;
}

void __init prom_free_prom_memory(void)
{
	return;
}

/* Do basic initialization */
void __init prom_init(void)
{
	extern void plat_smp_init(void);

	uint32_t model;
	model = sw_r32(RTL960XC_MODEL_NAME_INFO);
	pr_info("RTL960XC model is %x\n", model);
	model = model >> 16 & 0xFFFF;

	soc_info.id = model;

	switch (model) {
	case 0x9607:
		soc_info.name = "RTL9607C";
		soc_info.family = RTL9607_FAMILY_ID;
		soc_info.subtype = sw_r32(RTL9607C_CHIP_SUBTYPE_INFO) & 0x1f;
		pr_info("RTL9607C Subtype: %x\n", soc_info.subtype);
		break;
	default:
		soc_info.name = "RTL9600";
	}
	pr_info("SoC Type: %s\n", get_system_type());

	fw_init_cmdline();

	#ifdef CONFIG_SMP
		plat_smp_init();
	#endif
	if (mips_gcr_base)
		disable_sram();
}
