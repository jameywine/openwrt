// SPDX-License-Identifier: GPL-2.0

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/regmap.h>
#include <linux/sys_soc.h>
#include <linux/mfd/syscon.h>
#include <linux/phy/phy.h>
#include <linux/reset.h>

#define PCIE_MDIO_CTRL_PHY_WRITE	BIT(0)
#define PCIE_MDIO_CTRL_PHY_ADDR_SHIFT	8
#define PCIE_MDIO_CTRL_PHY_DATA_SHIFT	16

#define PCIE_PHY_POWER_CTRL_REG		0x08
#define PHY_RESET_BIT			BIT(7)
#define ENABLE_LTSSM_BIT		BIT(0)

#define PCIE_PHY_CTRL_MDRST0		(BIT(24) | BIT(30))
#define PCIE_PHY_CTRL_MDRST1		BIT(21)
#define PCIE_PHY_CTRL_DIS0		BIT(15)
#define PCIE_PHY_CTRL_DIS1		BIT(14)

#define MAX_PCIE_PHY_DATA_SIZE		0x30
#define PHY_ADDR_PARAM1_OFFSET		0x40

#define PHY_ADDR_0X09			0x09
#define REG_0X09_FORCE_CALIBRATION	BIT(9)

#define PHY_ADDR_MAP_ARRAY_INDEX(addr)	(addr)
#define ARRAY_INDEX_MAP_PHY_ADDR(index)	(index)

struct phy_data {
	u8 addr;
	u16 data;
};

struct phy_cfg {
	int param0_size;
	struct phy_data param0[MAX_PCIE_PHY_DATA_SIZE];
	int param1_size;
	struct phy_data param1[MAX_PCIE_PHY_DATA_SIZE];
	bool do_toggle;
	u32 reset_bit;
	u32 disable_bit;
};

struct rtk_phy {
	struct device *dev;
	struct phy *phy;
	struct phy_cfg *phy_cfg;
	void __iomem *reg_mdio_ctrl;
	struct regmap *pcie_misc_map;
	struct reset_control *phy_rst;
};

static int rtk_phy_write(struct rtk_phy *rtk_phy, u8 addr, u16 data)
{
	u32 val;

	val = PCIE_MDIO_CTRL_PHY_WRITE |
	(addr << PCIE_MDIO_CTRL_PHY_ADDR_SHIFT) |
	(data << PCIE_MDIO_CTRL_PHY_DATA_SHIFT);

	writel(val, rtk_phy->reg_mdio_ctrl);

	return 0;
}

static void do_rtk_pcie_phy_toggle(struct rtk_phy *rtk_phy, bool param1)
{
	struct phy_cfg *phy_cfg = rtk_phy->phy_cfg;
	struct phy_data *phy_data;
	u8 addr;
	u16 data;
	int i;

	if (!phy_cfg->do_toggle)
		return;

	i = PHY_ADDR_MAP_ARRAY_INDEX(PHY_ADDR_0X09);

	if (param1) {
		phy_data = phy_cfg->param1 + i;
		addr = phy_data->addr + PHY_ADDR_PARAM1_OFFSET;
	} else {
		phy_data = phy_cfg->param0 + i;
		addr = phy_data->addr;
	}

	data = phy_data->data;

	rtk_phy_write(rtk_phy, addr, data & (~REG_0X09_FORCE_CALIBRATION));
	mdelay(1);
	rtk_phy_write(rtk_phy, addr, data | REG_0X09_FORCE_CALIBRATION);
}

