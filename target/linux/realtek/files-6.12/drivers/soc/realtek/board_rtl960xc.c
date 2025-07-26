#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <dt-bindings/gpio/gpio.h>
#include <rtk/i2c.h>
#include <rtk/init.h>
#include <rtk/led.h>
#include <common/error.h>
#include "pushbutton.h"
#include "led-generic.h"


/* set board led initial state */
static void board_init_led(void) {
  rtk_led_config_t led_cfg;

  memset(&led_cfg, 0, sizeof(led_cfg));

  led_cfg.ledEnable[LED_CONFIG_SPD1000ACT] = ENABLED;
  led_cfg.ledEnable[LED_CONFIG_SPD500ACT] = ENABLED;
  led_cfg.ledEnable[LED_CONFIG_SPD100ACT] = ENABLED;
  led_cfg.ledEnable[LED_CONFIG_SPD10ACT] = ENABLED;
  led_cfg.ledEnable[LED_CONFIG_SPD1000] = ENABLED;
  led_cfg.ledEnable[LED_CONFIG_SPD500] = ENABLED;
  led_cfg.ledEnable[LED_CONFIG_SPD100] = ENABLED;
  led_cfg.ledEnable[LED_CONFIG_SPD10] = ENABLED;

  rtk_led_operation_set(LED_OP_PARALLEL);
  rtk_led_parallelEnable_set(2, ENABLED);  // port 0
  rtk_led_parallelEnable_set(3, ENABLED);  // port 1
  rtk_led_parallelEnable_set(17, ENABLED); // port 2
  rtk_led_parallelEnable_set(5, ENABLED);  // port 3
  //rtk_led_parallelEnable_set(16, ENABLED); // port 4
  //rtk_led_parallelEnable_set(4, ENABLED);  // port 5
  rtk_led_parallelEnable_set(9, ENABLED);  // LOS

  rtk_led_config_set(2,  LED_TYPE_UTP0,  &led_cfg);
  rtk_led_config_set(3,  LED_TYPE_UTP1,  &led_cfg);
  rtk_led_config_set(17, LED_TYPE_UTP2,  &led_cfg);
  rtk_led_config_set(5,  LED_TYPE_UTP3,  &led_cfg);
  //rtk_led_config_set(16, LED_TYPE_UTP4,  &led_cfg);
  //rtk_led_config_set(4,  LED_TYPE_FIBER, &led_cfg);
  rtk_led_config_set(9,  LED_TYPE_PON,   &led_cfg);
  
  //memset(&led_cfg, 0x0, sizeof(rtk_led_config_t));
  //led_cfg.ledEnable[LED_CONFIG_FORCE_MODE] = ENABLED;	
  //rtk_led_config_set(0, LED_TYPE_PON, &led_cfg); // power led
  //rtk_led_modeForce_set(0, LED_FORCE_BLINK);
  if(rtk_led_blinkRate_set(LED_BLINK_GROUP_FORCE_MODE, LED_BLINKRATE_256MS) != RT_ERR_OK)
	  printk("\n %s %d\n",__FUNCTION__,__LINE__);
}

void rtk_board_mptest_led(int op) {
  switch(op) {
	case 0: /* all off */
	case 1: /* all on */
		break;
	default: /* restore to default */
		board_init_led();
  }
}
//EXPORT_SYMBOL(rtk_board_mptest_led);

#ifndef CONFIG_PUSHBUTTON_USE_EVENT_HANDLE
static int gpio_reset = -1;
static int gpio_wifi  = -1;
static enum of_gpio_flags gpio_reset_flags;
static enum of_gpio_flags gpio_wifi_flags;
static void board_pb_init(void) {
};

static int board_pb_is_pushed(int which) {
  int ret = 0;
  switch(which) {
  case PB_RESET:
    if (gpio_reset>=0) {
		ret = gpio_get_value(gpio_reset);
		if (GPIO_ACTIVE_LOW&gpio_reset_flags)
			ret = !ret;
	}
	break;
  case PB_WIFISW:
    if (gpio_wifi>=0) {
		ret = gpio_get_value(gpio_wifi);
		if (GPIO_ACTIVE_LOW&gpio_wifi_flags)
			ret = !ret;
	}
	break;
  }
  return ret;
}

static struct pushbutton_operations board_pb_op = {
  .handle_init = board_pb_init,
  .handle_is_pushed = board_pb_is_pushed,
};
#endif

