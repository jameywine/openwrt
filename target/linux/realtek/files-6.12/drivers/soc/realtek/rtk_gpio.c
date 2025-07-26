/*
 * FILE NAME rtl_gpio.c
 *
 *
 * Copyright 2005 Realtek Semiconductor Corp.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE	LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <linux/version.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/inetdevice.h>
#include <linux/resource.h>
#include <linux/of_gpio.h>
#include <linux/of_address.h>
#include <linux/gpio/consumer.h>

#include "rtk_gpio.h"
#include "led-generic.h"
#include "pushbutton.h"

#define RTK_GPIO_DEBUG 1

#define PROBE_NULL	0
#define PROBE_ACTIVE	1
#define PROBE_RESET	2
#define PROBE_RELOAD	3

/*********************************************************
 * Device Tree Blob 
 **********************************************************/
#ifndef CONFIG_PUSHBUTTON_USE_EVENT_HANDLE
const char wps_gpio_name[]  = "wps-gpio";
const char wifi_gpio_name[] = "wifi-gpio";
const char reset_gpio_name[] = "board-reset-gpio";
#endif
#if defined(CONFIG_ARCH_RTL8198F)||defined(CONFIG_RTK_SOC_RTL8198D)
const char sys_led_name[]  = "system-led-gpio";
const char wifi_2G_led_name[] = "wifi2G-led-gpio";
const char wps_led_name[] = "wps-led-gpio";
#define REBOOT_THRESHOLD 3
#define LOAD_DEFAULT_THRESHOLD 5
#endif
#ifdef CONFIG_00R0
static struct led_struct led_internet_act;
#endif
struct rtk_gpio_data *rtk_gpio = NULL;

EXPORT_SYMBOL(rtk_gpio);

#if  !defined(CONFIG_PUSHBUTTON_USE_EVENT_HANDLE) || defined(CONFIG_ARCH_RTL8198F)||defined(CONFIG_RTK_SOC_RTL8198D)
static int rtk_gpio_probe_item(const char *item,
				      struct gpio_desc **dev_gpio)
{
    unsigned long gpio_flags;
    enum of_gpio_flags flags;
    int gpio_item = 0, ret = 0;
    struct device *dev = rtk_gpio->dev;
    struct device_node *np = dev->of_node;
    gpio_item = of_get_named_gpio_flags(np, item, 0, &flags);

