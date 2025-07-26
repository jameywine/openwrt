#include <linux/version.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/netdevice.h>
#include "led-generic.h"
#include "bspchip.h"

#define MAX_COUNT 0xFFFFFF

static struct led_struct led_wifi;
static unsigned long wlan0_tx_packets=0;
static unsigned long wlan0_rx_packets=0;
#if	defined(CONFIG_USE_PCIE_SLOT_0) && defined(CONFIG_USE_PCIE_SLOT_1)
static unsigned long wlan1_tx_packets=0;
static unsigned long wlan1_rx_packets=0;
#endif

#ifndef CONFIG_ARCH_LUNA_SLAVE
static unsigned long slave_trx_stats;
#endif
static unsigned int wlan_stats=0;
static unsigned int reset_wifi_led=0;

#ifdef CONFIG_RTL8192CD
extern unsigned int get_wireless_LED_rx_cnt(struct net_device *dev);
extern unsigned int get_wireless_LED_tx_cnt(struct net_device *dev);
extern unsigned int get_wireless_LED_interval(struct net_device *dev);

static unsigned int get_slave_led_wifi_interval(unsigned int led_interval_level);
#else
unsigned int get_wireless_LED_rx_cnt(struct net_device *dev)
{
	struct rtnl_link_stats64 *dev_stats;
	struct rtnl_link_stats64 temp;
	
	dev_stats = dev_get_stats(dev, &temp);
	return (dev_stats->rx_packets);
}

unsigned int get_wireless_LED_tx_cnt(struct net_device *dev)
{
	struct rtnl_link_stats64 *dev_stats;
	struct rtnl_link_stats64 temp;
	
	dev_stats = dev_get_stats(dev, &temp);
	return (dev_stats->tx_packets);
}

unsigned int get_wireless_LED_interval(struct net_device *dev)
{
	return msecs_to_jiffies(50);
}
#endif

void led_wifi_act_start(struct led_struct *p, unsigned int cycle) {

	p->cycle = cycle;
	p->timer.expires = jiffies + p->cycle;
	mod_timer(&(p->timer), p->timer.expires);
}


static int led_wifi_device_event(struct notifier_block *unused, unsigned long event, void *ptr)
{
#ifdef CONFIG_DEFAULTS_KERNEL_3_18
	struct net_device *dev = netdev_notifier_info_to_dev(ptr);
#else
	struct net_device *dev = ptr;
#endif
	unsigned int LED_interval;
#if defined(CONFIG_DUALBAND_CONCURRENT) && !defined(CONFIG_ARCH_LUNA_SLAVE)
	unsigned int LED_interval_level;
#endif
	if(memcmp(dev->name, "wlan",4)) //skip non wireless interface
		return NOTIFY_DONE;
	if(!memcmp(dev->name, "wlan0-", 6) || !memcmp(dev->name, "wlan1-", 6)) //skip non wireless root interface
		return NOTIFY_DONE;

	switch(event){
		case NETDEV_UP:
			if(wlan_stats == 0) //timer start if one wlan device is up
			{
			#if defined(CONFIG_DUALBAND_CONCURRENT) && !defined(CONFIG_ARCH_LUNA_SLAVE) 
				if(dev->name[4]!='0') {//from slave
					LED_interval_level = (REG32(BSP_TC1DATA) >> 24) & 0x3;
					LED_interval = get_slave_led_wifi_interval(LED_interval_level);
				}
				else
			#endif
				LED_interval = get_wireless_LED_interval(dev);
				led_on(LED_WIFI);
				led_wifi_act_start(&led_wifi, LED_interval);
			}
			if(dev->name[4]=='0')
				wlan_stats |= 1;
			else
				wlan_stats |= (1<<1);
			break;

		case NETDEV_DOWN:	
#ifdef CONFIG_ARCH_LUNA_SLAVE
			REG32(BSP_TC1DATA) = 0x2;
#else
			if(dev->name[4]=='0')
				wlan_stats &= ~1;
			else
				wlan_stats &= ~(1<<1);
			if(wlan_stats==0){ //timer stop if both wlan device is down
				led_act_stop(&led_wifi);
				led_off(LED_WIFI);
			}
#endif
			break;
		default:
			break;
	}

	return NOTIFY_DONE;
}

static struct notifier_block led_wifi_device_notifier = {
	.notifier_call = led_wifi_device_event
};

