
#include "luna_mtd_nand.h"
#ifndef LUNA_MTD_PROC_BBT2_H_R8OJPWZ4
#define LUNA_MTD_PROC_BBT2_H_R8OJPWZ4

#if USE_SECOND_BBT
#define LUNA_MTD_BBT2_NAME "bbt2"
#define LUNA_MTD_BBT2_DEFAULT 1
int is_bbt2_enable(void);
const struct proc_ops * get_mtd_bbt2_proc(void);
void luna_bbt2_show(struct seq_file *m);
#endif

#endif /* end of include guard: LUNA_MTD_PROC_BBT2_H_R8OJPWZ4 */