	if (gpio_item == -EPROBE_DEFER) {
		printk("rtk_gpio_probe failed: %s. ret = EPROBE_DEFER.\n",
		       item);
		ret = gpio_item;
		goto fail_gpio;
	}
	if (gpio_is_valid(gpio_item)) {
    		if (flags & OF_GPIO_ACTIVE_LOW)
			gpio_flags = GPIOF_ACTIVE_LOW | GPIOF_OUT_INIT_LOW | GPIOF_EXPORT_DIR_CHANGEABLE;
		else
			gpio_flags = GPIOF_OUT_INIT_HIGH | GPIOF_EXPORT_DIR_CHANGEABLE;

		dev_info(dev, "%s %d is active %s\n", __func__, gpio_item,
			 (flags & OF_GPIO_ACTIVE_LOW) ? "low" : "high");

		ret = devm_gpio_request_one(dev, gpio_item, gpio_flags, item);
		if (ret){
			printk
			    ("rtk_gpio_probe failed: devm_gpio_request_one for %s\n",
			     item);
			goto fail_gpio;
        }
		*dev_gpio = gpio_to_desc(gpio_item);
    }else{
        ret = -1;
    }

fail_gpio:        
        return ret;
}
#endif
static void rtk_gpio_pin_mux_clr(void){
#ifdef CONFIG_CORTINA_BOARD_G3HGU_ENG
#ifndef CONFIG_MMC
	GLOBAL_PIN_MUX_t reg_v;
	void __iomem *addr;
	addr = ioremap(GLOBAL_PIN_MUX, 4);
	if (addr) {
		reg_v.wrd = readl_relaxed(addr);
		printk("GLOBAL_PIN_MUX: addr(0x%x) = 0x%x\n", addr, reg_v.wrd);
		reg_v.bf.iomux_sd_mmc_resetn = 0;
		reg_v.bf.iomux_sd_volt_reg = 0;
		writel_relaxed(reg_v.wrd, addr);
		smp_mb();
		reg_v.wrd = readl_relaxed(addr);
		printk("GLOBAL_PIN_MUX: 0x%x\n", reg_v.wrd);
		iounmap(addr);
	}else{
		printk("GLOBAL_PIN_MUX: ioremap failed!\n");
	}
#endif
#endif //only for G3HGU_ENG
}
static void rtk_gpio_setting(void)
{
    int ret;
	rtk_gpio_pin_mux_clr();
	if(rtk_gpio->dev_wps_gpio){
		ret = gpiod_direction_input(rtk_gpio->dev_wps_gpio);
		if(ret != 0){
			printk("[%s]gpiod_direction_input(wps_gpio) failed! \n",
			       __func__);
		}
	}
	if(rtk_gpio->dev_wifi_gpio){
		ret = gpiod_direction_input(rtk_gpio->dev_wifi_gpio);
		if(ret != 0){
			printk
			    ("[%s]gpiod_direction_input(wifi_gpio) failed! \n",
			     __func__);
		}
	}
	if(rtk_gpio->dev_reset_gpio){
		ret = gpiod_direction_input(rtk_gpio->dev_reset_gpio);
		if(ret != 0){
			printk
			    ("[%s]gpiod_direction_input(reset_gpio) failed! \n",
			     __func__);
		}
	}
#if defined(CONFIG_ARCH_RTL8198F)||defined(CONFIG_RTK_SOC_RTL8198D)
	if(rtk_gpio->dev_sys_led_gpio){
		ret = gpiod_direction_output(rtk_gpio->dev_sys_led_gpio, 0);
		if(ret != 0){
			printk
			    ("[%s]gpiod_direction_output(sys_led_gpio) failed! \n",
			     __func__);
		}
	}
	if(rtk_gpio->dev_wifi_2G_led_gpio){
		ret = gpiod_direction_output(rtk_gpio->dev_wifi_2G_led_gpio, 0);
		if(ret != 0){
			printk
			    ("[%s]gpiod_direction_output(wifi_2G_led_gpio) failed! \n",
			     __func__);
		}
	}
	if(rtk_gpio->dev_wps_led_gpio){
                ret = gpiod_direction_output(rtk_gpio->dev_wps_led_gpio, 0);
                if(ret != 0){
                        printk
                            ("[%s]gpiod_direction_output(wps_led_gpio) failed! \n",
                             __func__);
                }
        }

#endif
#ifdef CONFIG_RTK_RESET_WPS_SHARE_BUTTON
	rtk_gpio->dev_wps_gpio = rtk_gpio->dev_reset_gpio;
	printk("This model WPS pin shared with RESET pin\n");
#endif
}

static int rtk_gpio_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
	    
	rtk_gpio = devm_kzalloc(&pdev->dev, sizeof(*rtk_gpio), GFP_KERNEL);
	if (!rtk_gpio)
		return -ENOMEM;
    
	rtk_gpio->dev = dev;
#ifndef CONFIG_PUSHBUTTON_USE_EVENT_HANDLE
	rtk_gpio_probe_item(wps_gpio_name, &rtk_gpio->dev_wps_gpio);
	rtk_gpio_probe_item(wifi_gpio_name, &rtk_gpio->dev_wifi_gpio);
	rtk_gpio_probe_item(reset_gpio_name, &rtk_gpio->dev_reset_gpio);
#endif
#if defined(CONFIG_ARCH_RTL8198F)||defined(CONFIG_RTK_SOC_RTL8198D)
	rtk_gpio_probe_item(sys_led_name, &rtk_gpio->dev_sys_led_gpio);
	rtk_gpio_probe_item(wifi_2G_led_name, &rtk_gpio->dev_wifi_2G_led_gpio);
        rtk_gpio_probe_item(wps_led_name, &rtk_gpio->dev_wps_led_gpio);
#endif
	
#if RTK_GPIO_DEBUG
	printk("----[ rtk gpio ]-------------------------------- \n");
	if(rtk_gpio->dev_wps_gpio)
		printk("[%s - %d]wps-gpio[%d] \n", __func__, __LINE__,
		       desc_to_gpio(rtk_gpio->dev_wps_gpio));

	if(rtk_gpio->dev_wifi_gpio)
		printk("[%s - %d]wifi-gpio[%d] \n", __func__, __LINE__,
		       desc_to_gpio(rtk_gpio->dev_wifi_gpio));

	if(rtk_gpio->dev_reset_gpio)
		printk("[%s - %d]reset-gpio[%d] \n", __func__, __LINE__,
		       desc_to_gpio(rtk_gpio->dev_reset_gpio));