static int rtk_phy_init(struct phy *phy)
{
	struct rtk_phy *rtk_phy = phy_get_drvdata(phy);
	struct phy_cfg *phy_cfg = rtk_phy->phy_cfg;
	int ret, i = 0;
	u32 val;

	/* PCIE phy mdio reset */
	ret = regmap_clear_bits(rtk_phy->pcie_misc_map, 0, phy_cfg->disable_bit);
	if (ret)
		return ret;

	ret = regmap_clear_bits(rtk_phy->pcie_misc_map, 0, phy_cfg->reset_bit);
	if (ret)
		return ret;

	ret = regmap_set_bits(rtk_phy->pcie_misc_map, 0, phy_cfg->reset_bit);
	if (ret)
		return ret;

	/* PCIE IP Reset */
	reset_control_deassert(rtk_phy->phy_rst);

	mdelay(10);

	/* Enable LTSSM and then reset PHY */
	val = ENABLE_LTSSM_BIT;
	writel(val, rtk_phy->reg_mdio_ctrl + PCIE_PHY_POWER_CTRL_REG);
	val |= PHY_RESET_BIT;
	writel(val, rtk_phy->reg_mdio_ctrl + PCIE_PHY_POWER_CTRL_REG);

	mdelay(50);

	/* Set param 0 */
	for (i = 0; i < phy_cfg->param0_size; i++) {
		struct phy_data *phy_data = phy_cfg->param0 + i;
		u8 addr = phy_data->addr;
		u16 data = phy_data->data;

		if (!addr && !data)
			continue;

		rtk_phy_write(rtk_phy, addr, data);

		mdelay(1);
	}

	/* toggle for param0 */
	do_rtk_pcie_phy_toggle(rtk_phy, false);

	/* Set param 1 */
	if (phy_cfg->param1_size) {

		for (i = 0; i < phy_cfg->param1_size; i++) {
			struct phy_data *phy_data = phy_cfg->param1 + i;
			u8 addr = phy_data->addr;
			u16 data = phy_data->data;

			if (!addr && !data)
				continue;

			rtk_phy_write(rtk_phy, addr + PHY_ADDR_PARAM1_OFFSET, data);

			mdelay(1);
		}

		/* toggle for param1 */
		do_rtk_pcie_phy_toggle(rtk_phy, true);
	}

	return 0;
}

static int rtk_phy_exit(struct phy *phy)
{
	return 0;
}

static const struct phy_ops ops = {
	.init		= rtk_phy_init,
	.exit		= rtk_phy_exit,
	.owner		= THIS_MODULE,
};

static int rtk_pciephy_probe(struct platform_device *pdev)
{
	struct rtk_phy *rtk_phy;
	struct device *dev = &pdev->dev;
	struct phy_provider *phy_provider;
	const struct phy_cfg *phy_cfg;

	rtk_phy = devm_kzalloc(dev, sizeof(*rtk_phy), GFP_KERNEL);
	if (!rtk_phy)
		return -ENOMEM;

	rtk_phy->reg_mdio_ctrl = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(rtk_phy->reg_mdio_ctrl))
		return dev_err_probe(dev, PTR_ERR(rtk_phy->reg_mdio_ctrl),
				     "Failed to map phy-reg-mdio base\n");

	rtk_phy->pcie_misc_map = syscon_regmap_lookup_by_phandle(dev->of_node, "realtek,pcie-misc");
	if (IS_ERR(rtk_phy->pcie_misc_map))
		return dev_err_probe(dev, PTR_ERR(rtk_phy->pcie_misc_map),
				     "Failed to map pcie-misc registers\n");

	rtk_phy->phy_rst = devm_reset_control_array_get_optional_exclusive(dev);
	if (IS_ERR(rtk_phy->phy_rst))
		return dev_err_probe(dev, PTR_ERR(rtk_phy->phy_rst),
				     "Failed to get pcie phy resets\n");

	rtk_phy->dev = dev;
	phy_cfg = of_device_get_match_data(dev);
	if (!phy_cfg)
		return dev_err_probe(dev, -EINVAL, "phy config are not assigned!\n");

	rtk_phy->phy_cfg = devm_kzalloc(dev, sizeof(*phy_cfg), GFP_KERNEL);
	if (!rtk_phy->phy_cfg)
		return dev_err_probe(dev, -ENOMEM, "Failed to allocate phy_cfg\n");;

	memcpy(rtk_phy->phy_cfg, phy_cfg, sizeof(*phy_cfg));

	platform_set_drvdata(pdev, rtk_phy);

	rtk_phy->phy = devm_phy_create(dev, NULL, &ops);
	if (IS_ERR(rtk_phy->phy))
		return dev_err_probe(dev, PTR_ERR(rtk_phy->phy),
				     "Failed to create PCIe phy\n");

	phy_set_drvdata(rtk_phy->phy, rtk_phy);

	phy_provider = devm_of_phy_provider_register(dev, of_phy_simple_xlate);
	if (IS_ERR(phy_provider))
		return PTR_ERR(phy_provider);

	return 0;
}


