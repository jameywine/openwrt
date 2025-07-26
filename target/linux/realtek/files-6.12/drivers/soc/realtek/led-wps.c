#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/timer.h>
//#include <asm/processor.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include "led-generic.h"
#include "pushbutton.h"

//WPS MODE
#define GENERIC_MODEL_WPS		0
#define BIG_MODEL_WPS 			1
#define ALPHA_MODEL_WPS		2
#define E8B_MODEL_WPS			3

#define WPS_MODE				E8B_MODEL_WPS

typedef enum WPS_LED_STATE
{
	WPS_LED_INPROG_BLINK=0,
	WPS_LED_ERROR_BLINK=1,
	WPS_LED_OVERLAP_BLINK=2,
	WPS_LED_STOP_BLINK=3,
	WPS_LED_ERROR_SLOW_BLINK=4
} WPS_LED_STATE_t;

static char factory_flag='0';
static WPS_LED_STATE_t AutoCfg_LED_Toggle_Mode; //blink state
static unsigned int AutoCfg_LED_Toggle_Step = 0; //current blink step
//cathy
static unsigned char wps_wlan_restart=0;

// static int is_factory_mode(void)
// {
// 	if(factory_flag == '1')
// 		return 1;
//
// 	return 0;
// }

#if ( WPS_MODE == E8B_MODEL_WPS )
	/*every 0.1 second*/
	#define InprogBlinkTypeStep	3
	int InprogBlinkType[InprogBlinkTypeStep]={ 1,1,0 };
	#define ErrorBlinkSlowTypeStep 18
	int ErrorBlinkSlowType[ErrorBlinkSlowTypeStep]={ 1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0 };
	#define ErrorBlinkTypeStep	2
	int ErrorBlinkType[ErrorBlinkTypeStep]={ 1,0 };
	#define OverlapBlinkTypeStep	15
	int OverlapBlinkType[OverlapBlinkTypeStep]={	1,0,1,0,1,
						 	0,1,0,1,0,
						 	0,0,0,0,0 };
#else //for default
	/*every 50 or 100 ticks*/
	#define InprogBlinkTypeStep	2
	int InprogBlinkType[InprogBlinkTypeStep]={ 1,0 };
	#define ErrorBlinkTypeStep	2
	int ErrorBlinkType[ErrorBlinkTypeStep]={ 1,0 };
	#define OverlapBlinkTypeStep	2
	int OverlapBlinkType[OverlapBlinkTypeStep]={ 1,0 };
#endif //( WPS_MODE == E8B_MODEL_WPS )

static struct timer_list wps_led_timer;	

/*start led toggle by initial a timer*/
static void wps_led_timer_func(struct timer_list *timer) 
{

	//printk("AutoCfg_LED_Toggle_Mode :%d",AutoCfg_LED_Toggle_Mode);
	switch( AutoCfg_LED_Toggle_Mode )
	{
		case WPS_LED_INPROG_BLINK:
#if defined(CONFIG_ONE_WPS_LED)
			if (InprogBlinkType[AutoCfg_LED_Toggle_Step])
                                led_on(LED_WPS_GREEN);
                        else
                                led_off(LED_WPS_GREEN);
#else
			if (InprogBlinkType[AutoCfg_LED_Toggle_Step])
				led_on(LED_WPS_YELLOW);
			else
				led_off(LED_WPS_YELLOW);
#endif

			AutoCfg_LED_Toggle_Step = (AutoCfg_LED_Toggle_Step + 1) % InprogBlinkTypeStep;

			break;
                case WPS_LED_ERROR_SLOW_BLINK:
#if defined(CONFIG_ONE_WPS_LED)
                        if (ErrorBlinkSlowType[AutoCfg_LED_Toggle_Step])
                                led_on(LED_WPS_GREEN);
                        else
                                led_off(LED_WPS_GREEN);
#else
                        if (ErrorBlinkSlowType[AutoCfg_LED_Toggle_Step])
                                led_on(LED_WPS_YELLOW);
                        else
                                led_off(LED_WPS_YELLOW);
#endif
                        AutoCfg_LED_Toggle_Step = (AutoCfg_LED_Toggle_Step + 1) % ErrorBlinkSlowTypeStep;
                        break;
		case WPS_LED_ERROR_BLINK:
#if defined(CONFIG_ONE_WPS_LED)
			if (ErrorBlinkType[AutoCfg_LED_Toggle_Step])
                                led_on(LED_WPS_GREEN);
                        else
                                led_off(LED_WPS_GREEN);
#else
			if (ErrorBlinkType[AutoCfg_LED_Toggle_Step])
				led_on(LED_WPS_RED);
			else
				led_off(LED_WPS_RED);
#endif

			AutoCfg_LED_Toggle_Step = (AutoCfg_LED_Toggle_Step + 1) % ErrorBlinkTypeStep;

			break;
		case WPS_LED_OVERLAP_BLINK:
#if defined(CONFIG_ONE_WPS_LED)
			if (OverlapBlinkType[AutoCfg_LED_Toggle_Step])
                                led_on(LED_WPS_GREEN);
                        else
                                led_off(LED_WPS_GREEN);
#else
			if (OverlapBlinkType[AutoCfg_LED_Toggle_Step])
				led_on(LED_WPS_RED);
			else
				led_off(LED_WPS_RED);
#endif

			AutoCfg_LED_Toggle_Step = (AutoCfg_LED_Toggle_Step + 1) % OverlapBlinkTypeStep;		

			break;
		default:			
			break;
		}//switch( AutoCfg_LED_Toggle_Mode )
	if (AutoCfg_LED_Toggle_Mode != WPS_LED_STOP_BLINK)
		mod_timer(&wps_led_timer, jiffies + msecs_to_jiffies(100));
}

