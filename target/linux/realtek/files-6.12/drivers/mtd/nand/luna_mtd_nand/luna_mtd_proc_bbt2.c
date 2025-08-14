/*
 * =====================================================================================
 *
 *       Filename:  luna_mtd_protect.c
 *
 *    Description:
 *
 *        Version:
 *        Created:
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:
 *        Company:
 *
 * =====================================================================================
 */

#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#if !defined(__LUNA_KERNEL__)
#include <stdint.h>
#include <stdio.h>
#endif
#include "luna_mtd_proc_bbt2.h"

#if USE_SECOND_BBT

int32_t luna_mtd_bbt2_status = LUNA_MTD_BBT2_DEFAULT;

int
is_bbt2_enable(void)
{
  return luna_mtd_bbt2_status;
}

// mtd_protect proc
#define BUFFER_SIZE 100
static int
mtd_bbt2_proc_w(struct file *f, const char __user * buffer,
		size_t count, loff_t * ofs)
{
	uint8_t tmpbuf[BUFFER_SIZE];
  int32_t value;
	size_t copy_len = count > (BUFFER_SIZE-1) ? (BUFFER_SIZE-1) : count;
	if (buffer && !copy_from_user(tmpbuf, buffer, copy_len)) {
		tmpbuf[copy_len] = '\0';
		if (sscanf(tmpbuf, "%d", &value) <= 0) {
		} else {
      luna_mtd_bbt2_status = value ? 1 : 0;
			// do nothing
		}
	}
	return copy_len;
}
#undef BUFFER_SIZE

//void luna_bbt2_show(struct seq_file *m);
static int
mtd_bbt2_proc_r(struct seq_file *m, void *v)
{
	seq_printf(m, "BBT2 is %s\n", luna_mtd_bbt2_status ? "enable" : "disable");
	if (luna_mtd_bbt2_status) {
		luna_bbt2_show(m);
	}
	return 0;
}

static int
mtd_bbt2_proc_open(struct inode *inode, struct file *file){
	return single_open(file, mtd_bbt2_proc_r, NULL);
}

static const struct proc_ops _mtd_bbt2_ops = {
	.proc_write   = mtd_bbt2_proc_w,
	.proc_open    = mtd_bbt2_proc_open,
	.proc_read    = seq_read,
	.proc_lseek  = seq_lseek,
	.proc_release = single_release,
};


const struct proc_ops *
get_mtd_bbt2_proc()
{
  return &_mtd_bbt2_ops;
}

#endif

