#ifndef LUNA_MTD_USAGE_COUNT_H
#define LUNA_MTD_USAGE_COUNT_H


#ifndef CONFIG_MTD_USAGE_PROC
#define CONFIG_MTD_USAGE_PROC 1
#endif

#if !CONFIG_MTD_USAGE_PROC
#define luna_mtd_add_read(cnt)
#define luna_mtd_add_write(cnt)
#define luna_mtd_add_erase(cnt)

#else
#define LUNA_MTD_USAGE_COUNT_NAME "count"
const struct proc_ops * get_mtd_usage_count_proc(void);
void luna_mtd_add_read(int block);
void luna_mtd_add_write(int block);
void luna_mtd_add_erase(int block);
uint32_t luna_mtd_get_read_count(int block);
uint32_t luna_mtd_get_write_count(int block);
uint32_t luna_mtd_get_erase_count(int block);

#endif

#endif /* LUNA_MTD_USAGE_COUNT_H */