#if defined(CONFIG_ARCH_RTL8198F)||defined(CONFIG_RTK_SOC_RTL8198D)
	if(rtk_gpio->dev_sys_led_gpio)
		printk("[%s - %d]sys-led-gpio[%d] \n", __func__, __LINE__,
		       desc_to_gpio(rtk_gpio->dev_sys_led_gpio));

	if(rtk_gpio->dev_wifi_2G_led_gpio)
		printk("[%s - %d]wifi-2G-led--gpio[%d] \n", __func__, __LINE__,
		       desc_to_gpio(rtk_gpio->dev_wifi_2G_led_gpio));

	if(rtk_gpio->dev_wps_led_gpio)
                printk("[%s - %d]wps-led-gpio[%d] \n", __func__, __LINE__,
                       desc_to_gpio(rtk_gpio->dev_wps_led_gpio));
#endif
#endif
    rtk_gpio_setting();
	return 0;
}

static int rtk_gpio_remove(struct platform_device *pdev)
{

	return 0;
}

static const struct of_device_id rtk_gpio_dt_match[] = {
	{ .compatible = "realtek,g3hgu-gpio" },
    { .compatible = "realtek,g3lite-gpio" },
	{ },
};

MODULE_DEVICE_TABLE(of, rtk_gpio_dt_match);

static struct platform_driver rtk_gpio_driver = {
	.driver = {
		.name		= "rtk_gpio",
		.owner		= THIS_MODULE,
		.of_match_table	= rtk_gpio_dt_match,
	},
	.probe = rtk_gpio_probe,
	.remove	= rtk_gpio_remove,
};

/******************************************************************
 * porting from luna_pro(linux-3.18)
 * ***************************************************************/

#ifndef CONFIG_APOLLO_MP_TEST
static char default_flag='0';
static char default_ah_flag='0';
static char reboot_flag='0';
#else
char default_flag='0';
char default_ah_flag='0';
char reboot_flag='0';
extern unsigned char led_test_start;
#endif

#ifdef CONFIG_WLAN_ON_OFF_BUTTON
#ifndef CONFIG_APOLLO_MP_TEST
static char wlan_onoff_flag='0';
#else
char wlan_onoff_flag='0';
#endif
#endif

#ifdef CONFIG_WPS_LED
extern void wps_led_off(void);
#endif
extern void internet_dsl_led_stop(void);

unsigned char fwupgrade_led_toggle=0;

void tr068_internet_led(char state);

static char power_flag='2';
static char internet_flag='0';
static char internet_flag_state='0';
static int power_read_proc(struct seq_file *seq, void *v)
{
	int len = 0;
	seq_printf(seq,"%c\n", power_flag);
	return len;

}

static ssize_t power_write_proc(struct file *file, const char __user * buffer,
				size_t count, loff_t * ppos)
{	
	if (buffer && !copy_from_user(&power_flag, buffer, sizeof(power_flag))) {  
		switch(power_flag) {
		case '0':
			led_off(LED_POWER_RED);
			led_on(LED_POWER_GREEN);

			break;
		case '2':
			led_off(LED_POWER_GREEN);
			led_on(LED_POWER_RED);

			break;
		}
		return count;
	}
	return -EFAULT;
}

