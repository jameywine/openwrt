//#include "bspgpio.h"
#include "led-generic.h"
#include "pushbutton.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <rtk/i2c.h>
#include <rtk/init.h>
#include <rtk/led.h>

#define BSP_PCI_MISC 0xb8000504
#define REG32(reg) (*(volatile unsigned int *)(reg))

const char board_setting_name[] = "board_setting";

void bsp_pci_phy_reset(int port) {
  u32 mrst;
  switch (port) {
  case 0:
    REG32(BSP_PCI_MISC) &= ~(1 << 15);
    REG32(BSP_PCI_MISC) |= (1 << 30);
    mrst = 24;
    break;
  case 1:
    REG32(BSP_PCI_MISC) &= ~(1 << 14);
    mrst = 21;
    break;
  default:
    return;
  }
  REG32(BSP_PCI_MISC) &= ~(1 << mrst);
  mb();
  REG32(BSP_PCI_MISC) |= (1 << mrst);
  mdelay(1);
}

static int pci_reset_pin(const char *prop_name) {
  int gpio = -EFAULT;
  struct device_node *node;
  //enum of_gpio_flags flags;
  node = of_find_node_by_name(NULL, board_setting_name);
  if (node) {
    gpio = of_get_named_gpio(node, prop_name, 0);
    // printk("%s(%d):%d,%x\n",__func__,__LINE__,gpio,flags);
  }
  return gpio;
}

void PCIE_reset_pin(int *reset) {
  *reset = pci_reset_pin("pci0_gpio_rst");
  printk("PCIE0 reset pin is set to GPIO %d\n", *reset);
  BUG_ON(*reset < 0);
}
EXPORT_SYMBOL(PCIE_reset_pin);

#ifdef CONFIG_USE_PCIE_SLOT_1
void PCIE1_reset_pin(int *reset) {
  //*reset = PCIE1_RESET_PIN;
  *reset = pci_reset_pin("pci1_gpio_rst");
  printk("PCIE1 reset pin is set to GPIO %d\n", *reset);
  BUG_ON(*reset < 0);
}
EXPORT_SYMBOL(PCIE1_reset_pin);
#endif

#ifdef CONFIG_RTL_8221B_SUPPORT
const char rtl8221b_reset_name[2][20] = {"rtl8221b_dev0_reset", "rtl8221b_dev1_reset"};

static int rtl8221b_reset_pin(const char *prop_name)
{
  int gpio = -EFAULT;
  struct device_node *node;
  enum of_gpio_flags flags;

  node = of_find_node_by_name(NULL, board_setting_name);
  if (node) {
    gpio = of_get_named_gpio_flags(node, prop_name, 0, &flags);
    printk("RTL8221B: %s pin is set to GPIO #%d\n", prop_name, gpio);
  }
  return (gpio);
}

int rtk_8221b_reset_gpio_get(unsigned int dev)
{
  if (dev > 1)
    return (-EFAULT);
  else
    return (rtl8221b_reset_pin(rtl8221b_reset_name[dev]));
}

EXPORT_SYMBOL(rtk_8221b_reset_gpio_get);
#endif