static const struct phy_cfg rtl9607_revB_phy_cfg0 = {
	.param0_size = MAX_PCIE_PHY_DATA_SIZE,
	.param0 = {  [1] = {0x01, 0xa852},
		    [6] = {0x06, 0x0017},
		    [8] = {0x08, 0x3591},
		    [9] = {0x09, 0x520c},
		    [10] = {0x0a, 0xf670},
		    [11] = {0x0b, 0xa90d},
		    [13] = {0x0d, 0xe720},
		    [14] = {0x0e, 0x1010},
		    [28] = {0x1c, 0x2001},
		    [30] = {0x1e, 0x66eb},
		    [32] = {0x20, 0xd4a4},
		    [33] = {0x21, 0x485a},
		    [35] = {0x23, 0x0b66},
		    [36] = {0x24, 0x4f0c},
		    [41] = {0x29, 0xf0f3},
		    [43] = {0x2b, 0xa0a1}, },
	.param1_size = MAX_PCIE_PHY_DATA_SIZE,
	.param1 = {  [1] = {0x01, 0xa849},
		    [6] = {0x06, 0x0017},
		    [8] = {0x08, 0x3591},
		    [9] = {0x09, 0x520c},
		    [10] = {0x0a, 0xf650},
		    [11] = {0x0b, 0xa90d},
		    [13] = {0x0d, 0xe720},
		    [14] = {0x0e, 0x1010},
		    [28] = {0x1c, 0x2001},
		    [32] = {0x20, 0xd4a6},
		    [33] = {0x21, 0x586a},
		    [35] = {0x23, 0x0b66},
		    [41] = {0x29, 0xf0f3},
		    [43] = {0x2b, 0xa0a1},
		    [47] = {0x2f, 0x5046}, },
	.do_toggle = true,
	.reset_bit = PCIE_PHY_CTRL_MDRST0,
	.disable_bit = PCIE_PHY_CTRL_DIS0,
};

static const struct phy_cfg rtl9607_revB_phy_cfg1 = {
	.param0_size = MAX_PCIE_PHY_DATA_SIZE,
	.param0 = {  [1] = {0x01, 0xa852},
		    [6] = {0x06, 0x0017},
		    [8] = {0x08, 0x3591},
		    [9] = {0x09, 0x520c},
		    [10] = {0x0a, 0xf670},
		    [11] = {0x0b, 0xa90d},
		    [13] = {0x0d, 0xe720},
		    [14] = {0x0e, 0x1010},
		    [28] = {0x1c, 0x2001},
		    [30] = {0x1e, 0x66eb},
		    [32] = {0x20, 0xd4a4},
		    [33] = {0x21, 0x485a},
		    [35] = {0x23, 0x0b66},
		    [36] = {0x24, 0x4f0c},
		    [41] = {0x29, 0xf0f3},
		    [43] = {0x2b, 0xa0a1}, },
	.param1_size = 0,
	.param1 = { /* no parameter */ },
	.do_toggle = true,
	.reset_bit = PCIE_PHY_CTRL_MDRST1,
	.disable_bit = PCIE_PHY_CTRL_DIS1,
};