void tr068_internet_led(char state)
{
#ifdef CONFIG_APOLLO_MP_TEST
	if (led_test_start)
		return;
#endif
	
	switch(state) {
	case '0':
#ifdef CONFIG_00R0
		led_flash_stop(&led_internet_act);
#endif
		#if 1
		led_off(LED_INTERNET_GREEN);
		led_off(LED_INTERNET_RED);
		#else
		led_on(LED_INTERNET_GREEN);
                led_on(LED_INTERNET_RED);
		#endif
		//tr068_internet_led_stop();
		break;
	case '1':
#ifdef CONFIG_00R0
		led_flash_stop(&led_internet_act);
#endif
		#if 1
		led_on(LED_INTERNET_GREEN);
		led_off(LED_INTERNET_RED);
		#else
                led_off(LED_INTERNET_GREEN);
                led_on(LED_INTERNET_RED);
		#endif
		//tr068_internet_led_start();
		break;
	case '2':
#ifdef CONFIG_00R0
		led_flash_stop(&led_internet_act);
#endif
		#if 1
		led_off(LED_INTERNET_GREEN);
		led_on(LED_INTERNET_RED);
		#else
                led_on(LED_INTERNET_GREEN);
                led_off(LED_INTERNET_RED);
		#endif
		//tr068_internet_led_stop();
		break;
#ifdef CONFIG_00R0
		case '3': //blinks quickly 5HZ , booting and connection establishment 
			led_flash_start(&led_internet_act, LED_INTERNET_GREEN, HZ/5);
			break;
		case '4': //blinks slowly 1HZ, FW upgarding
			led_flash_start(&led_internet_act, LED_INTERNET_GREEN, HZ/1);
			break;
#endif

	default:
		return;
	}

	internet_flag = state;
}

static int internet_read_proc(struct seq_file *seq, void *v)
{
	int len = 0;
	seq_printf(seq,"%c\n", internet_flag);
	return len;

}

static ssize_t internet_write_proc(struct file *file,
				   const char __user * buffer, size_t count,
				   loff_t * ppos)
{	
	char input;
	if (buffer && !copy_from_user(&input, buffer, sizeof(input))) { 
		if(input == 'r') // restore
		{
			internet_flag_state = '0';
			tr068_internet_led(internet_flag);
		}
		else if(input == 'o') // off
		{
			internet_flag_state = '1';
			tr068_internet_led(0);
		}
		else
		{
			if(internet_flag_state == '0')
				tr068_internet_led(input);
			else
				internet_flag = input;
		}
		return count;
	}
	return -EFAULT;
}

//Add proc file for push button. Start.
static int default_read_proc(struct seq_file *seq, void *v)
{
	int len = 0;
#ifdef CONFIG_APOLLO_MP_TEST
	if (led_test_start)
		seq_printf(seq,"0\n");
	else
#endif
		seq_printf(seq,"%c\n", default_flag);
	return len;

}

static ssize_t default_write_proc(struct file *file, const char __user * buffer,
				  size_t count, loff_t * ppos)
{
      if (count < 2)
         return -EFAULT;
      if (buffer && !copy_from_user(&default_flag, buffer, 1)) {
         return count;
         }
      return -EFAULT;
}

static int default_ah_read_proc(struct seq_file *seq, void *v)
{
	int len = 0;
#ifdef CONFIG_APOLLO_MP_TEST
	if (led_test_start)
		seq_printf(seq,"0\n");
	else
#endif
		seq_printf(seq,"%c\n", default_ah_flag);
	return len;

}

static ssize_t default_ah_write_proc(struct file *file,
				     const char __user * buffer, size_t count,
				     loff_t * ppos)
{
      if (count < 2)
         return -EFAULT;
      if (buffer && !copy_from_user(&default_ah_flag, buffer, 1)) {
         return count;
         }
      return -EFAULT;
}

static int reboot_read_proc(struct seq_file *seq, void *v)
{
      int len = 0;
#ifdef CONFIG_APOLLO_MP_TEST
	  if (led_test_start)
		  seq_printf(seq,"0\n");
	  else
#endif
		seq_printf(seq,"%c\n", reboot_flag);
	return len;

}

static ssize_t reboot_write_proc(struct file *file, const char __user * buffer,
				 size_t count, loff_t * ppos)
{
      if (count < 2)
         return -EFAULT;
      if (buffer && !copy_from_user(&reboot_flag, buffer, 1)) {
         return count;
         }
      return -EFAULT;
}

#ifdef CONFIG_WLAN_ON_OFF_BUTTON
static int wlan_onoff_read_proc(struct seq_file *seq, void *v)
{
      int len = 0;
#ifdef CONFIG_APOLLO_MP_TEST
	if (led_test_start)
		seq_printf(seq,"0\n");
	else
#endif
		seq_printf(seq,"%c\n",wlan_onoff_flag);

	return len;

}

static ssize_t wlan_onoff_write_proc(struct file *file,
				     const char __user * buffer, size_t count,
				     loff_t * ppos)
{
      if (buffer && !copy_from_user(&wlan_onoff_flag, buffer, 1)) {
         return count;
      }
      return -EFAULT;
}
#endif