static void wps_led_start_toggle(void)
{
	wps_led_timer.expires = jiffies + msecs_to_jiffies(100);

	wps_led_timer.function = &wps_led_timer_func;
	mod_timer(&wps_led_timer, jiffies + msecs_to_jiffies(100));
}

static void wps_led_stop_toggle(void)
{
	del_timer_sync(&wps_led_timer); 
}

/* WPS autoconfig initialization */
static void autoconfig_gpio_init(void)
{	
	/* stop blinking */
	AutoCfg_LED_Toggle_Mode = WPS_LED_STOP_BLINK;
	wps_led_stop_toggle();

	led_off(LED_WPS_GREEN);
	led_off(LED_WPS_RED);
	led_off(LED_WPS_YELLOW);
}

/* WPS autoconfig off, turn off all leds */
static inline void autoconfig_gpio_off(void)
{
	autoconfig_gpio_init();
}

/* WPS autoconfig, on means green led on */
static void autoconfig_gpio_on(void)
{
	AutoCfg_LED_Toggle_Mode = WPS_LED_STOP_BLINK;
	wps_led_stop_toggle();

	led_on(LED_WPS_GREEN);
#if !defined(CONFIG_ONE_WPS_LED)
	led_off(LED_WPS_RED);
	led_off(LED_WPS_YELLOW);
#endif
}

static void autoconfig_gpio_blink(WPS_LED_STATE_t mode)
{
	wps_led_stop_toggle();

	led_off(LED_WPS_GREEN);
	led_off(LED_WPS_RED);
	led_off(LED_WPS_YELLOW);

	AutoCfg_LED_Toggle_Mode = mode;
	AutoCfg_LED_Toggle_Step = 0;
	wps_led_start_toggle();
}

/* WPS autoconfig, InProgress, green led blinking */
static inline void autoconfig_gpio_inprog_blink(void)
{
	autoconfig_gpio_blink(WPS_LED_INPROG_BLINK);
}

static inline void autoconfig_gpio_error_slow_blink(void)
{
	autoconfig_gpio_blink(WPS_LED_ERROR_SLOW_BLINK);
}
/* WPS autoconfig, Error, red led blinking */
static inline void autoconfig_gpio_error_blink(void)
{
	autoconfig_gpio_blink(WPS_LED_ERROR_BLINK);
}

/* WPS autoconfig, Overlap, red led blinking */
static inline void autoconfig_gpio_overlap_blink(void)
{
	autoconfig_gpio_blink(WPS_LED_OVERLAP_BLINK);
}
#if (defined(CONFIG_SLOT_0_8812) || defined(CONFIG_SLOT_0_8812AR_VN))  && defined(CONFIG_WLAN0_5G_WLAN1_2G)
unsigned int check_slav_on = 0;
#endif
#ifndef CONFIG_PUSHBUTTON_USE_EVENT_HANDLE
#ifdef CONFIG_WLAN_WIFIWPSONOFF_BUTTON
extern unsigned int ext_wps_flag;
#ifdef CONFIG_APOLLO_MP_TEST
extern unsigned char led_test_start;
#endif
#endif
#endif
static int gpio_read_proc(struct seq_file *seq, void *v)
{
	char flag;
#if (defined(CONFIG_SLOT_0_8812) || defined(CONFIG_SLOT_0_8812AR_VN))  && defined(CONFIG_WLAN0_5G_WLAN1_2G)
	check_slav_on = 1;
#endif
	if(wps_wlan_restart) {
		flag = '2';
	}
#ifndef CONFIG_PUSHBUTTON_USE_EVENT_HANDLE
#ifdef CONFIG_WLAN_WIFIWPSONOFF_BUTTON
#ifdef CONFIG_APOLLO_MP_TEST
    else if(ext_wps_flag == 1 && led_test_start == 0)
#else
    else if(ext_wps_flag == 1)
#endif
    {
            flag = '1';
    }
    else if(ext_wps_flag == 0)
    {
            flag = '0';
    }
#else	
	else if (pb_is_pushed(PB_WPS) == 1) {
		flag = '1';
	}
#endif  // CONFIG_WLAN_WIFIWPSONOFF_BUTTON
#endif
	else {
		flag = '0';
	}
	seq_printf(seq,"%c\n",flag);
	
	return 0;
}

