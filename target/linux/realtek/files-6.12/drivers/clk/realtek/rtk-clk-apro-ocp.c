#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/clk-provider.h>
enum APRO_CLKS {
    APRO_CLK_OCP = 0,
    APRO_CLK_LX,
    APRO_CLK_MAX,
};
static struct clk_onecell_data clk_data;
static struct clk *clks[APRO_CLK_MAX] = { 0 };

uint32_t __attribute__ ((weak))
    pll_ocp_freq_mhz(void)
{
    unsigned int ocp_pll_ctrl0 = readl((void __iomem *) 0xB8000200);
    unsigned int ocp_pll_ctrl3 = readl((void __iomem *) 0xB800020C);
    unsigned int oc0_cmugcr = readl((void __iomem *) 0xB8000380);
    //printk("ocp_pll_ctrl0 = 0x%X, ocp_pll_ctrl3 = 0x%X,oc0_cmugcr = 0x%X \n", ocp_pll_ctrl0, ocp_pll_ctrl3,oc0_cmugcr);
    uint32_t cpu_freq_sel0 = (ocp_pll_ctrl0 >> 16) & ((1 << 6) - 1);
    uint32_t en_DIV2_cpu0 = (ocp_pll_ctrl3 >> 18) & 1;
    uint32_t cmu_mode = (oc0_cmugcr) & ((1 << 2) - 1);
    uint32_t freq_div = (oc0_cmugcr >> 4) & ((1 << 3) - 1);
    uint32_t cpu_mhz = ((cpu_freq_sel0 + 2) * 50) / (1 << en_DIV2_cpu0);
    if (0 != cmu_mode) {
        cpu_mhz /= (1 << freq_div);
    }

    return cpu_mhz;
}

static unsigned int pll_sys_LX_freq_mhz(void)
{
    unsigned int reg_val, LX_freq;

    reg_val = readl((void __iomem *) 0xBB01F054);
    reg_val &= (0xf);
    LX_freq = 1000 / (reg_val + 5);

    return LX_freq * 1000 * 1000;;
}


extern uint32_t pll_ocp_freq_mhz(void);

static void __init apro_clocks_init(struct device_node *np)
{
    u32 ocp_clk = 0, lx_clk = 0;

    ocp_clk = pll_ocp_freq_mhz() * 1000 * 1000;
    clks[APRO_CLK_OCP] =
        clk_register_fixed_rate(NULL, "ocp", NULL, 0, ocp_clk);

    if (IS_ERR(clks[APRO_CLK_OCP])) {
        printk("[%s %d] apro_clocks_init failed!!!!\n", __func__,
               __LINE__);
    }


    lx_clk = pll_sys_LX_freq_mhz();
    clks[APRO_CLK_LX] =
        clk_register_fixed_rate(NULL, "LX", NULL, 0, lx_clk);

    if (IS_ERR(clks[APRO_CLK_LX])) {
        printk("[%s %d] apro_clocks_init failed!!!!\n", __func__,
               __LINE__);
    }


    clk_data.clks = clks;
    clk_data.clk_num = ARRAY_SIZE(clks);
    of_clk_add_provider(np, of_clk_src_onecell_get, &clk_data);

    printk("[%s]clk_data.clk_num=%d, ocp=%u, lx=%u\n", __func__,
           clk_data.clk_num, ocp_clk, lx_clk);

}

CLK_OF_DECLARE(apro, "realtek,apro-ocpclk", apro_clocks_init);
