#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/proc_fs.h>
//#include <asm/processor.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
//#include "led-generic.h"
#include "pushbutton.h"
#include "led-usb.h"

static struct led_struct led_usb0;
static struct led_struct led_usb1;
static char g_led_usb = '1';
//TODO for G3
void usb_reset_digital_phy(unsigned char phy_port)
{
	switch (phy_port) {
		case 0:
            /* luna_pro, linux-3.18 */
			//REG32(BSP_USB_PHY_CTRL) |= BSP_USB_UTMI_RESET0;	//utmi_p0 reset
            //TODO
            printk("[%s] Not implemnted!\n", __func__);
			break;

		case 1:
			//REG32(BSP_USB_PHY_CTRL) |= BSP_USB_UTMI_RESET1;	//utmi_p1 reset
            //TODO
            printk("[%s] Not implemnted!\n", __func__);
			break;

		default:
			printk("%s: wrong phy port number:%u!\n", __func__, phy_port);
			break;
	}

	return;
}

static void usb_led_act_timer_func(unsigned long data) {
	struct led_struct *p = (struct led_struct *)data;
	
	if (p->_counter>0) {

		p->state = !p->state;
		if (p->state && g_led_usb != '0') 
			led_on(p->led);
		else
			led_off(p->led);
		
		p->_counter--;
		mod_timer(&(p->timer), jiffies + p->cycle);
	}		
	else 
	{
		if(g_led_usb =='0')
			led_off(p->led);
		else
			led_on(p->led);
	}
}


void usb_led_act_start(struct led_struct *p, unsigned int cycle, unsigned char counter) {

	p->cycle = cycle;
	p->_counter = counter;
	p->timer.expires = jiffies + p->cycle;
	mod_timer(&(p->timer), p->timer.expires);
}

void usb_act_func(unsigned char which, unsigned char LEDstate) {
	struct led_struct *p = (which==LED_USB_0)? &led_usb0: &led_usb1;


	if(g_led_usb == '0') return;

	switch (LEDstate)
	{
		case USB_LED_ALWAYS_ON:
			usb_led_act_start(p, 0, 0);
			break;
		case USB_LED_INPROG_BLINK:
			usb_led_act_start(p, HZ/4, 8);
			break;
		case USB_LED_STOP_BLINK:
			led_act_stop(p);
			led_off(p->led);
			break;
		default:
			break;
	}
		
}

/*err, this is ugly and must be fixed later. Andrew */
static int any_usb_connected(void) {
	//TODO, for G3
#if 0
	    #ifdef CONFIG_RTL9607C
        u32 ipen, ipen2, reg, val;
        ipen = REG32(BSP_IP_SEL);
        ipen2 = REG32(NEW_BSP_IP_SEL);

        /*xhci*/
        if (((ipen>>4)&0x3)==0x3) {
                reg = BSP_XHCI_BASE + 0x420;
                val = le32_to_cpu(REG32(reg));
                if (val & 1)
                        return 1;

                reg = BSP_XHCI_BASE + 0x430;
                val = le32_to_cpu(REG32(reg));
                if (val & 1)
                        return 1;
        }

        /* ehci */
        if (ipen&(1<<3)) {
                val = le32_to_cpu(REG32(BSP_OHCI_BASE+0x54));
                if (val&1)
                        return 1;
                val = le32_to_cpu(REG32(BSP_EHCI_BASE+0x54));
                if (val&1)
                        return 1;
        }

        /* ehci2 */
        if (ipen&(1<<13)) {
                val = le32_to_cpu(REG32(BSP_OHCI2_BASE+0x54));
                if (val&1)
                        return 1;
                val = le32_to_cpu(REG32(BSP_EHCI2_BASE+0x54));
                if (val&1)
                        return 1;
        }

        #endif
#endif
	return 0;
}

#if defined(CONFIG_YUEME) || defined(CONFIG_CMCC) || defined(CONFIG_CU)
static void check_usb_status(void)
{
	if (any_usb_connected())
		usb_act_func(LED_USB_0, USB_LED_ALWAYS_ON);
}

static int check_usb_device_state(void)
{
	return any_usb_connected();
}
#endif

#if defined(CONFIG_YUEME) || defined(CONFIG_CMCC) || defined(CONFIG_CU)
static int ledusb_read_proc(struct seq_file *seq, void *v)
{
        if(check_usb_device_state() == 0)
                seq_printf(seq,"USB LED turn off state\n");
        else
                seq_printf(seq,"USB LED turn on state\n");
        return 0;
}

static ssize_t ledusb_write_proc(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
        if (buffer && !copy_from_user(&g_led_usb, buffer, sizeof(g_led_usb))) {
                if(g_led_usb == '0')
                        printk("disable usb led \n");
                else
                {
                        check_usb_status();
                        printk("restore usb led \n");
                }
                return count;
        }
        return -EFAULT;
}
static int ledusb_open(struct inode *inode, struct file *file)
{
        return single_open(file, ledusb_read_proc, inode->i_private);
}
#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,0,0)
static const struct file_operations ledusb_fops = {
        .owner          = THIS_MODULE,
        .open           = ledusb_open,
        .read           = seq_read,
        .write          = ledusb_write_proc,
        .llseek         = seq_lseek,
        .release        = single_release,
};
#else
static struct proc_ops ledusb_fops = {
        .proc_open           = ledusb_open,
        .proc_read           = seq_read,
        .proc_write          = ledusb_write_proc,
        .proc_lseek         = seq_lseek,
        .proc_release        = single_release,
};
#endif
#endif // YUEME

static void usb_led_init_func(struct led_struct *p, unsigned char which){
	p->led = which;
	init_timer(&p->timer);
	p->timer.function = usb_led_act_timer_func;
	p->timer.data = (unsigned long) p;
}


static int __init led_usb_init(void) {
#ifdef CONFIG_SW_USB_LED0
	usb_led_init_func(&led_usb0, LED_USB_0);
#endif //CONFIG_SW_USB_LED0
#ifdef CONFIG_SW_USB_LED1
	usb_led_init_func(&led_usb1, LED_USB_1);
#endif	//CONFIG_SW_USB_LED1
#if defined(CONFIG_YUEME) || defined(CONFIG_CMCC) || defined(CONFIG_CU)
	proc_create_data("led_usb", 0644, NULL,&ledusb_fops, NULL);
#endif // CONFIG_YUEME


	return 0;
}

static void __exit led_usb_exit(void) {
}


module_init(led_usb_init);
module_exit(led_usb_exit);


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("GPIO driver for Reload default");