#ifndef CONFIG_PUSHBUTTON_USE_EVENT_HANDLE
// Mason Yu. Add proc file for push button.
static void pb_reset_event(int event)
{
	static int pb_state = PROBE_NULL;
	static unsigned int pb_counter = 0;

	switch(pb_state) {
	case PROBE_NULL:
		if (event) {
			pb_state = PROBE_ACTIVE;
			pb_counter++;
		}
		break;
	case PROBE_ACTIVE:
		if (event) {
#ifdef CONFIG_E8B
			if (pb_counter >= 10) {
#ifdef CONFIG_CMCC
				default_flag = '1';	//CMCC long reset to load default, not factory reset
                                printk("Set default_flag = '1'\n");
#else
				default_flag = '2';	
				printk("Set default_flag = '2'\n");
#endif

			}
#ifdef CONFIG_CMCC
			else if(pb_counter < 10) {
				printk("Push Button do nothing.\n");
			}
#endif
			else if (pb_counter >= 2){
					default_flag='3';
			}
            //For AnHui province use.
            if (pb_counter >= 20) {
				default_ah_flag = '2';	
				printk("Set default_ah_flag = '2'\n");
			} else if (pb_counter >= 5) {
				default_ah_flag='3';
			}
#endif
			pb_counter++;
		} else {
#ifdef CONFIG_E8B
#ifdef CONFIG_CMCC
			if(pb_counter < 10){
				printk("Push Button do nothing.\n");
			} else {
				default_flag = '1';     //CMCC long reset to load default, not factory reset
                                printk("Set default_flag = '1'\n");
			}
#else
			if (pb_counter < 2) {
				printk("Push Button do nothing.\n");			
			} else if(pb_counter >= 5) {
				default_flag = (pb_counter >= 10) ? '2' : '1';	
				printk("Set default_flag = '%c'\n",
				       default_flag);
				//Mason Yu,  LED flash while factory reset
			} else
				default_flag = '0';
            
            //For Anhui Province use.
			if (pb_counter < 5) {
				printk("Push Button do nothing.\n");			
			} else if (pb_counter >= 10) {
				default_ah_flag =
				    (pb_counter >= 20) ? '2' : '1';
				printk("Set default_ah_flag = '%c'\n",
				       default_ah_flag);
			} else {
				default_ah_flag = '0';
			}
            //end
#endif
#else //CONFIG_E8B
#if defined(CONFIG_ARCH_RTL8198F)||defined(CONFIG_RTK_SOC_RTL8198D)
			if (pb_counter < REBOOT_THRESHOLD) {
#ifdef CONFIG_RTK_RESET_WPS_SHARE_BUTTON
				printk("WPS pin pushed.\n");
#else
				printk("Push RESET Button do nothing.\n");
#endif
			} else if (pb_counter >= LOAD_DEFAULT_THRESHOLD) {
				//press > 5 second, load default and reboot
				default_flag = '1';
				printk("Set default_flag = '1'\n");
			} else {
				//press 3~5 second, reboot only.
				reboot_flag = '1';
				printk("Set reboot_flag = '1'\n");
			}
#else //#if defined(CONFIG_ARCH_RTL8198F)||defined(CONFIG_RTK_SOC_RTL8198D)
			// Mason Yu
#ifdef CONFIG_APOLLO_MP_TEST
			if (led_test_start) {
				if (pb_counter >= 1) {
					default_flag='1';	
					printk
					    ("led test: Set default_flag = '1'\n");
			}
			} else if (pb_counter < 2)
#else //CONFIG_APOLLO_MP_TEST
			if (pb_counter < 2)
#endif //CONFIG_APOLLO_MP_TEST
			{				
				printk("Push Button do nothing.\n");			
			} else if (pb_counter >= 5) {
				//reload default
			        default_flag='1';	
			        printk("Set default_flag = '1'\n");
				
			} else	//2<=probe_counter<5
			{				
				reboot_flag='1';
				printk("Set reboot_flag = '1'\n");				
			}
#endif //#if defined(CONFIG_ARCH_RTL8198F)||defined(CONFIG_RTK_SOC_RTL8198D)
#endif //CONFIG_E8B
			pb_state = PROBE_NULL;			
			pb_counter = 0;
		}
		break;
	}
}

