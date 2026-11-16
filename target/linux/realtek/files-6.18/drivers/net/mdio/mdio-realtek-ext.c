// SPDX-License-Identifier: GPL-2.0-only

#include <linux/fwnode.h>
#include <linux/fwnode_mdio.h>
#include <linux/mfd/syscon.h>
#include <linux/of.h>
#include <linux/of_mdio.h>
#include <linux/phy.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/types.h>

#define RTMDIO_MAX_PHY				32
#define RTMDIO_MAX_SMI_BUS			2
#define RTMDIO_PAGE_SELECT			0x1f

#define RTMDIO_RUN				BIT(0)

#define RTMDIO_960X_CFG_POLL_MDX_CTRL		(0x23028)
#define   RTMDIO_960X_CFG_MDC_SET0_EN		BIT(19)
#define   RTMDIO_960X_CFG_MDC_SET1_EN		BIT(20)
#define   RTMDIO_960X_CFG_PREAMBLE_SET0_EN	BIT(4)
#define   RTMDIO_960X_CFG_PREAMBLE_SET1_EN	BIT(5)
#define RTMDIO_960X_CFG_POLL_CTRL_0		(0x2302C)
// TODO try different values with logic analyzer
#define   RTMDIO_960X_SMI_FMT_SET0		GENMASK(1, 0)
#define   RTMDIO_960X_SMI_FMT_SET1		GENMASK(3, 2)
#define   RTMDIO_960X_SMI_FMT_C22		0
#define   RTMDIO_960X_SMI_FMT_C45		0x2
#define RTMDIO_960X_CFG_POLL_CTRL_1		(0x23030)
#define RTMDIO_960X_CFG_POLL_MDX_PMSK		(0x23034)
#define RTMDIO_960X_CFG_POLL_MDX_ADDR		(0x23038)
#define   RTMDIO_960X_CFG_POLL_MDX_PORT_MASK	GENMASK(4, 0)
#define RTMDIO_960X_CFG_10GPHY_POLLING_SEL4	(0x23050)
#define RTMDIO_960X_CFG_10GPHY_POLLING_SEL3	(0x23054)
#define RTMDIO_960X_CFG_10GPHY_POLLING_SEL2	(0x23058)
#define RTMDIO_960X_CFG_10GPHY_POLLING_SEL1	(0x2305C)
#define RTMDIO_960X_CFG_GPHY_POLLING_SEL	(0x23060)
#define RTMDIO_960X_CFG_10GPHY_POLLING_SEL0	(0x23064)
#define RTMDIO_960X_SMI_INDRT_ACCESS_CTRL_0	(0x230B8)
#define   RTMDIO_960X_CMD_FAIL			0 /* non-functional */
#define   RTMDIO_960X_CMD_READ			0
#define   RTMDIO_960X_CMD_WRITE			BIT(4)
#define   RTMDIO_960X_FMT_C22			0
#define   RTMDIO_960X_FMT_C45			BIT(3)
#define   RTMDIO_960X_CMD_MASK			GENMASK(4, 0)
#define RTMDIO_960X_SMI_INDRT_ACCESS_CTRL_1	(0x230BC)
#define RTMDIO_960X_SMI_INDRT_ACCESS_CTRL_2	(0x230C0)
#define RTMDIO_960X_SMI_INDRT_ACCESS_CTRL_3	(0x230C4)
#define RTMDIO_960X_SMI_INDRT_ACCESS_BC_CTRL	(0x230C8)
#define RTMDIO_960X_SMI_INDRT_ACCESS_MMD_CTRL	(0x230CC)
#define RTMDIO_960X_IO_MODE_EN			(0x23014)
#define   RTMDIO_960X_IO_MDIO_SET0_EN		BIT(10)
#define   RTMDIO_960X_IO_MDIO_SET1_EN		BIT(11)
// Master bits doesn't do anything?
//#define   RTMDIO_960X_IO_MDIO_SET0_MASTER	BIT(8)
//#define   RTMDIO_960X_IO_MDIO_SET1_MASTER	BIT(9)

