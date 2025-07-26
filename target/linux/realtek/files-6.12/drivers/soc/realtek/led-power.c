#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/netdevice.h>
#include <linux/ip.h>
#include <linux/in.h>

#include <linux/delay.h>
#include "led-generic.h"

#define MODULE_NAME "power_led_control"
#define dbg(fmt, args...) //printk(fmt, ##args)

/*
static struct led_struct power_led_config;
*/

typedef enum {
    PWR_LED_OFF, // registration not OK
    PWR_LED_ON, // registration OK
    PWR_LED_START_BLINK, // Offhook
    PWR_LED_STOP_BLINK_RESTORE, //Onhook
} _LED_STATE;

static int power_led_control_read_proc(struct seq_file *seq, void *v)
{
	return 0;
}

static int power_led_control_open(struct inode *inode, struct file *file)
{
	return single_open(file, power_led_control_read_proc, inode->i_private);
}

static ssize_t power_led_control_write_proc(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    char led_control;
	

    if (buffer && !copy_from_user(&led_control, buffer, sizeof(led_control)))
	{
		switch (led_control-'0')
		{
			case PWR_LED_OFF:
				led_off (LED_POWER_GREEN);
				break;	
			case PWR_LED_ON:
				led_on (LED_POWER_GREEN);
				break;	
//			case PWR_LED_START_BLINK:
//				led_flash_start(&power_led_config, LED_POWER_GREEN, HZ/2);
//				break;	
//			case PWR_LED_STOP_BLINK_RESTORE:
//				led_flash_stop(&power_led_config);
//				break;	
			default:
				printk("Invalid operation !");
		}
	}
	return count;
}
#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,0,0)
static const struct file_operations power_led_control_fops = {
    .owner          = THIS_MODULE,
    .open           = power_led_control_open,
    .read           = seq_read,
    .write          = power_led_control_write_proc,
    .llseek         = seq_lseek,
    .release        = single_release,
};
#else
static struct proc_ops power_led_control_fops = {
        .proc_open           = power_led_control_open,
        .proc_read           = seq_read,
        .proc_write          = power_led_control_write_proc,
        .proc_lseek         = seq_lseek,
        .proc_release        = single_release,
};
#endif
static int __init power_led_control_init(void)
{
/*
	power_led_config.led = LED_POWER_GREEN;
	power_led_config.act_state = 1; // Default is ON
	power_led_config.cycle = HZ /2;
	power_led_config.backlog = 10;
*/
//	led_flash_start(&power_led_config, LED_POWER_GREEN, HZ/2);
	proc_create_data("led_power", 0644, NULL,&power_led_control_fops, NULL);

	return 0;
}

static void __exit power_led_control_exit(void)
{
	printk(KERN_INFO MODULE_NAME": module unloaded\n");
}

/* init and cleanup functions */
module_init(power_led_control_init);
module_exit(power_led_control_exit);

/* module information */
MODULE_DESCRIPTION("Control power LED via software");
MODULE_AUTHOR("Sam Hsu <sam_hsu@realtek.com>");