#ifdef CONFIG_WLAN_ON_OFF_BUTTON
//cathy, if pressed less than 4 seconds, do on-off switch
static void pb_wlan_event(int event)
{
#ifndef CONFIG_WLAN_WIFIWPSONOFF_BUTTON
	static unsigned int pb_counter = 0;
	static int pb_state = PROBE_NULL;
	switch(pb_state) {
	case PROBE_NULL:
		if (event) {
			pb_state = PROBE_ACTIVE;
			pb_counter ++;
		}
		break;
	case PROBE_ACTIVE:
		if (event) {
			pb_counter++;
		} else {
			pb_state = PROBE_NULL;			
			if (pb_counter < 4) {
				wlan_onoff_flag='1';
				printk("wifi on-off switch\n");			
			}
			pb_counter = 0;
		}
		break;
	}
#endif
}
#endif

#ifdef CONFIG_WLAN_WIFIWPSONOFF_BUTTON
unsigned int ext_wps_flag = 0;
static void pb_wifi_wps_onoff(int event)
{
	static unsigned int pb_counter = 0;
	static int pb_state = PROBE_NULL;
	//printk("%s:%d pb_counter %d, event %d, rtk_gpio->dev_wps_gpio %d, gpiod_get_value %d, pb_state %d\n", __FUNCTION__, __LINE__, pb_counter, event, rtk_gpio->dev_wps_gpio, gpiod_get_value(rtk_gpio->dev_wps_gpio), pb_state);
	switch(pb_state)
	{
		case PROBE_NULL:
		if (event)
		{
			pb_state = PROBE_ACTIVE;
			pb_counter ++;
		}
		break;
		case PROBE_ACTIVE:
		if (event)
		{
			pb_counter++;
			if(pb_counter >= 4)
			{
				ext_wps_flag = 1;
				printk("wps button(%d) \n", pb_counter);
			}
		}
		else
		{
			pb_state = PROBE_NULL;
			if (pb_counter >= 1 && pb_counter <= 3 )
			{
				wlan_onoff_flag='1';
				printk("wifi on-off switch(%d)\n", pb_counter);
			}
				ext_wps_flag = 0;
				pb_counter = 0;
		}
		//printk("pb_counter = %d \n" , pb_counter) ;
		break;
	}
}
#endif


static void rtl_gpio_timer(unsigned long data)
{
	struct timer_list *timer = (struct timer_list *)data;
	pb_reset_event(  pb_is_pushed(PB_RESET) );	
#ifdef CONFIG_WLAN_ON_OFF_BUTTON	
	pb_wlan_event(  pb_is_pushed(PB_WIFISW) );
#endif
#ifdef CONFIG_WLAN_WIFIWPSONOFF_BUTTON
	pb_wifi_wps_onoff(pb_is_pushed(PB_WPS));
#endif

	mod_timer(timer, jiffies + HZ);
}

static struct timer_list probe_timer;
#endif

static int powerflag_open(struct inode *inode, struct file *file)
{
        return single_open(file, power_read_proc, inode->i_private);
}


#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,0,0)
static const struct file_operations powerflag_fops = {
        .owner          = THIS_MODULE,
        .open           = powerflag_open,
        .read           = seq_read,
        .write          = power_write_proc,
        .llseek         = seq_lseek,
        .release        = single_release,
};
#else
static struct proc_ops powerflag_fops={
	.proc_open = powerflag_open,
	.proc_write = power_write_proc,
	.proc_release = single_release,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
};
#endif
static int internetflag_open(struct inode *inode, struct file *file)
{
        return single_open(file, internet_read_proc, inode->i_private);
}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,0,0)
static const struct file_operations internetflag_fops = {
        .owner          = THIS_MODULE,
        .open           = internetflag_open,
        .read           = seq_read,
        .write          = internet_write_proc,
        .llseek         = seq_lseek,
        .release        = single_release,
};
#else
static struct proc_ops internetflag_fops = {
        .proc_open           = internetflag_open,
        .proc_read           = seq_read,
        .proc_write          = internet_write_proc,
        .proc_lseek         = seq_lseek,
        .proc_release        = single_release,
};
#endif