#ifdef CONFIG_ARCH_LUNA_SLAVE
static unsigned int set_led_wifi_interval(unsigned int led_interval)
{
	unsigned int led_interval_milli_seconds = led_interval*1000/HZ;
	if(led_interval_milli_seconds > 1000) //NO_BLINK_TIME
		return 0;
	else if(led_interval_milli_seconds <= 1000 && led_interval_milli_seconds > 500) 
		return 1;
	else if(led_interval_milli_seconds <= 500 && led_interval_milli_seconds > 40) //LED_INTERVAL_TIME
		return 2;
	else //LED_ON_TIME
		return 3;
}
static unsigned int get_slave_led_wifi_interval(unsigned int led_interval_level)
{
	switch(led_interval_level)
	{
		case 0:
			return msecs_to_jiffies(1500);
		case 1:
			return msecs_to_jiffies(1000);
		case 2:
			return msecs_to_jiffies(500);
		case 3:
			return msecs_to_jiffies(40);
		default:
			return msecs_to_jiffies(1000);
	}
}
#endif

static void led_wifi_act_timer_func(struct timer_list * dummy) {
	struct led_struct *p = &led_wifi;
	struct net_device *dev = dev_get_by_name(&init_net, "wlan0");
	unsigned int LED_tx_cnt;
	unsigned int LED_rx_cnt;
	unsigned int LED_interval;
#if defined(CONFIG_DUALBAND_CONCURRENT) && !defined(CONFIG_ARCH_LUNA_SLAVE)
	unsigned int LED_interval_level;
#elif defined(CONFIG_USE_PCIE_SLOT_0) && defined(CONFIG_USE_PCIE_SLOT_1)
	struct net_device *dev_wlan1 = dev_get_by_name(&init_net, "wlan1");
	unsigned int wlan1_LED_tx_cnt;
	unsigned int wlan1_LED_rx_cnt;
	unsigned int wlan1_LED_interval;
#endif
	if(dev){

		LED_tx_cnt = get_wireless_LED_tx_cnt(dev);
		LED_rx_cnt = get_wireless_LED_rx_cnt(dev);
#if	defined(CONFIG_USE_PCIE_SLOT_0) && defined(CONFIG_USE_PCIE_SLOT_1)
		if(dev_wlan1){
			wlan1_LED_tx_cnt = get_wireless_LED_tx_cnt(dev_wlan1);
			wlan1_LED_rx_cnt = get_wireless_LED_rx_cnt(dev_wlan1);
		}
#endif	
		if(wlan0_tx_packets < LED_tx_cnt || wlan0_rx_packets < LED_rx_cnt)
		{

#ifdef CONFIG_ARCH_LUNA_SLAVE
			if( (REG32(BSP_TC1DATA) & MAX_COUNT) < MAX_COUNT)
				REG32(BSP_TC1DATA) += 1;
			else
				REG32(BSP_TC1DATA) &= 0x3000002;
#else
			p->state = !p->state;
			if (p->state) 
				led_on(p->led);
			else
				led_off(p->led);
#endif

		}
#if (defined(CONFIG_DUALBAND_CONCURRENT) && !defined(CONFIG_ARCH_LUNA_SLAVE)) \
	|| (defined(CONFIG_USE_PCIE_SLOT_0) && defined(CONFIG_USE_PCIE_SLOT_1))
#if defined(CONFIG_DUALBAND_CONCURRENT) && !defined(CONFIG_ARCH_LUNA_SLAVE)
		else if(slave_trx_stats < (REG32(BSP_TC1DATA) & MAX_COUNT)|| (slave_trx_stats == MAX_COUNT && slave_trx_stats > (REG32(BSP_TC1DATA) & MAX_COUNT))){
			p->state = !p->state;
			if (p->state) 
				led_on(p->led);
			else
				led_off(p->led);

			slave_trx_stats = REG32(BSP_TC1DATA) & MAX_COUNT;
		}
#elif defined(CONFIG_USE_PCIE_SLOT_0) && defined(CONFIG_USE_PCIE_SLOT_1)
		else if(dev_wlan1 &&( wlan1_tx_packets < wlan1_LED_tx_cnt || wlan1_rx_packets < wlan1_LED_rx_cnt))
		{
			p->state = !p->state;
			if (p->state) 
				led_on(p->led);
			else
				led_off(p->led);
		}
#endif
#endif
#ifndef CONFIG_ARCH_LUNA_SLAVE
		else{
			if(!p->state){
				p->state = !p->state;
				led_on(p->led);
			}
		}
#endif

		wlan0_tx_packets = LED_tx_cnt;
		wlan0_rx_packets = LED_rx_cnt;
#if defined(CONFIG_DUALBAND_CONCURRENT) && !defined(CONFIG_ARCH_LUNA_SLAVE)
		slave_trx_stats = REG32(BSP_TC1DATA) & MAX_COUNT;
#elif defined(CONFIG_USE_PCIE_SLOT_0) && defined(CONFIG_USE_PCIE_SLOT_1)
		if(dev_wlan1){
			wlan1_tx_packets = wlan1_LED_tx_cnt;
			wlan1_rx_packets = wlan1_LED_rx_cnt;
		}
#endif

		LED_interval = get_wireless_LED_interval(dev);

#ifdef CONFIG_ARCH_LUNA_SLAVE
		REG32(BSP_TC1DATA) &= ~(3<<24);
		REG32(BSP_TC1DATA) |= (set_led_wifi_interval(LED_interval) << 24);
#endif

#if defined(CONFIG_DUALBAND_CONCURRENT) && !defined(CONFIG_ARCH_LUNA_SLAVE)
		LED_interval_level = (REG32(BSP_TC1DATA) >> 24) & 0x3;
		if( LED_interval > get_slave_led_wifi_interval(LED_interval_level))
			LED_interval =  get_slave_led_wifi_interval(LED_interval_level);
#elif defined(CONFIG_USE_PCIE_SLOT_0) && defined(CONFIG_USE_PCIE_SLOT_1)
		if(dev_wlan1){
			wlan1_LED_interval = get_wireless_LED_interval(dev_wlan1);
			if( LED_interval > wlan1_LED_interval)
				LED_interval =  wlan1_LED_interval;
		}
#endif
		p->cycle = LED_interval;
#if defined(CONFIG_USE_PCIE_SLOT_0) && defined(CONFIG_USE_PCIE_SLOT_1)
		if(dev_wlan1)
			dev_put(dev_wlan1);
#endif
		dev_put(dev);
	}

	mod_timer(&(p->timer), jiffies + p->cycle);

}

