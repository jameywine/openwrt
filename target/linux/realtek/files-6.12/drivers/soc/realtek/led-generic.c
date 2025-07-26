#include <linux/jiffies.h>
#include "led-generic.h"
#include <linux/gpio/consumer.h>
#include "rtk_gpio.h"

static struct led_operations *led_op = 0;

void (*led_handshaking_func)(int) = 0;
void (*led_alarm_func)(int) = 0;
void (*led_act_func)(int) = 0;
void (*led_tr068_internet_act_func)(void) = 0;

#define DBG(fmt, ...) //printk(fmt, __VA_ARGS__)

static void led_flash_timer_func(struct timer_list *t) {
//	struct led_struct *p = (struct led_struct *)data;
	struct led_struct *p = from_timer(p, t, timer);

	if (!p->disable) {
	p->state = !p->state;//invert. maybe array is better.
	if (p->state)
		led_on(p->led);
	else
		led_off(p->led);
	}
	mod_timer(&(p->timer), jiffies + p->cycle);
}

static void led_generic_timer_start(struct led_struct *p,  void (*func)(struct timer_list *)) {
	//init_timer(&p->timer);
	timer_setup(&p->timer, func, 0);
	//p->timer.function = func;
//	p->timer.data = (unsigned long) p;
	p->timer.expires = jiffies + p->cycle;
	mod_timer(&(p->timer), p->timer.expires);
}

static void led_handle_flash_generic(struct led_struct *p,  unsigned short cycle) {
	del_timer(&(p->timer)); // stop it first.
	p->cycle = cycle;	
	led_generic_timer_start(p, led_flash_timer_func);
}



static void led_act_timer_func(struct timer_list *t) {
//	struct led_struct *p = (struct led_struct *)data;
	struct led_struct *p = from_timer(p, t, timer);
	
	if (p->_counter) {
		//printk(".");
		p->state = !p->state;//invert. maybe array is better.
		if (p->state) 
			led_on(p->led);
		else
			led_off(p->led);
		
		if (p->state == p->act_state)
			p->_counter--;
	}			
	mod_timer(&(p->timer), jiffies + p->cycle);
}

static void led_dummy_set(int which, int on) {
}

int led_register_operations(struct led_operations *ops) {
	if (led_op) {
		printk(KERN_ERR "LED device already registered by %s\n", led_op->name);
		return -1;
	}
	led_op = ops;
	if (!ops->handle_set)
		ops->handle_set = led_dummy_set;
	
	ops->handle_init();
	
	return 0;
}

void led_on(int which) {
	DBG("%s: %d\n", __FUNCTION__, which);
#if defined(CONFIG_ARCH_RTL8198F)||defined(CONFIG_RTK_SOC_RTL8198D)
#ifndef CONFIG_LEDS_GPIO
#ifdef CONFIG_RTK_RESET_WPS_SHARE_LED
	if(((which>=LED_POWER)&&(which<=LED_POWER_RED))
			||((which>=LED_WPS_GREEN)&&(which<=LED_WPS_YELLOW))){
	if(rtk_gpio->dev_sys_led_gpio)
		gpiod_set_value(rtk_gpio->dev_sys_led_gpio, 1);
	}
#else
	if(rtk_gpio->dev_wps_led_gpio)
		gpiod_set_value(rtk_gpio->dev_wps_led_gpio, 1);
#endif
#endif
#else
	if (led_op)
		led_op->handle_set(which, LED_ON);
#endif
}

void led_off(int which) {
	DBG("%s: %d\n", __FUNCTION__, which);
#if defined(CONFIG_ARCH_RTL8198F)||defined(CONFIG_RTK_SOC_RTL8198D)
#ifndef CONFIG_LEDS_GPIO
#ifdef CONFIG_RTK_RESET_WPS_SHARE_LED
	if(((which>=LED_POWER)&&(which<=LED_POWER_RED))
			||((which>=LED_WPS_GREEN)&&(which<=LED_WPS_YELLOW))){
	if(rtk_gpio->dev_sys_led_gpio)
		gpiod_set_value(rtk_gpio->dev_sys_led_gpio, 0);
	}
#else
	if(rtk_gpio->dev_wps_led_gpio)
		gpiod_set_value(rtk_gpio->dev_wps_led_gpio, 0);
#endif
#endif
#else
	if (led_op)
		led_op->handle_set(which, LED_OFF);
#endif
}

