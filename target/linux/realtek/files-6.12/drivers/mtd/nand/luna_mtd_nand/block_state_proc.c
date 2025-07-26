#include "block_state_proc.h"


#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/module.h>


struct block_state nand_block_state;

static int block_state_show(struct seq_file *seq, void *v)
{
	seq_printf(seq, "%d %d\n", nand_block_state.block_num, nand_block_state.bad_block_num);
	return 0;
}

static ssize_t block_state_write(struct file *file, const char __user *buffer,
				size_t count, loff_t *data)
{
	return count;
}

static int block_state_open(struct inode *inode, struct file *file)
{
	//return single_open(file, block_state_show, inode->i_private);
	return single_open(file, block_state_show, inode->i_private);
}

// proc 的 file_operations 結構
static const struct proc_ops block_state_proc = {
	.proc_open       = block_state_open,
	.proc_read       = seq_read,
	.proc_lseek     = seq_lseek,
	.proc_write      = block_state_write,
	.proc_release = single_release,
};

#define BLOCK_STATE_NAME "nand_state"
int __init block_state_init(void)
{
	if (!proc_create(BLOCK_STATE_NAME, 0644, NULL, &block_state_proc)) {
		printk(KERN_WARNING "create proc " BLOCK_STATE_NAME " failed!\n");
		return -1;
	}
	return 0;
}

module_init(block_state_init);