#define rtmdio_ctrl_from_bus(bus) \
	(((struct rtmdio_chan *)(bus)->priv)->ctrl)

struct rtmdio_port {
	struct device_node *dn;
	int page;
};

struct rtmdio_ctrl {
	struct mutex lock;
	struct regmap *map;
	const struct rtmdio_config *cfg;
};

struct rtmdio_chan {
	struct rtmdio_ctrl *ctrl;
	struct rtmdio_port port[RTMDIO_MAX_PHY];
	unsigned int bus_id;
};

struct rtmdio_config {
	int (*read_phy)(struct mii_bus *bus, u32 addr, u32 page, u32 reg, u32 *val);
	int (*write_phy)(struct mii_bus *bus, u32 addr, u32 page, u32 reg, u32 val);
	int (*read_mmd_phy)(struct mii_bus *bus, u32 addr, u32 devnum, u32 regnum, u32 *val);
	int (*write_mmd_phy)(struct mii_bus *bus, u32 addr, u32 devnum, u32 regnum, u32 val);
};

static int rtmdio_run_cmd(struct mii_bus *bus, int cmd, int mask, int regnum, int fail)
{
	struct rtmdio_ctrl *ctrl = rtmdio_ctrl_from_bus(bus);
	int ret, val;

	ret = regmap_update_bits(ctrl->map, regnum, mask, cmd | RTMDIO_RUN);
	ret = regmap_read_poll_timeout(ctrl->map, regnum, val, !(val & RTMDIO_RUN), 20, 500000);
	if (ret)
		WARN_ONCE(1, "mdio bus access timed out\n");
	else if (val & fail) {
		WARN_ONCE(1, "mdio bus access failed\n");
		ret = -EIO;
	}

	return ret;
}

static int rtmdio_960x_run_cmd(struct mii_bus *bus, int cmd)
{
	return rtmdio_run_cmd(bus, cmd, RTMDIO_960X_CMD_MASK,
			      RTMDIO_960X_SMI_INDRT_ACCESS_CTRL_0, RTMDIO_960X_CMD_FAIL);
}

static int rtmdio_960x_read_phy(struct mii_bus *bus, u32 addr, u32 page, u32 reg, u32 *val)
{
	struct rtmdio_ctrl *ctrl = rtmdio_ctrl_from_bus(bus);
	struct rtmdio_chan *priv = (struct rtmdio_chan *)(bus)->priv;
	unsigned int smi_fmt_mask = priv->bus_id == 0 ? RTMDIO_960X_SMI_FMT_SET0 : RTMDIO_960X_SMI_FMT_SET1;
	unsigned int smi_fmt_val = priv->bus_id == 0 ? FIELD_PREP(RTMDIO_960X_SMI_FMT_SET0, RTMDIO_960X_SMI_FMT_C22) : FIELD_PREP(RTMDIO_960X_SMI_FMT_SET1, RTMDIO_960X_SMI_FMT_C22);
	int err;

	regmap_update_bits(ctrl->map, RTMDIO_960X_CFG_POLL_CTRL_0, smi_fmt_mask, smi_fmt_val);
	regmap_write(ctrl->map, RTMDIO_960X_CFG_POLL_MDX_ADDR, addr | (priv->bus_id << 5));
	regmap_write(ctrl->map, RTMDIO_960X_SMI_INDRT_ACCESS_CTRL_0, reg << 6 | page << 11);

	err = rtmdio_960x_run_cmd(bus, RTMDIO_960X_CMD_READ | RTMDIO_960X_FMT_C22);
	if (!err)
		err = regmap_read(ctrl->map, RTMDIO_960X_SMI_INDRT_ACCESS_CTRL_3, val);
	if (!err)
		*val >>= 16;

	return err;
}