void led_flash_start(struct led_struct *p, int which, unsigned int cycle) {
	DBG("%s: led %d, cycle %d * 10ms\n", __FUNCTION__, which, cycle);
	p->led = which;
	led_handle_flash_generic(p, cycle);
}

void led_flash_stop(struct led_struct *p) {
	DBG("%s: led %d\n", __FUNCTION__, p->led);
	del_timer(&(p->timer)); 
}

void led_act_start(struct led_struct *p, int which, unsigned int cycle, unsigned char backlog) {
	led_act_stop(p);
	p->cycle = cycle;
	p->led = which;
	p->backlog = backlog;
	//init_timer(&p->timer);

	p->state = p->act_state = 1;
	timer_setup(&p->timer, led_act_timer_func, 0);
//	p->timer.function = led_act_timer_func;
//	p->timer.data = (unsigned long) p;
	p->timer.expires = jiffies + p->cycle;
	mod_timer(&(p->timer), p->timer.expires);
}

void led_act_stop(struct led_struct *p) {
	p->_counter = 0;
	del_timer(&(p->timer));
}


/*********************************************************
 * Device Tree Blob 
 **********************************************************/
#include <linux/platform_device.h>
#include <linux/of_address.h>
#include <linux/module.h>
int rtk_led_num[LED_TOTAL] = {[0 ... (LED_TOTAL - 1)] = -1};
static int rtk_led_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
	struct device_node *np =dev->of_node;
	u32 led_num = 0;
    int ret = 0;
	ret = of_property_read_u32(np, "LED_POWER_GREEN", &led_num);
	if (ret == 0)
		rtk_led_num[LED_POWER_GREEN] = led_num;
	
	ret = of_property_read_u32(np, "LED_WPS_GREEN", &led_num);
	if (ret == 0)
		rtk_led_num[LED_WPS_GREEN] = led_num;
	
	ret = of_property_read_u32(np, "LED_INTERNET_GREEN", &led_num);
	if (ret == 0)
		rtk_led_num[LED_INTERNET_GREEN] = led_num;
	
	
	printk("RTK LED Driver : LED_POWER_GREEN:LED(%d), LED_WPS_GREEN:LED(%d), LED_INTERNET_GREEN:LED(%d)\n ",rtk_led_num[LED_POWER_GREEN], rtk_led_num[LED_WPS_GREEN],rtk_led_num[LED_INTERNET_GREEN]);
	
	return 0;
}

static int rtk_led_remove(struct platform_device *pdev)
{

	return 0;
}
static const struct of_device_id rtk_led_dt_match[] = {
	{ .compatible = "realtek,g3hgu-led" },
    { .compatible = "realtek,g3lite-led" },
	{ },
};

//MODULE_DEVICE_TABLE(of, rtk_led_dt_match);

static struct platform_driver rtk_led_driver = {
	.driver = {
		.name		= "rtk_led",
		.owner		= THIS_MODULE,
		.of_match_table	= rtk_led_dt_match,
	},
	.probe = rtk_led_probe,
	.remove	= rtk_led_remove,
};


/******** Module init/exit **********************/
static int __init rtk_prco_led_init(void)
{
    platform_driver_register(&rtk_led_driver);
	return 0;
}


static void __exit rtk_prco_led_exit(void)
{
	//printk("Unload Realtek GPIO Driver \n");
    platform_driver_unregister(&rtk_led_driver);
}

module_init(rtk_prco_led_init);
module_exit(rtk_prco_led_exit);


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("RTK LED driver");