static void led_wifi_init_func(struct led_struct *p, unsigned char which){
	p->led = which;
	setup_timer(&p->timer, led_wifi_act_timer_func, 0);
}

static int led_wifi_read_proc(struct seq_file *seq, void *v)
{
	char flag;
	if(reset_wifi_led) {
		flag = '1';
	}
	else {
		flag = '0';
	}
	seq_printf(seq,"%c\n", flag);
	
	return 0;
}

static int led_wifi_write_proc(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
	char flag;
	struct net_device *dev;
	unsigned int LED_interval;

	if (count < 2)
		return -EFAULT;
	if (buffer && !copy_from_user(&flag, buffer, 1)) {
		if (flag == '0'){
			reset_wifi_led = 1;
			led_act_stop(&led_wifi);
			led_off(LED_WIFI);
		}
		else if (flag == '1') {
			reset_wifi_led = 0;
			led_act_stop(&led_wifi);
			led_on(LED_WIFI);
		}
		else if (flag == '2'){ //restore
			reset_wifi_led = 0;
			dev = dev_get_by_name(&init_net, "wlan0");
			if(dev){	
				LED_interval = get_wireless_LED_interval(dev);
				led_on(LED_WIFI);
				led_wifi_act_start(&led_wifi, LED_interval);
				dev_put(dev);
			}
		}
		return count;
	}
	else
		return -EFAULT;
}

static int led_wifi_open(struct inode *inode, struct file *file)
{
        return single_open(file, led_wifi_read_proc, inode->i_private);
}
#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,0,0)
static const struct file_operations ledwifi_fops = {
        .owner          = THIS_MODULE,
        .open           = led_wifi_open,
        .read           = seq_read,
        .write          = led_wifi_write_proc,
        .llseek         = seq_lseek,
        .release        = single_release,
};
#else
static struct proc_ops ledwifi_fops = {
        .proc_open           = led_wifi_open,
        .proc_read           = seq_read,
        .proc_write          = led_wifi_write_proc,
        .proc_lseek         = seq_lseek,
        .proc_release        = single_release,
};
#endif
static void __init led_wifi_init(void)
{
	led_wifi_init_func(&led_wifi, LED_WIFI);
#if defined(CONFIG_DUALBAND_CONCURRENT) && !defined(CONFIG_ARCH_LUNA_SLAVE)
	if(REG32(BSP_TC1CTL) & (1 << 28))
		printk("%s: Warning! Timer/Counter 1 Control Register 0xb8003218 TC1En (bit 28) is enabled by others\n", __FILE__);
	REG32(BSP_TC1CTL) |= (1 << 28);  //TC1CTL 0xb8003218, TC1En
	REG32(BSP_TC1DATA) = 0x2;
	slave_trx_stats = REG32(BSP_TC1DATA) & MAX_COUNT; //bit 25:24 led_interval_level, bit 23:0 count
#endif

	register_netdevice_notifier(&led_wifi_device_notifier);
	proc_create_data("led_wifi", 0644, NULL,&ledwifi_fops, NULL);
}

static void __exit led_wifi_exit(void)
{
	remove_proc_entry("led_wifi", NULL);
	unregister_netdevice_notifier(&led_wifi_device_notifier);
}
module_init(led_wifi_init);
module_exit(led_wifi_exit);