static ssize_t gpio_write_proc(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
	char flag;

	if (count < 2)
		return -EFAULT;
	if (buffer && !copy_from_user(&flag, buffer, 1)) {
		if (flag == 'E')
			autoconfig_gpio_init();
		else if (flag == '0') {
			wps_wlan_restart = 0;
			autoconfig_gpio_off();
		}
		else if (flag == '1')
			autoconfig_gpio_on();
		else if (flag == '2')
			autoconfig_gpio_inprog_blink();

#if defined (CONFIG_USB_RTL8187SU_SOFTAP) || defined (CONFIG_USB_RTL8192SU_SOFTAP) || defined(CONFIG_RTL8192CD) || defined(CONFIG_RTL8192CD_MODULE)
		else if (flag == '3')
			autoconfig_gpio_error_blink();
		else if (flag == '4')
			autoconfig_gpio_overlap_blink();
#endif //CONFIG_USB_RTL8187SU_SOFTAP
		//cathy
		else if (flag == '5')
			autoconfig_gpio_error_slow_blink();
		else if (flag == 'R') 
			wps_wlan_restart = 1;
		return count;
	}
	else
		return -EFAULT;
}

static int ledwps_open(struct inode *inode, struct file *file)
{
        return single_open(file, gpio_read_proc, inode->i_private);
}

static int factory_write_proc(struct file * file, const char __user * userbuf, size_t count, loff_t * off)
{
	char flag[20] = {0};

	if (count < 2)
		return -EFAULT;

	if (userbuf && !copy_from_user(&flag, userbuf, 1)) {
		if (flag[0] == '1') {
			printk("!!Switch to factory mode.\n");
			factory_flag='1';
			//gpio_timer_interval=(HZ/10);
			//gpio_timer_interval=(HZ);
		}
		else
		{
			printk("!!Switch to user mode.\n");
			factory_flag='0';
			//gpio_timer_interval=(HZ);
		}

		return count;
	}
	else
		return -EFAULT;
}
static int factory_read_proc(struct seq_file *s, void *v)
{
	seq_printf(s, "%c\n", factory_flag);

	return 0;
}

int factory_single_open(struct inode *inode, struct file *file)
{
        return (single_open(file, factory_read_proc, NULL));
}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,0,0)
static const struct file_operations ledwps_fops = {
        .owner          = THIS_MODULE,
        .open           = ledwps_open,
        .read           = seq_read,
        .write          = gpio_write_proc,
        .llseek         = seq_lseek,
        .release        = single_release,
};

struct file_operations factory_proc_fops = {
        .owner          = THIS_MODULE,
        .open           = factory_single_open,
		.write			= factory_write_proc,
        .read           = seq_read,
        .llseek         = seq_lseek,
        .release        = single_release,
};

#else

static struct proc_ops ledwps_fops = {
        .proc_open           = ledwps_open,
        .proc_read           = seq_read,
        .proc_write          = gpio_write_proc,
        .proc_lseek         = seq_lseek,
        .proc_release        = single_release,
};

static struct proc_ops factory_proc_fops = {
        .proc_open           = factory_single_open,
        .proc_read           = seq_read,
        .proc_write          = factory_write_proc,
        .proc_lseek         = seq_lseek,
        .proc_release        = single_release,
};

#endif

static int __init led_wps_init(void) {
	timer_setup(&wps_led_timer, wps_led_timer_func, 0);
	proc_create_data("gpio", 0644, NULL,&ledwps_fops, NULL);
	proc_create_data("factory", 0644, NULL, &factory_proc_fops, NULL);
	return 0;
}

static void __exit led_wps_exit(void) {
}

void wps_led_off(void){
	autoconfig_gpio_off();
}


module_init(led_wps_init);
module_exit(led_wps_exit);


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("WPS LED BEHAVIOR");