static const struct phy_cfg rtl9607_revA_phy_cfg0 = {
	.param0_size = MAX_PCIE_PHY_DATA_SIZE,
	.param0 = {  [0] = {0x00, 0x4008},
		    [1] = {0x01, 0xa812},
		    [2] = {0x02, 0x6042},
		    [4] = {0x04, 0x5000},
		    [5] = {0x05, 0x230a},
		    [6] = {0x06, 0x0011},
		    [9] = {0x09, 0x520c},
		    [10] = {0x0a, 0xc670},
		    [11] = {0x0b, 0xb905},
		    [13] = {0x0d, 0xef16},
		    [14] = {0x0e, 0x0000},
		    [32] = {0x20, 0x9499},
		    [33] = {0x21, 0x66aa},
		    [39] = {0x27, 0x011a}, },
	.param1_size = MAX_PCIE_PHY_DATA_SIZE,
	.param1 = {  [0] = {0x00, 0x4008},
		    [1] = {0x01, 0xa811},
		    [2] = {0x02, 0x6042},
		    [4] = {0x04, 0x5000},
		    [5] = {0x05, 0x230a},
		    [6] = {0x06, 0x0011},
		    [9] = {0x09, 0x520c},
		    [10] = {0x0a, 0xc670},
		    [11] = {0x0b, 0xb905},
		    [13] = {0x0d, 0xef16},
		    [14] = {0x0e, 0x0000},
		    [15] = {0x0f, 0x000c},
		    [27] = {0x1b, 0xaea1},
		    [30] = {0x1e, 0x28eb},
		    [32] = {0x20, 0x94aa},
		    [33] = {0x21, 0x88ff},
		    [34] = {0x22, 0x0093},
		    [39] = {0x27, 0x011a},
		    [47] = {0x2f, 0x65bd}, },
	.do_toggle = true,
	.reset_bit = PCIE_PHY_CTRL_MDRST0,
	.disable_bit = PCIE_PHY_CTRL_DIS0,
};

static const struct phy_cfg rtl9607_revA_phy_cfg1 = {
	.param0_size = MAX_PCIE_PHY_DATA_SIZE,
	.param0 = {  [0] = {0x00, 0x8a50},
		    [2] = {0x02, 0x26f9},
		    [3] = {0x03, 0x6bcd},
		    [4] = {0x04, 0x8049},
		    [6] = {0x06, 0x1088},
		    [7] = {0x06, 0x52b3},
		    [8] = {0x08, 0x5285},
		    [9] = {0x09, 0x6300},
		    [11] = {0x0b, 0x0009},
		    [12] = {0x0c, 0x0800},
		    [14] = {0x0e, 0x0093},
		    [32] = {0x20, 0x0105},
		    [33] = {0x21, 0x1000}, },
	.param1_size = 0,
	.param1 = { /* no parameter */ },
	.do_toggle = false,
	.reset_bit = PCIE_PHY_CTRL_MDRST1,
	.disable_bit = PCIE_PHY_CTRL_DIS1,
};

static const struct of_device_id pciephy_rtk_dt_match[] = {
	{ .compatible = "realtek,rtl9607-revA-pciephy0", .data = &rtl9607_revA_phy_cfg0 },
	{ .compatible = "realtek,rtl9607-revA-pciephy1", .data = &rtl9607_revA_phy_cfg1 },
	{ .compatible = "realtek,rtl9607-revB-pciephy0", .data = &rtl9607_revB_phy_cfg0 },
	{ .compatible = "realtek,rtl9607-revB-pciephy1", .data = &rtl9607_revB_phy_cfg1 },
	{},
};
MODULE_DEVICE_TABLE(of, pciephy_rtk_dt_match);

static struct platform_driver rtk_pciephy_driver = {
	.probe		= rtk_pciephy_probe,
	.driver		= {
		.name	= "rtk-pciephy",
		.of_match_table = pciephy_rtk_dt_match,
	},
};

module_platform_driver(rtk_pciephy_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Realtek PCIE phy driver");
