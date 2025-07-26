#include <linux/version.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/mm.h>
#include <linux/notifier.h>
#include <linux/panic_notifier.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/reboot.h>

extern struct proc_dir_entry *realtek_proc;
static int last_reboot_reason;

#define REASON_COLDBOOT 0
#define REASON_SOFTREBOOT 1
#define REASON_PANIC 2
#define REASON_WATCHDOG 3
#define REASON_MASK 0xf

__weak void rtk_soc_reboot_reason_set(int reason) {
}

__weak int rtk_soc_reboot_reason_get(void) {
	return -1;
}


static int __panic_handler(struct notifier_block *this,
							unsigned long event, void *unused)
{
	rtk_soc_reboot_reason_set(REASON_PANIC);
	return NOTIFY_DONE;
}

static struct notifier_block panic_notifier = {
	.notifier_call  = __panic_handler,
};

static int __reboot_handler(struct notifier_block *this,
							unsigned long event, void *unused)
{
	rtk_soc_reboot_reason_set(REASON_SOFTREBOOT);
	return NOTIFY_DONE;
}

static struct notifier_block reboot_notifier = {
	.notifier_call  = __reboot_handler,
};

static int reboot_reason_proc_read(struct seq_file *sf, void *v)
{
	seq_printf(sf, "%d\n", last_reboot_reason);
    return 0;
}

static int reboot_reason_single_open(struct inode *inode,
                                       struct file *file)
{
    return (single_open(file, reboot_reason_proc_read, NULL));
}

static int reboot_reason_proc_write(struct file *filp, const char *buf, size_t count, loff_t *offp)
{
	char tmpbuf[10] = {0};
	int reason;
	int len = (count >= sizeof(tmpbuf)) ? sizeof(tmpbuf)-1 : count;
	
	if (buf && !copy_from_user(tmpbuf, buf, len))
	{
		tmpbuf[len] = '\0';
		reason = simple_strtol(tmpbuf, NULL, 10);

		if(reason >= 0 && (reason & REASON_MASK) != REASON_MASK) {
			reason = reason & REASON_MASK;
			rtk_soc_reboot_reason_set(reason);
			return len;
		}
	}
	return -EFAULT;
}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,0,0)
static const struct file_operations fops_reboot_reason = {
	.open = reboot_reason_single_open,
	.write = reboot_reason_proc_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#else
static struct proc_ops fops_reboot_reason = {
	.proc_open           = reboot_reason_single_open,
	.proc_write          = reboot_reason_proc_write,
	.proc_read           = seq_read,
	.proc_lseek         = seq_lseek,
	.proc_release        = single_release,
};
#endif

static int __init reboot_reason_init_early(void) {
	last_reboot_reason = rtk_soc_reboot_reason_get();
	return 0;
}

static int __init reboot_reason_init(void)
{
    struct proc_dir_entry *pe;

	if (last_reboot_reason<0) {
		printk("Platform not support reboot_reason\n");
		return -EINVAL;
	}

	atomic_notifier_chain_register(&panic_notifier_list,
                                      &panic_notifier);

	register_reboot_notifier(&reboot_notifier);

    pe = proc_create_data("reboot_reason",
                          S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH,
                          realtek_proc, &fops_reboot_reason, NULL);
    if (!pe) {
        return -EINVAL;
    }

    return 0;
}
arch_initcall_sync(reboot_reason_init_early);
late_initcall(reboot_reason_init);