static int gpio_internet = -1;
static int gpio_power = -1;
static int gpio_wps_g = -1;
static int gpio_usb = -1;
static int gpio_fxs = -1;
static int gpio_internet_flags=0;
static int gpio_power_flags=0;
static int gpio_wps_g_flags=0;
// static int gpio_usb_flags=0;
// static int gpio_fxs_flags=0;
#define GPIO_SET(w, flag, op)  do { \
        if (w<0) break; \
        if (GPIO_ACTIVE_LOW&flag) gpio_set_value(w,!op); \
        else gpio_set_value(w,op); \
} while (0);
static void board_01_handle_set(int which, int op) {
        switch (which) {
        case LED_POWER_GREEN:
                GPIO_SET(gpio_power,gpio_power_flags,op);
                break;
        case LED_INTERNET_GREEN:
                GPIO_SET(gpio_internet,gpio_internet_flags,op);
                break;
        case LED_WPS_GREEN:
                GPIO_SET(gpio_wps_g,gpio_wps_g_flags,op);
                break;
#ifdef CONFIG_RTL8672_SW_USB_LED
        case LED_USB_0:
                GPIO_SET(gpio_usb,gpio_usb_flags,op);
                break;
#endif
		// case LED_FXS:
  //               GPIO_SET(gpio_fxs,gpio_fxs_flags,op);
  //               break;
        default:
                led_handle_set(which, op);
        }
}
static void board_01_handle_init(void) {
		enum led_state {off,on};
   do {
          struct device_node *dn, *child;
          dn = of_find_compatible_node(NULL, NULL, "gpio-leds");
          if (dn){
                  const char *led_name;
                  for_each_child_of_node(dn, child) {
                          //enum of_gpio_flags flags;
                          int gpio;
                          of_property_read_string(child, "label", &led_name);
                          gpio = of_get_named_gpio(child, "gpios", 0);
                          if (led_name && (gpio >= 0)) {
                                  if (!strcmp("LED_PPP_G", led_name)) {
                                          gpio_internet = gpio;
                                          // gpio_internet_flags = flags;
                                  }
                                  else if (!strcmp("LED_PWR_G", led_name))
                                  {
                                          gpio_power = gpio;
                                          // gpio_power_flags = flags;
                                  }
                                  else if (!strcmp("LED_WPS_G", led_name))
                                  {
                                          gpio_wps_g = gpio;
                                          // gpio_wps_g_flags = flags;
                                  }
                                  else if (!strcmp("LED_USB0", led_name))
                                  {
                                          gpio_usb = gpio;
                                          // gpio_usb_flags = flags;
                                  }
								  else if (!strcmp("LED_FXS", led_name))
                                  {
                                          gpio_fxs = gpio;
                                          // gpio_fxs_flags = flags;
                                  }
                          }
                  }
          }
  } while (0);
};
static struct led_operations board_01_operation = {
        .name = "board_01",
        .handle_init = board_01_handle_init,
        .handle_set = board_01_handle_set,
};
static int __init board_init(void) {

#ifdef CONFIG_PCIE_POWER_SAVING
  REG32(IO_MODE_EN_REG) |= IO_MODE_INTRPT1_EN; // ENABLE interrupt 1
#endif
  rtk_core_init();
  rtk_i2c_init(0);
  if(rtk_led_init() != RT_ERR_OK)
	  printk("\n %s %d\n",__FUNCTION__,__LINE__);
  board_init_led();

#ifndef CONFIG_PUSHBUTTON_USE_EVENT_HANDLE
  /* probe device tree for push-button */
  do {
	  struct device_node *dn, *child;
	  
	  dn = of_find_compatible_node(NULL, NULL, "gpio-keys");
	  if (dn){
		  const char *btn_name;
		  for_each_child_of_node(dn, child) {
			  //enum of_gpio_flags flags;
			  int gpio;
			  of_property_read_string(child, "label", &btn_name);
			  gpio = of_get_named_gpio(child, "gpios", 0);
			  if (btn_name && (gpio >= 0)) {				
				  if (!strcmp("btn,wifi", btn_name)) {
					  gpio_wifi = gpio;
					  // gpio_wifi_flags = flags;
				  } else if (!strcmp("btn,reset", btn_name)) {
					  gpio_reset = gpio;
					  // gpio_reset_flags = flags;
				  }
			  }
		  }
	  }
  } while (0);   
	pb_register_operations(&board_pb_op);
#endif
    
  led_register_operations(&board_01_operation);
  return 0;
}

module_init(board_init);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("RTL960XC board initialization");
