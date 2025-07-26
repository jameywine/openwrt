#ifndef RTK_GPIO_H_
#define RTK_GPIO_H_

#ifndef CONFIG_RTL9607C
#ifdef CONFIG_ARCH_CORTINA_G3LITE
#include <soc/cortina/g3lite_registers.h>
#endif
#ifdef CONFIG_CORTINA_BOARD_G3HGU_ENG
#include <soc/cortina/g3_registers.h>
#endif
#endif

struct rtk_gpio_data{
	struct device		*dev;
	struct gpio_desc	*dev_wps_gpio;
	struct gpio_desc	*dev_wifi_gpio;
	struct gpio_desc	*dev_reset_gpio;
#if defined(CONFIG_ARCH_RTL8198F)||defined(CONFIG_RTK_SOC_RTL8198D)
	struct gpio_desc	*dev_sys_led_gpio;
	struct gpio_desc	*dev_wifi_2G_led_gpio;
	struct gpio_desc        *dev_wps_led_gpio;
#endif
};
extern struct rtk_gpio_data *rtk_gpio;
#endif
