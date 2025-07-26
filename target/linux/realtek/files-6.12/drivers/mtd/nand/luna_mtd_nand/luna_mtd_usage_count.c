#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#if !defined(__LUNA_KERNEL__)
#include <stdint.h>
#include <stdio.h>
#endif
#include "luna_mtd_usage_count.h"


#if CONFIG_MTD_USAGE_PROC

static int block_index_start = -1;
static int block_index_end = -1;
#define BLOCK_COUNT_MAX 2048
struct block_usage_count {
	uint32_t read_count;
	uint32_t wriet_count;
	uint32_t erase_count;
};

struct block_usage_count block_cnt_array[BLOCK_COUNT_MAX] = {0};

void
luna_mtd_add_read(int block)
{
    if (block >= 0 && block < BLOCK_COUNT_MAX) {
        block_cnt_array[block].read_count += 1;
    }
}

uint32_t
luna_mtd_get_read_count(int block)
{
    if (block >= 0 && block < BLOCK_COUNT_MAX) {
        return block_cnt_array[block].read_count;
    }
    return 0;
}

void
luna_mtd_add_write(int block)
{
    if (block >= 0 && block < BLOCK_COUNT_MAX) {
        block_cnt_array[block].wriet_count += 1;
    }
}

uint32_t
luna_mtd_get_write_count(int block)
{
    if (block >= 0 && block < BLOCK_COUNT_MAX) {
        return block_cnt_array[block].wriet_count;
    }
    return 0;
}

void
luna_mtd_add_erase(int block)
{
    if (block >= 0 && block < BLOCK_COUNT_MAX) {
        block_cnt_array[block].erase_count += 1;
    }
}

uint32_t
luna_mtd_get_erase_count(int block)
{
    if (block >= 0 && block < BLOCK_COUNT_MAX) {
        return block_cnt_array[block].erase_count;
    }
    return 1;
}

static void
listBlockUsage(struct seq_file *m, int index_start, int index_end)
{
	int i;

	if (index_start < 0 || index_start >=BLOCK_COUNT_MAX) {
		for (i = 0; i < BLOCK_COUNT_MAX; ++i) {
			if (0 == block_cnt_array[i].erase_count
                    && 0 == block_cnt_array[i].read_count
                    && 0 == block_cnt_array[i].wriet_count) {
				continue;
			}
			if (NULL == m) {
				printk(
					"block%04d: r:%-10u, w:%-10u, e:%-10u\n",
                    i,
					block_cnt_array[i].read_count,
					block_cnt_array[i].wriet_count,
					block_cnt_array[i].erase_count
				);
			} else {
				seq_printf(m,
					"block%04d: r:%-10u, w:%-10u, e:%-10u\n",
                    i,
					block_cnt_array[i].read_count,
					block_cnt_array[i].wriet_count,
					block_cnt_array[i].erase_count
				);
			}
		}
    } else if (index_end <0 || index_end <= index_start) {
        i = index_start;
        if (NULL == m) {
            printk(
                    "block%04d: r:%-10u, w:%-10u, e:%-10u\n",
                    i,
                    block_cnt_array[i].read_count,
                    block_cnt_array[i].wriet_count,
                    block_cnt_array[i].erase_count
                  );
        } else {
            seq_printf(m,
                    "block%04d: r:%-10u, w:%-10u, e:%-10u\n",
                    i,
                    block_cnt_array[i].read_count,
                    block_cnt_array[i].wriet_count,
                    block_cnt_array[i].erase_count
                    );
        }
    } else {
		for (i = index_start; i < index_end && i < BLOCK_COUNT_MAX; ++i) {
			if (NULL == m) {
				printk(
					"block%04d: r:%-10u, w:%-10u, e:%-10u\n",
                    i,
					block_cnt_array[i].read_count,
					block_cnt_array[i].wriet_count,
					block_cnt_array[i].erase_count
				);
			} else {
				seq_printf(m,
					"block%04d: r:%-10u, w:%-10u, e:%-10u\n",
                    i,
					block_cnt_array[i].read_count,
					block_cnt_array[i].wriet_count,
					block_cnt_array[i].erase_count
				);
			}
		}
    }
}

// mtd_usage_count proc
#define BUFFER_SIZE 100
static int
mtd_usage_count_proc_w(struct file *f, const char __user * buffer,
		size_t count, loff_t * ofs)
{
    int param_count;
    uint8_t tmpbuf[BUFFER_SIZE];
    size_t copy_len = count > (BUFFER_SIZE-1) ? (BUFFER_SIZE-1) : count;
    if (buffer && !copy_from_user(tmpbuf, buffer, copy_len)) {
        tmpbuf[copy_len] = '\0';
        param_count = sscanf(tmpbuf, "%d %d", &block_index_start, &block_index_end);
            if (param_count > 0) {
                if (param_count == 1) {
                    block_index_end = -1;
                }
                if (block_index_start > BLOCK_COUNT_MAX) {
                    block_index_start = -1;
                    block_index_end   = -1;
                }
                if (block_index_end <= block_index_start) {
                    block_index_end = -1;
                }
                if (block_index_start < 0) {
                    block_index_start = -1;
                }
            } else {
                block_index_start = -1;
                block_index_end   = -1;
                // do nothing
            }
    }
    return copy_len;
}
#undef BUFFER_SIZE


static int
mtd_usage_count_proc_r(struct seq_file *m, void *v)
{
    seq_printf(m, "Set info range by\n");
    seq_printf(m, "echo start_index end_index >  /proc/ .. \n");
    seq_printf(m, "set start_index -1 to show all and ignore end_index to show single!\n");
	listBlockUsage(m, block_index_start, block_index_end);
#if 0
    block_index_start = -1;
    block_index_end   = -1;
#endif
	return 0;
}

static int
mtd_usage_count_proc_open(struct inode *inode, struct file *file){
	return single_open(file, mtd_usage_count_proc_r, NULL);
}

static const struct proc_ops _mtd_usage_count_ops = {
	.proc_write   = mtd_usage_count_proc_w,
	.proc_open    = mtd_usage_count_proc_open,
	.proc_read    = seq_read,
	.proc_lseek  = seq_lseek,
	.proc_release = single_release,
};

const struct proc_ops *
get_mtd_usage_count_proc(void)
{
	return &_mtd_usage_count_ops;
}

#endif

