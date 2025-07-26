// SPDX-License-Identifier: GPL-2.0

#include <linux/err.h>
#include <linux/module.h>
#include <linux/reset.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/reset-controller.h>
static int vbus_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct reset_control *reset;

	reset = of_reset_control_get(node, NULL);
	if (IS_ERR(reset)) {
		dev_err(dev, "can't find reset\n");
		return PTR_ERR(reset);
	}
	reset_control_assert(reset);
	dev_info(dev, "released\n");
	return 0;
}
static const struct of_device_id vbus_dt_ids[] = {
	{ .compatible = "rtk,vbus-chain" },
	{}
};
static struct platform_driver vbus_driver = {
	.probe		= vbus_probe,
	.driver		= {
		.name	= "vbus_chain",
		.owner	= THIS_MODULE,
		.of_match_table = vbus_dt_ids,
	},
};
static int __init vbus_init(void)
{
	return platform_driver_register(&vbus_driver);
}
late_initcall(vbus_init);
MODULE_DESCRIPTION("VBUS Control");
MODULE_LICENSE("GPL");