static int rtmdio_960x_write_phy(struct mii_bus *bus, u32 addr, u32 page, u32 reg, u32 val)
{
	struct rtmdio_ctrl *ctrl = rtmdio_ctrl_from_bus(bus);
	struct rtmdio_chan *priv = (struct rtmdio_chan *)(bus)->priv;
	unsigned int smi_fmt_mask = priv->bus_id == 0 ? RTMDIO_960X_SMI_FMT_SET0 : RTMDIO_960X_SMI_FMT_SET1;
	unsigned int smi_fmt_val = priv->bus_id == 0 ? FIELD_PREP(RTMDIO_960X_SMI_FMT_SET0, RTMDIO_960X_SMI_FMT_C22) : FIELD_PREP(RTMDIO_960X_SMI_FMT_SET1, RTMDIO_960X_SMI_FMT_C22);

	regmap_update_bits(ctrl->map, RTMDIO_960X_CFG_POLL_CTRL_0, smi_fmt_mask, smi_fmt_val);
	regmap_write(ctrl->map, RTMDIO_960X_CFG_POLL_MDX_ADDR, addr | (priv->bus_id << 5));
	regmap_write(ctrl->map, RTMDIO_960X_SMI_INDRT_ACCESS_CTRL_3, val);
	regmap_write(ctrl->map, RTMDIO_960X_SMI_INDRT_ACCESS_CTRL_0, reg << 6 | page << 11);

	return rtmdio_960x_run_cmd(bus, RTMDIO_960X_CMD_WRITE | RTMDIO_960X_FMT_C22);
}

static int rtmdio_960x_read_mmd_phy(struct mii_bus *bus, u32 addr, u32 devnum, u32 regnum, u32 *val)
{
	struct rtmdio_ctrl *ctrl = rtmdio_ctrl_from_bus(bus);
	struct rtmdio_chan *priv = (struct rtmdio_chan *)(bus)->priv;
	unsigned int smi_fmt_mask = priv->bus_id == 0 ? RTMDIO_960X_SMI_FMT_SET0 : RTMDIO_960X_SMI_FMT_SET1;
	unsigned int smi_fmt_val = priv->bus_id == 0 ? FIELD_PREP(RTMDIO_960X_SMI_FMT_SET0, RTMDIO_960X_SMI_FMT_C45) : FIELD_PREP(RTMDIO_960X_SMI_FMT_SET1, RTMDIO_960X_SMI_FMT_C45);
	int err;

	regmap_update_bits(ctrl->map, RTMDIO_960X_CFG_POLL_CTRL_0, smi_fmt_mask, smi_fmt_val);
	regmap_write(ctrl->map, RTMDIO_960X_CFG_POLL_MDX_ADDR, addr | (priv->bus_id << 5));
	regmap_write(ctrl->map, RTMDIO_960X_SMI_INDRT_ACCESS_MMD_CTRL, (devnum << 16) | (regnum & 0xffff));

	err = rtmdio_960x_run_cmd(bus, RTMDIO_960X_CMD_READ | RTMDIO_960X_FMT_C45);
	if (!err)
		err = regmap_read(ctrl->map, RTMDIO_960X_SMI_INDRT_ACCESS_CTRL_3, val);
	if (!err)
		*val >>= 16;

	return err;
}