static int default_open(struct inode *inode, struct file *file)
{
        return single_open(file, default_read_proc, inode->i_private);
}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,0,0)
static const struct file_operations default_fops = {
        .owner          = THIS_MODULE,
        .open           = default_open,
        .read           = seq_read,
        .write          = default_write_proc,
        .llseek         = seq_lseek,
        .release        = single_release,
};
#else
static struct proc_ops default_fops = {
        .proc_open           = default_open,
        .proc_read           = seq_read,
        .proc_write          = default_write_proc,
        .proc_lseek         = seq_lseek,
        .proc_release        = single_release,
};
#endif
static int default_ah_open(struct inode *inode, struct file *file)
{
        return single_open(file, default_ah_read_proc, inode->i_private);
}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,0,0)
static const struct file_operations default_ah_fops = {
        .owner          = THIS_MODULE,
        .open           = default_ah_open,
        .read           = seq_read,
        .write          = default_ah_write_proc,
        .llseek         = seq_lseek,
        .release        = single_release,
};
#else
static struct proc_ops default_ah_fops = {
        .proc_open           = default_ah_open,
        .proc_read           = seq_read,
        .proc_write          = default_ah_write_proc,
        .proc_lseek         = seq_lseek,
        .proc_release        = single_release,
};
#endif
static int reboot_open(struct inode *inode, struct file *file)
{
        return single_open(file, reboot_read_proc, inode->i_private);
}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,0,0)
static const struct file_operations reboot_fops = {
        .owner          = THIS_MODULE,
        .open           = reboot_open,
        .read           = seq_read,
        .write          = reboot_write_proc,
        .llseek         = seq_lseek,
        .release        = single_release,
};
#else
static struct proc_ops reboot_fops = {
        .proc_open           = reboot_open,
        .proc_read           = seq_read,
        .proc_write          = reboot_write_proc,
        .proc_lseek         = seq_lseek,
        .proc_release        = single_release,
};
#endif
#ifdef CONFIG_WLAN_ON_OFF_BUTTON
static int wlanonoff_open(struct inode *inode, struct file *file)
{
        return single_open(file, wlan_onoff_read_proc, inode->i_private);
}
#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,0,0)
static const struct file_operations wlanonoff_fops = {
        .owner          = THIS_MODULE,
        .open           = wlanonoff_open,
        .read           = seq_read,
        .write          = wlan_onoff_write_proc,
        .llseek         = seq_lseek,
        .release        = single_release,
};
#else
static struct proc_ops wlanonoff_fops = {
        .proc_open           = wlanonoff_open,
        .proc_read           = seq_read,
        .proc_write          = wlan_onoff_write_proc,
        .proc_lseek         = seq_lseek,
        .proc_release        = single_release,
};
#endif
#endif
/******** Module init/exit **********************/
static int __init rtl_gpio_init(void)
{
	int ret = 0;

	printk("Realtek GPIO Driver for Flash Reload Default\n");
	proc_create_data("power_flag", 0644, NULL,&powerflag_fops, NULL);
	proc_create_data("internet_flag", 0644, NULL,&internetflag_fops, NULL);
	proc_create_data("load_default", 0, NULL,&default_fops, NULL);
    proc_create_data("load_ah_default", 0, NULL,&default_ah_fops, NULL);    //For Anhui Province use.
	proc_create_data("load_reboot", 0, NULL,&reboot_fops, NULL);
#ifdef CONFIG_WLAN_ON_OFF_BUTTON
	proc_create_data("wlan_onoff", 0, NULL,&wlanonoff_fops, NULL);
#endif

#ifdef CONFIG_00R0
	led_internet_act.led = LED_INTERNET_GREEN;
	led_internet_act.act_state = 1; // Default is ON
	led_internet_act.cycle = HZ /5;
	led_internet_act.backlog = 10;
#endif

#ifndef CONFIG_PUSHBUTTON_USE_EVENT_HANDLE
	init_timer (&probe_timer);
	probe_timer.expires = jiffies + HZ;
	probe_timer.data = (unsigned long)&probe_timer;
	probe_timer.function = &rtl_gpio_timer;
	mod_timer(&probe_timer, jiffies + HZ);
#endif

    platform_driver_register(&rtk_gpio_driver);
	return ret;
}

static void __exit rtl_gpio_exit(void)
{
	printk("Unload Realtek GPIO Driver \n");
    platform_driver_unregister(&rtk_gpio_driver);
}

module_init(rtl_gpio_init);
module_exit(rtl_gpio_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("GPIO driver for Reload default");