static int rtmdio_960x_write_mmd_phy(struct mii_bus *bus, u32 addr, u32 devnum, u32 regnum, u32 val)
{
	struct rtmdio_ctrl *ctrl = rtmdio_ctrl_from_bus(bus);
	struct rtmdio_chan *priv = (struct rtmdio_chan *)(bus)->priv;
	unsigned int smi_fmt_mask = priv->bus_id == 0 ? RTMDIO_960X_SMI_FMT_SET0 : RTMDIO_960X_SMI_FMT_SET1;
	unsigned int smi_fmt_val = priv->bus_id == 0 ? FIELD_PREP(RTMDIO_960X_SMI_FMT_SET0, RTMDIO_960X_SMI_FMT_C45) : FIELD_PREP(RTMDIO_960X_SMI_FMT_SET1, RTMDIO_960X_SMI_FMT_C45);

	regmap_update_bits(ctrl->map, RTMDIO_960X_CFG_POLL_CTRL_0, smi_fmt_mask, smi_fmt_val);
	regmap_write(ctrl->map, RTMDIO_960X_CFG_POLL_MDX_ADDR, addr | (priv->bus_id << 5));
	regmap_write(ctrl->map, RTMDIO_960X_SMI_INDRT_ACCESS_CTRL_3, val);
	regmap_write(ctrl->map, RTMDIO_960X_SMI_INDRT_ACCESS_MMD_CTRL, (devnum << 16) | (regnum & 0xffff));

	return rtmdio_960x_run_cmd(bus, RTMDIO_960X_CMD_WRITE | RTMDIO_960X_FMT_C45);
}

static int rtmdio_read(struct mii_bus *bus, int phy, int regnum)
{
	struct rtmdio_ctrl *ctrl = rtmdio_ctrl_from_bus(bus);
	struct rtmdio_chan *priv = (struct rtmdio_chan *)(bus)->priv;
	int err, val;

	guard(mutex)(&ctrl->lock);
	if (regnum == RTMDIO_PAGE_SELECT)
		return priv->port[phy].page;

	err = (*ctrl->cfg->read_phy)(bus, phy, priv->port[phy].page, regnum, &val);
	return err ? err : val;
}

static int rtmdio_write(struct mii_bus *bus, int phy, int regnum, u16 val)
{
	struct rtmdio_ctrl *ctrl = rtmdio_ctrl_from_bus(bus);
	struct rtmdio_chan *priv = (struct rtmdio_chan *)(bus)->priv;
	int err, page;

	guard(mutex)(&ctrl->lock);
	page = priv->port[phy].page;

	if (regnum == RTMDIO_PAGE_SELECT) {
		priv->port[phy].page = val;
		return 0;
	}

	err = (*ctrl->cfg->write_phy)(bus, phy, page, regnum, val);
	return err;
}

static int rtmdio_read_c45(struct mii_bus *bus, int phy, int devnum, int regnum)
{
	struct rtmdio_ctrl *ctrl = rtmdio_ctrl_from_bus(bus);
	int err, val;

	guard(mutex)(&ctrl->lock);
	err = (*ctrl->cfg->read_mmd_phy)(bus, phy, devnum, regnum, &val);

	return err ? err : val;
}

static int rtmdio_write_c45(struct mii_bus *bus, int phy, int devnum, int regnum, u16 val)
{
	struct rtmdio_ctrl *ctrl = rtmdio_ctrl_from_bus(bus);
	int err;

	guard(mutex)(&ctrl->lock);
	err = (*ctrl->cfg->write_mmd_phy)(bus, phy, devnum, regnum, val);

	return err;
}

static void rtmdio_960x_init(struct mii_bus *bus)
{
	struct rtmdio_ctrl *ctrl = rtmdio_ctrl_from_bus(bus);
	struct rtmdio_chan *priv = (struct rtmdio_chan *)(bus)->priv;

	switch (priv->bus_id) {
	case 0:
		// TODO reset on unload
		regmap_set_bits(ctrl->map, RTMDIO_960X_IO_MODE_EN, RTMDIO_960X_IO_MDIO_SET0_EN);
		regmap_update_bits(ctrl->map, RTMDIO_960X_CFG_POLL_MDX_CTRL, RTMDIO_960X_CFG_MDC_SET0_EN | RTMDIO_960X_CFG_PREAMBLE_SET0_EN, RTMDIO_960X_CFG_MDC_SET0_EN);
		break;
	case 1:
		regmap_set_bits(ctrl->map, RTMDIO_960X_IO_MODE_EN, RTMDIO_960X_IO_MDIO_SET1_EN);
		regmap_update_bits(ctrl->map, RTMDIO_960X_CFG_POLL_MDX_CTRL, RTMDIO_960X_CFG_MDC_SET1_EN | RTMDIO_960X_CFG_PREAMBLE_SET1_EN, RTMDIO_960X_CFG_MDC_SET1_EN);
		break;
	}

	/*
	 * Use PHY from RTMDIO_960X_CFG_POLL_MDX_ADDR to write to
	 */
	regmap_write(ctrl->map, RTMDIO_960X_SMI_INDRT_ACCESS_CTRL_2, BIT(0));
}

static int rtmdio_probe_bus(struct device *dev, struct rtmdio_ctrl *ctrl, struct fwnode_handle *fw_bus)
{
	struct rtmdio_chan *chan;
	struct mii_bus *bus;
	int ret;
	unsigned int bus_id;

	ret = fwnode_property_read_u32(fw_bus, "reg", &bus_id);
	if (ret)
		return dev_err_probe(dev, ret, "%pfwP no bus number\n", fw_bus);
	if (bus_id >= RTMDIO_MAX_SMI_BUS)
		return dev_err_probe(dev, -EINVAL, "%pfwP illegal bus number\n", fw_bus);

	bus = devm_mdiobus_alloc_size(dev, sizeof(*chan));
	if (!bus)
		return -ENOMEM;

	chan = bus->priv;
	chan->ctrl = ctrl;
	chan->bus_id = bus_id;

	bus->name = "Realtek MDIO bus";
	bus->read = rtmdio_read;
	bus->write = rtmdio_write;
	bus->read_c45 = rtmdio_read_c45;
	bus->write_c45 = rtmdio_write_c45;
	snprintf(bus->id, MII_BUS_ID_SIZE, "realtek-mdio-ext-%d", bus_id);

	rtmdio_960x_init(bus);

	ret = devm_of_mdiobus_register(dev, bus, to_of_node(fw_bus));
	if (ret)
		return dev_err_probe(dev, ret, "cannot register MDIO bus\n");

	return 0;
}

static int rtmdio_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct rtmdio_ctrl *ctrl;
	int ret;

	ctrl = devm_kzalloc(dev, sizeof(*ctrl), GFP_KERNEL);
	if (!ctrl)
		return -ENOMEM;

	ret = devm_mutex_init(dev, &ctrl->lock);
	if (ret)
		return ret;

	platform_set_drvdata(pdev, ctrl);
	ctrl->cfg = (const struct rtmdio_config *)device_get_match_data(dev);
	ctrl->map = syscon_node_to_regmap(pdev->dev.of_node->parent);
	if (IS_ERR(ctrl->map))
		return PTR_ERR(ctrl->map);

	device_for_each_child_node_scoped(dev, child) {
		ret = rtmdio_probe_bus(dev, ctrl, child);
		if (ret)
			return ret;
	}

	regmap_write(ctrl->map, RTMDIO_960X_CFG_POLL_MDX_PMSK, 0);

	return 0;
}

static const struct rtmdio_config rtmdio_960x_ext_cfg = {
	.read_phy	= rtmdio_960x_read_phy,
	.write_phy	= rtmdio_960x_write_phy,
	.read_mmd_phy	= rtmdio_960x_read_mmd_phy,
	.write_mmd_phy	= rtmdio_960x_write_mmd_phy,
};

static const struct of_device_id rtmdio_ids[] = {
	{
		.compatible = "realtek,rtl9607-ext-mdio",
		.data = &rtmdio_960x_ext_cfg,
	},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, rtmdio_ids);

static struct platform_driver rtmdio_driver = {
	.probe = rtmdio_probe,
	.driver = {
		.name = "mdio-rtl9607-ext",
		.of_match_table = rtmdio_ids,
	},
};

module_platform_driver(rtmdio_driver);

MODULE_DESCRIPTION("RTL9607C EXT-MDIO driver");
MODULE_LICENSE("GPL");
