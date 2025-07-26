/******************************************************************************
 * $Id: luan_spi_nand.c,v 1.0 2016/06/13   Exp $
 * drivers/mtd/nand/rtk_nand.c
 * Overview: Realtek NAND Flash Controller Driver
 * Copyright (c) 2016 Realtek Semiconductor Corp. All Rights Reserved.
 * Modification History:
 *    #000 2016-06-13 Yang, ChaoYuan   create file
 *    #001 2017-02-17 Yang, ChaoYuan   modify the file name to luna_mtd_nand.c and support onfi nand
 *
 *******************************************************************************/


#define DEBUG

#include <linux/device.h>
#undef DEBUG
#include <linux/mtd/mtd.h>
#include <uapi/linux/major.h>
#include <linux/kdev_t.h>
#include <linux/root_dev.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/rawnand.h>
#include <mtd/mtd-abi.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/of_device.h>
#include "naf_kernel_util.h"
#include <linux/module.h>
#include <linux/uaccess.h>
#include "../../mtdcore.h"
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include "luna_mtd_nand.h"
#include "luna_mtd_protect.h"
#include "luna_mtd_usage_count.h"
#include "luna_mtd_proc_bbt2.h"
#include "block_state_proc.h"

#ifndef CONFIG_ECC_ERR_SHOW_PAGE
#define CONFIG_ECC_ERR_SHOW_PAGE 1
#endif

#define ECC_CHUNK_LEN     (512)
#define ECC_TAG_LEN       (BCH_TAG_SIZE)
#define ECC_SYNDROME_LEN  (BCH6_SYNDROME_SIZE)
#if CONFIG_ECC_ERR_SHOW_PAGE
#define SHOW_DATA_LEN 16
static void rtk_show_page(struct mtd_info *mtd);
#endif

void
process_trace(struct task_struct *curr)
{
	struct task_struct *t = curr;
	printk("---- real parent ----\n");
	while (NULL != t) {
		printk("=> %d %s\n", t->pid, t->comm);
		if (t == t->real_parent) {
			break;
		}
		t = t->real_parent;
	}
	printk("---- parent ----\n");
	t = curr;
	while (NULL != t) {
		printk("=> %d %s\n", t->pid, t->comm);
		if (t == t->parent) {
			break;
		}
		t = t->parent;
	}
	printk("---- end ----\n");
}




int g_ecc_select = SNAF_ECC_SELECT;

static uint32_t verify_page_addr;
/* inline function */
inline static void
_set_flags(uint32_t *arr, uint32_t i) {
	uint32_t idx=i/(8*sizeof(uint32_t));
	i &= (8*sizeof(uint32_t))-1;
	arr[idx] |= 1UL << i;
}

inline static int
_get_flags(uint32_t *arr, uint32_t i) {
	uint32_t idx=i/(8*sizeof(uint32_t));
	i &= (8*sizeof(uint32_t))-1;
	return (arr[idx] & (1UL << i)) != 0;
}



static struct nand_chip *rtk_nand_chip;
static struct mtd_info *rtk_mtd;
#if defined(CONFIG_MTD_SPI_NAND_RTK)
static struct spi_nand_flash_info_s spi_nand_flash_info;
struct spi_nand_flash_info_s *_spi_nand_info = &spi_nand_flash_info;
#endif

#if defined(CONFIG_MTD_ONFI_NAND_RTK)
static struct onfi_info_s onfi_nand_flash_info;
struct onfi_info_s *_onfi_nand_info = &onfi_nand_flash_info;
#endif

static uint32_t RTK_NAND_PAGE_BUF_SIZE = (MAX_PAGE_BUF_SIZE+MAX_OOB_BUF_SIZE);
/* NOTE: Need to be changed if use a flash with more block */
#define MAX_BLOCKS (1024<<3)

static uint8_t *_ecc_buf  = NULL;
static uint8_t *_page_buf = NULL;
static uint8_t *_bbt               = NULL;
static uint32_t *_bbt_table        = NULL;
#if USE_SECOND_BBT
static uint32_t *_bbt_table2       = NULL;
#endif

static int luna_nand_ooblayout_ecc(struct mtd_info *mtd, int section,
					 struct mtd_oob_region *oobregion)
{
	if (mtd->oobsize == 64) {
		switch (section) {
			case 0:
				oobregion->offset = 24;
				oobregion->length = 40;
				return 0;
				break;
			default:
				return -ERANGE;
		}
	} else if (mtd->oobsize == 128) {
		switch (section) {
			case 0:
				oobregion->offset = 48;
				oobregion->length = 80;
				return 0;
				break;
			default:
				return -ERANGE;
		}
	}
	return -ERANGE;
}

static int luna_nand_ooblayout_free(struct mtd_info *mtd, int section,
					  struct mtd_oob_region *oobregion)
{
	if (mtd->oobsize == 64) {
		switch (section) {
			case 0:
				oobregion->offset = 2;
				oobregion->length = 20;
				return 0;
				break;
			default:
				return -ERANGE;
		}
	} else if (mtd->oobsize == 128) {
		switch (section) {
			case 0:
				oobregion->offset = 2;
				oobregion->length = 44;
				return 0;
				break;
			default:
				return -ERANGE;
		}
	}
	return -ERANGE;
}

const struct mtd_ooblayout_ops luna_nand_ooblayout_ops =
{
	.ecc = luna_nand_ooblayout_ecc,
	.free = luna_nand_ooblayout_free
};
#if 0
static struct nand_ecclayout nand_bch_oob_64 = {
	.eccbytes = 44, //ecc 40byte, + 4 bbi
	.eccpos = {
			24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
			34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
			44, 45, 46, 47, 48, 49, 50, 51, 52, 53,
			54, 55, 56, 57, 58, 59, 60, 61, 62, 63},
	.oobfree = {
		{.offset = 2,
		 .length = 20}}
};

static struct nand_ecclayout nand_bch_oob_128 = {
	.eccbytes = 44+40, //ecc 40byte, + 4 bbi + ecc 40byte
	.eccpos = {
			24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
			34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
			44, 45, 46, 47, 48, 49, 50, 51, 52, 53,
			54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
			88, 89, 90, 91, 92, 93, 94, 95, 96, 97,
			98, 99, 100, 101, 102, 103, 104, 105, 106, 107,
			108, 109, 110/*, 111, 112, 113, 114, 115, 116, 117,
			118, 119, 120, 121, 122, 123, 124, 122, 126, 127 */
			/* because that eccpos is declared in length 32 only */
	},
	.oobfree = {
		{.offset = 2,
		 .length = 20},
		{.offset = 64,
		.length = 24}}
};

#endif

#ifdef CONFIG_MTD_CMDLINE_PARTS
/* for fixed partition */
const char *ptypes[] = {"cmdlinepart", NULL};
//const char *ptypes[] = {"mtdparts=rtk_nand:640k@0(boot),6M@0x180000(linux),-(rootfs)", NULL};
#else
//eric, use cmdlinepart now
static struct mtd_partition rtl8686_parts[] = {
	{ name: "boot",		offset: 0,		size:  0xc0000, mask_flags: 0},
	{ name: "env",		offset: 0xc0000,	size: 0xe0000, mask_flags:0},
	{ name: "env2",		offset: 0xe0000,	size: 0x100000, mask_flags:0},
	{ name: "config",	offset: 0x100000,	size: 0x1900000, mask_flags:0},
	{ name: "k0",		offset: 0x1900000,	size: 0x2100000, mask_flags:0},
	{ name: "r0",		offset: 0x2100000,	size: 0x4b00000, mask_flags:0},
	{ name: "k1",		offset: 0x4b00000,	size: 0x5300000, mask_flags:0},
	{ name: "r1",		offset: 0x5300000,	size: 0x7d00000, mask_flags:0},
	{ name: "opt4",		offset: 0x7d00000,	size: 0x8000000, mask_flags:0},
	{ name: "Partition_009",offset: 0,		size: 0x1000, mask_flags:0},
	{ name: "Partition_010",offset: 0,		size: 0x1000, mask_flags:0},
	{ name: "linux",	offset: 0x1900000,	size: 0x2100000, mask_flags:0},
	{ name: "rootfs",	offset: 0x2100000,	size: 0x4b00000, mask_flags:0},
};
#endif


static inline void
rtk_chip_addr_shift(struct mtd_info *mtd, uint32_t *page_id)
{
	struct luna_nand_t *chip = (struct luna_nand_t *)mtd_to_nand(mtd);
	struct nand_chip *this = (struct nand_chip *)mtd_to_nand(mtd);
	int blk_shift = (this->phys_erase_shift - this->page_shift);
	int blk_num = (*page_id)>>blk_shift;
	*page_id = ((blk_num +(chip->selected_chip*NAND_NUM_OF_BLOCK(NAND_INFO)))<<blk_shift) + (((1<<blk_shift)-1) & (*page_id));
	return;
}

inline static int32_t
page_addr_to_bn(struct mtd_info *mtd, int32_t page_addr)
{
	struct nand_chip *this = (struct nand_chip *)mtd_to_nand(mtd);
	int blk_shift = (this->phys_erase_shift - this->page_shift);
	int32_t blk_num = (page_addr)>>blk_shift;
	return blk_num;
}

static int
rtk_pio_read (struct mtd_info *mtd, uint16_t chipnr, int page_id, int column, int len, u_char *buf)
{
	register struct luna_nand_t *chip = (struct luna_nand_t *)mtd_to_nand(mtd);

	luna_mtd_add_read(page_addr_to_bn(mtd, page_id));
	NAND_MODEL->_pio_read(NAND_INFO, buf, len, page_id, column);
	return 0;
}

// static int
// is_plr_first_load_part(struct mtd_info *mtd, uint32_t blk_pge_addr)
// {
// 	struct luna_nand_t *chip = (struct luna_nand_t *)mtd_to_nand(mtd);
// 	plr_first_load_layout_t *fl_buf=(plr_first_load_layout_t *)chip->_page_buf;
//
// 	luna_mtd_add_read(page_addr_to_bn(mtd, blk_pge_addr));
// 	NAND_MODEL->_page_read(NAND_INFO, chip->_page_buf, blk_pge_addr);
//
// 	if(ecc_engine_action(((uint32_t)NAND_INFO->_ecc_ability), fl_buf->data0, fl_buf->oob0, 0) == ECC_CTRL_ERR) return ECC_CTRL_ERR;
// 	if(ecc_engine_action(((uint32_t)NAND_INFO->_ecc_ability), fl_buf->data1, fl_buf->oob1, 0) == ECC_CTRL_ERR) return ECC_CTRL_ERR;
// 	if(PLR_FL_GET_SIGNATURE(fl_buf) == SIGNATURE_PLR_FL) return 1;
//
// 	return 0;
// }


#if USE_SECOND_BBT
void luna_bbt2_show(struct seq_file *m)
{
#define BAD_BLOCK_TABLE2 (chip->_bbt_table2)
	struct luna_nand_t *chip = (struct luna_nand_t *)mtd_to_nand(rtk_mtd);
	uint32_t bn;
	for (bn = 1; bn < NAND_NUM_OF_BLOCK(NAND_INFO); bn++) {
		if (_get_flags(BAD_BLOCK_TABLE2, bn)) {
			seq_printf(m,"BBT2 shows that %d is bad\n", bn);
		}
	}
#undef BAD_BLOCK_TABLE2
}

#define BAD_BLOCK_TABLE2 (chip->_bbt_table2)
static void
bbt2_blk_check(struct mtd_info *mtd, uint32_t bn, int chip_no)
{
	struct luna_nand_t *chip = (struct luna_nand_t *)mtd_to_nand(mtd);
	int blk_shift = (((struct nand_chip*)chip)->phys_erase_shift);
	int page_shift = (((struct nand_chip*)chip)->page_shift) ;
	uint8_t bb_tag[4];
	uint32_t cs_addr_shift = 0;
	uint32_t blk_cs_shift = 0;
#if CONFIG_MTD_NAND_MULTI_RTK
	if (1 == chip_no) {
		cs_addr_shift = 1 << PGE_BLK_SHF(NAND_INFO->_num_block);
		blk_cs_shift = NAND_NUM_OF_BLOCK(NAND_INFO);
	} else if ( 0 != chip_no) {
		return;
	}
#endif
	luna_mtd_add_read(bn);
	rtk_pio_read(mtd, 0, cs_addr_shift+((bn<<blk_shift)>>page_shift), chip->_writesize+(NAND_OOB_SIZE(NAND_INFO)-4), 4, bb_tag);
	if (0 == bb_tag[2] && bn > 4) {
		printk("BBT2 scan %d as bad\n", bn);
		nand_block_state.bad_block_num += 1;
		_set_flags(BAD_BLOCK_TABLE2, bn+blk_cs_shift);
	}
}
#undef BAD_BLOCK_TABLE2
#else
#define  bbt2_blk_check(a ,b, c) ;
#endif

#define BOOT_SECTION_BLK_NUM 6
static int
create_bbt(struct mtd_info *mtd, uint8_t *buf, struct nand_bbt_descr *bd, int chip_no)
{
	struct luna_nand_t *chip = (struct luna_nand_t *)mtd_to_nand(mtd);
	int blk_shift = (((struct nand_chip*)chip)->phys_erase_shift);
	int page_shift = (((struct nand_chip*)chip)->page_shift) ;
	uint8_t bb_tag[4];
	uint32_t *bbt_table = (uint32_t*)buf;
	uint32_t bn;
	int32_t ret;
	uint32_t cs_addr_shift = 0;
	uint32_t blk_cs_shift = chip_no*NAND_NUM_OF_BLOCK(NAND_INFO);
#if CONFIG_MTD_NAND_MULTI_RTK
	if (1 == chip_no) {
		cs_addr_shift = 1 << PGE_BLK_SHF(NAND_INFO->_num_block);
	} else if ( 0 != chip_no) {
		return -1;
	}
#endif

	static int good_blk_num = 0;
	for (bn = 1; bn < NAND_NUM_OF_BLOCK(NAND_INFO); bn++) {
		luna_mtd_add_read(bn);
		rtk_pio_read(mtd, 0, cs_addr_shift+((bn<<blk_shift)>>page_shift), chip->_writesize, 4, bb_tag);
		if (0xFF == bb_tag[0]) {
			good_blk_num++;
			if (good_blk_num >= BOOT_SECTION_BLK_NUM) {
				bbt2_blk_check(mtd, bn, chip_no);
			}
			continue;
		}
		luna_mtd_add_read(bn);
		ret = NAND_MODEL->_page_read_ecc(NAND_INFO, chip->_page_buf, cs_addr_shift+(bn<<blk_shift>>page_shift), chip->_ecc_buf);
		if ( ! IS_ECC_DECODE_FAIL(ret)
				&& 0xFF == chip->_page_buf[chip->_writesize]) {
			good_blk_num++;
			if (good_blk_num >= BOOT_SECTION_BLK_NUM) {
				bbt2_blk_check(mtd, bn, chip_no);
			}
			continue;
		}

		nand_block_state.bad_block_num += 1;
		_set_flags(bbt_table, bn+blk_cs_shift);
	}
	return 0;
}

static int
create_logical_skip_bbt(struct mtd_info *mtd, uint32_t *bbt_table)
{
	struct luna_nand_t *chip = (struct luna_nand_t *)mtd_to_nand(mtd);

	unsigned int good_num;
	int j;
	unsigned int bbi_block;
	unsigned int skip_block;
	uint32_t blk_cs_shift = 0;

	uint32_t cs_no;
	for (cs_no = 0; cs_no < chip->chip_number; ++cs_no) {

	skip_block = 1;
	good_num   = 1;
	bbi_block  = 1;
	for (;bbi_block < NAND_NUM_OF_BLOCK(NAND_INFO);bbi_block++) {
		j=_get_flags(bbt_table, bbi_block+blk_cs_shift);

		if (j==0) { //good block
			chip->_bbt[skip_block+blk_cs_shift]=bbi_block-good_num;
			good_num++;
			skip_block++;
		} else {
			printk("detect block %d(%d) is bad \n",bbi_block, bbi_block+blk_cs_shift);
		}
	}
	printk("good block number %d\n", nand_block_state.block_num - nand_block_state.bad_block_num);
	for (;skip_block< NAND_NUM_OF_BLOCK(NAND_INFO); skip_block++) {
		chip->_bbt[skip_block+blk_cs_shift]=0xff;
	}
	blk_cs_shift += NAND_NUM_OF_BLOCK(NAND_INFO);

	}
	return 0;

    //for (skip_block=0;skip_block<20;skip_block++) {
    //	printf("this->bbt[%d]= %d\n",skip_block,chip->_bbt[skip_block]);
    //}
}

static inline int
get_good_block(struct mtd_info *mtd, int *page_id)
{
	struct luna_nand_t *chip = (struct luna_nand_t *)mtd_to_nand(mtd);
	struct nand_chip *this = (struct nand_chip *)mtd_to_nand(mtd);
	int blk_shift = (this->phys_erase_shift - this->page_shift);
	int blk_num = (*page_id)>>blk_shift;
	uint32_t blk_cs_shift = 0;
#if CONFIG_MTD_NAND_MULTI_RTK
	blk_cs_shift += chip->selected_chip*NAND_NUM_OF_BLOCK(NAND_INFO);
#endif

	uint32_t skip_num = chip->_bbt[blk_cs_shift+blk_num];
	if ( skip_num == 0xFF) {
		return -1;
	}
	*page_id = ((blk_num + skip_num)<<blk_shift) + (((1<<blk_shift)-1) & (*page_id));

#if CONFIG_MTD_NAND_MULTI_RTK
	uint32_t cs_addr_shift = 0;
	if (1 == chip->selected_chip) {
		cs_addr_shift = 1 << PGE_BLK_SHF(NAND_INFO->_num_block);
	}
	*page_id += cs_addr_shift;
#endif
	return 0;
}


static inline int
chkAddrValid(struct mtd_info *mtd, const int *page_id)
{
	struct nand_chip *this = (struct nand_chip *)mtd_to_nand(mtd);
	int blk_shift = (this->phys_erase_shift - this->page_shift);
	int blk_num = (*page_id)>>blk_shift;
	if (blk_num < luna_mtd_get_blk_protection_top()) {
		printk("[" RTK_MTD_INFO "] process %i (%s) \n",  current->pid, current->comm);
		printk("[" RTK_MTD_INFO "] Operation on wrong block number %d (blocks number < %d are protected)\n", blk_num, luna_mtd_get_blk_protection_top());
		dump_stack();
		process_trace(current);
		return -1;
	}
	return 0;
}


static uint8_t
rtk_nand_read_byte(struct nand_chip *_chip)
{
	struct luna_nand_t *chip = (struct luna_nand_t *)_chip;

	if (chip->readIndex > 3) {
		return 0;
	}

	return chip->status.b[chip->readIndex++];

}


static void
rtk_nand_write_buf(struct nand_chip *_chip, const uint8_t *buf, int len)
{
	struct luna_nand_t *chip = (struct luna_nand_t*)_chip;

	if (chip->column+len > RTK_NAND_PAGE_BUF_SIZE) {
		return;
	}
	memcpy( chip->_page_buf+chip->column, buf, len);
	chip->wdata_blank_flag = 0;
	chip->column += len;
}

static void
rtk_nand_read_buf(struct nand_chip *_chip, uint8_t *buf, int len)
{
	struct luna_nand_t *chip = (struct luna_nand_t *)_chip;

	len = len < (RTK_NAND_PAGE_BUF_SIZE -  chip->column ) ? len : (RTK_NAND_PAGE_BUF_SIZE -  chip->column );
	memcpy( buf, chip->_page_buf+chip->column, len);
	chip->readIndex = chip->column + len;
	chip->column +=   len;
}

static void
rtk_nand_select_chip(struct nand_chip *_chip, int chip_num)
{
	struct luna_nand_t *chip = (struct luna_nand_t *)_chip;
	chip->selected_chip = chip_num;
	if (chip_num >= RTK_MAX_FLASH_NUMBER) {
		printk("Select not correct Flash Number %d\n", chip_num);
		BUG();
	}
}


#if  USE_SECOND_BBT
/**
 * nand_get_device - [GENERIC] Get chip for selected access
 * @chip: NAND chip structure
 *
 * Lock the device and its controller for exclusive access
 *
 * Return: -EBUSY if the chip has been suspended, 0 otherwise
 */
static int nand_get_device2(struct nand_chip *chip)
{
	mutex_lock(&chip->lock);
	if (chip->suspended) {
		mutex_unlock(&chip->lock);
		return -EBUSY;
	}
	mutex_lock(&chip->controller->lock);

	return 0;
}

/**
 * nand_get_device - [GENERIC] Get chip for selected access
 * @mtd: MTD device structure
 * @new_state: the state which is requested
 *
 * Get the device and lock it for exclusive access
 */
static int
nand_get_device(struct mtd_info *mtd, int new_state)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	return nand_get_device2(chip);
}

/**
 * nand_release_device - [GENERIC] release chip
 * @chip: NAND chip object
 *
 * Release chip lock and wake up anyone waiting on the device.
 */
static void nand_release_device2(struct nand_chip *chip)
{
	/* Release the controller and the chip */
	mutex_unlock(&chip->controller->lock);
	mutex_unlock(&chip->lock);
}

/**
 * nand_release_device - [GENERIC] release chip
 * @mtd: MTD device structure
 *
 * Release chip lock and wake up anyone waiting on the device.
 */
static void nand_release_device(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	nand_release_device2(chip);
}




/**
 * @brief
 *
 * @param mtd
 * @param blk_num
 * @param getchip 0 if flash chip number is selected
 *                1 if flash chip is not selected , block number will show the offset
 */
static int
rtk_query_bbt2(struct mtd_info *mtd, int blk_num, int getchip)
{
	struct luna_nand_t *chip = (struct luna_nand_t *)mtd_to_nand(mtd);
	uint32_t blk_cs_shift = 0;
	if (!getchip) {
		blk_cs_shift = chip->selected_chip*NAND_NUM_OF_BLOCK(NAND_INFO);
	}
	if (_get_flags(chip->_bbt_table2, blk_num+blk_cs_shift)) {
		printk("BBT2 shows that %d is bad\n", blk_num);
	}
	return _get_flags(chip->_bbt_table2, blk_num+blk_cs_shift);

}

static int
rtk_mark_bbt2(struct mtd_info *mtd, uint32_t blk_num)
{
	uint8_t status = 0;
	struct luna_nand_t *chip = (struct luna_nand_t *)mtd_to_nand(mtd);
	int blk_shift = (((struct nand_chip*)chip)->phys_erase_shift);
	int page_shift = (((struct nand_chip*)chip)->page_shift) ;
	uint32_t page_addr = (blk_num<<blk_shift)>>page_shift;
#define BAD_BLOCK_TABLE2 (chip->_bbt_table2)
	memset(chip->_page_buf, 0x00, mtd->writesize+mtd->oobsize);
	chip->_page_buf[mtd->writesize]   = 0xFF;
	chip->_page_buf[mtd->writesize+1] = 0xFF;
	luna_mtd_add_erase(page_addr_to_bn(mtd, page_addr));
	status = NAND_MODEL->_block_erase(NAND_INFO, page_addr);
	if (!status){
		luna_mtd_add_write(page_addr_to_bn(mtd, page_addr));
		status = NAND_MODEL->_page_write_ecc(NAND_INFO, chip->_page_buf, page_addr, chip->_ecc_buf);
	}
	if (!status){
		nand_block_state.bad_block_num += 1;
		_set_flags(BAD_BLOCK_TABLE2, blk_num);
	}

	return status;
#undef BAD_BLOCK_TABLE2
}

static inline int
rtk_mark_bbt2_r(struct mtd_info *mtd, int blk_num)
{
	u8_t status = 0;
	nand_get_device(mtd, FL_WRITING);
	status = rtk_mark_bbt2(mtd, blk_num);
	nand_release_device(mtd);
	return status;
}
#endif

static int
rtk_nand_block_bad(struct nand_chip *this, loff_t ofs)
{

	struct luna_nand_t *chip = (struct luna_nand_t *)this;
	unsigned int phy_block, logical_block;
	uint32_t blk_cs_shift = 0;

	logical_block = ofs>>this->phys_erase_shift;

	/*return bad block table */

#ifdef USE_BBT_SKIP
	//if use logical block ,always return good
	phy_block = logical_block + chip->_bbt[logical_block+blk_cs_shift];
	if (chip->_bbt[logical_block] >= 0xff) {
		return 1;
	} else {
#if USE_SECOND_BBT
		if (!is_bbt2_enable()) {
			return 0;
		}

		return rtk_query_bbt2(nand_to_mtd(this), phy_block, 1);
#else
		return 0;
#endif
	}
#else

	/* read bad table array, return 1 is bad , return 0 is good */
	int j = _get_flags (chip->_bbt_table, logical_block );
	return j;
#endif
}

static int
rtk_nand_block_markbad(struct nand_chip *this, loff_t ofs)
{
	struct luna_nand_t *chip = (struct luna_nand_t *)this;
	unsigned int chunk_id,block;
	unsigned int phy_block, logical_block;

#ifndef USE_BBT_SKIP
	u8 nand_buffer[2048+64];
#endif
	chunk_id = ((int) ofs) >> this->page_shift;
	(void)logical_block;
	(void)phy_block;


#ifdef USE_BBT_SKIP
#if USE_SECOND_BBT
	logical_block = ofs>>this->phys_erase_shift;
	phy_block = logical_block + chip->_bbt[logical_block];
	if (rtk_query_bbt2(nand_to_mtd(this), phy_block, 1)) {
		return 0;
	}
	return rtk_mark_bbt2(nand_to_mtd(this), phy_block);
#else
#if CONFIG_UBI_MTD_RTK_PATCH
	logical_block      = ofs>>this->phys_erase_shift;
	phy_block          = logical_block + chip->_bbt[logical_block];
	int blk_shift      = (((struct nand_chip*)chip)->phys_erase_shift);
	int page_shift     = (((struct nand_chip*)chip)->page_shift);
	uint32_t page_addr = (phy_block<<blk_shift)>>page_shift;
	luna_mtd_add_erase(page_addr_to_bn(mtd, page_addr));
	NAND_MODEL->_block_erase(NAND_INFO, page_addr);
	chip->_bbt[logical_block] = 0xFF;
	return 0;
#endif
	//if use logical block bbt, always return fail
	return 1;
#endif
	(void)block;

#else
	block = chunk_id>>(this->phys_erase_shift);

	/* set bad table array*/
	printk("Application set block %d is bad \n");
	_set_flags (chip->_bbt_table, block);
	memset (nand_buffer, 0x0, sizeof(nand_buffer));
	//pio write to chunk
	//TODO write BBI mark to flash
	//rtk_PIO_write(chunk_id,0,chunk_size+oob_size,&nand_buffer[0]);

	return 0;
#endif

}

static int
rtk_nand_waitfunc(struct nand_chip* _chip)
{
	struct luna_nand_t *chip = (struct luna_nand_t *)_chip;
	// it will returh the status
	return chip->status.b[0];
}

static void
rtk_nand_chip_reset(struct mtd_info *mtd)
{
	struct luna_nand_t *chip = (struct luna_nand_t *)mtd_to_nand(mtd);
#if CONFIG_MTD_NAND_MULTI_RTK
	uint32_t cs_no;
	for (cs_no = 0; cs_no < chip->chip_number; ++cs_no) {
		NAND_INFO->_reset(cs_no);
	}
#else
#if IS_ENABLED(CONFIG_RTK_SPI_NAND_GEN3)
	NAND_INFO->_reset(0);
#else
	NAND_INFO->_reset();
#endif

#endif
}

static inline void
addBufferMark(struct mtd_info *mtd, uint32_t value1, uint32_t value2)
{
	struct luna_nand_t *chip = (struct luna_nand_t *)mtd_to_nand(mtd);
	(*(uint32_t*)(chip->_page_buf + chip->_writesize))    = value1;
	(*(uint32_t*)(chip->_page_buf + chip->_spare_size-4)) = value2;
	mb();
}

static inline void
chkBufferMarkClr(struct mtd_info *mtd, uint32_t value1, uint32_t value2)
{
	int32_t step = 100;
	struct luna_nand_t *chip = (struct luna_nand_t *)mtd_to_nand(mtd);

	while (step-- > 0 &&
		((*(uint32_t*)(chip->_page_buf + chip->_spare_size-4)) == value2 ||
		 (*(uint32_t*)(chip->_page_buf + chip->_writesize)) == value1 )) {

		mdelay(1);
	}
	if (step <= 0 &&
		((*(uint32_t*)(chip->_page_buf + chip->_spare_size-4)) == value2 ||
		 (*(uint32_t*)(chip->_page_buf + chip->_writesize)) == value1 )) {
		printk("[" RTK_MTD_INFO "] buffer mark time out\n");
	}
}

static int
check_blank(uint32_t *end_pos, uint32_t *start_pos)
{
	do {
		if (*start_pos != 0xFFFFFFFF) {
			return 1;
		}
	} while (++start_pos != end_pos);
	return 0;
}


static void
rtk_nand_cmdfunc(struct nand_chip *_chip, unsigned command, int column, int page_addr)
{
	struct luna_nand_t *chip = (struct luna_nand_t *)_chip;

	switch (command) {
	case NAND_CMD_READ0:
		get_good_block(nand_to_mtd(_chip), &page_addr);
		addBufferMark(nand_to_mtd(_chip), 0x0, 0x12345678);
		luna_mtd_add_read(page_addr_to_bn(nand_to_mtd(_chip), page_addr));
		NAND_MODEL->_page_read(NAND_INFO, chip->_page_buf, page_addr);
		chkBufferMarkClr(nand_to_mtd(_chip), 0x0, 0x12345678);
		verify_page_addr = page_addr;
		chip->column = column;
		chip->page_addr = page_addr;


		break;
	case NAND_CMD_READOOB:
		get_good_block(nand_to_mtd(_chip), &page_addr);
		addBufferMark(nand_to_mtd(_chip), 0x0, 0x12345678);
		luna_mtd_add_read(page_addr_to_bn(nand_to_mtd(_chip), page_addr));
		NAND_MODEL->_page_read(NAND_INFO, chip->_page_buf, page_addr);
		chkBufferMarkClr(nand_to_mtd(_chip), 0x0, 0x12345678);
		column = chip->_writesize;
		chip->column = column;
		chip->page_addr = page_addr;
		break;
	case NAND_CMD_CACHEDPROG:
	case NAND_CMD_PAGEPROG:
		chip->status.v = 0;
		if (chkAddrValid(nand_to_mtd(_chip), &(chip->page_addr))) {
			chip->status.b[0] = NAND_STATUS_FAIL;
			break;
		}
		if (chip->wdata_blank_flag) {
			chip->status.v         = 0;
			chip->wdata_blank_flag = 0;
			printk(KERN_DEBUG "Data blank in %x\n", chip->page_addr);
		} else {
			luna_mtd_add_write(page_addr_to_bn(nand_to_mtd(_chip), chip->page_addr));
			chip->status.b[0] = NAND_MODEL->_page_write(NAND_INFO, chip->_page_buf, chip->page_addr);
		}
		chip->readIndex = 0;
		break;
	case NAND_CMD_SEQIN:
		get_good_block(nand_to_mtd(_chip), &page_addr);
		chip->pre_command = NAND_CMD_SEQIN;
		chip->column = column;
		chip->page_addr = page_addr;
		break;
	case NAND_CMD_STATUS:
        chip->status.v = 0;
		chip->status.b[0] = NAND_GET_STATUS();
		chip->readIndex = 0;
		break;
	case NAND_CMD_RESET:
		rtk_nand_chip_reset(nand_to_mtd(_chip));
		break;
	case NAND_CMD_READID:
		chip->status.v = 0;
		chip->status.b[0] = NAND_INFO->man_id>>8;
		if(0 == chip->status.b[0])
			chip->status.b[0] = (NAND_INFO->man_id);
		chip->status.b[1] = (NAND_INFO->dev_id)>>8;
		if(0 == chip->status.b[1])
			chip->status.b[1] = (NAND_INFO->dev_id);
		chip->status.b[2] = (NAND_INFO->dev_id);
		chip->readIndex = 0;
		break;
	case NAND_CMD_ERASE1:
		chip->readIndex = 0;
		chip->status.v = 0;
		if (get_good_block(nand_to_mtd(_chip), &page_addr) || chkAddrValid(nand_to_mtd(_chip), &page_addr)) {
			chip->status.b[0] = NAND_STATUS_FAIL;
			break;
		}
		luna_mtd_add_erase(page_addr_to_bn(nand_to_mtd(_chip), page_addr));
		chip->status.b[0] = NAND_MODEL->_block_erase(NAND_INFO, page_addr);
	case NAND_CMD_ERASE2:
		break;

		/*
		 * read error status commands require only a short delay
		 */

	case NAND_CMD_RNDOUT:
	case NAND_CMD_RNDIN:


	default:
		printk("Use NOT SUPPORTED command in NAND flash!\n");
		BUG();
		break;
		/*
		 * If we don't have access to the busy pin, we apply the given
		 * command delay
		 */

	}
	chip->pre_command = command;
	return;
}


static int
rtk_nand_scan_bbt(struct mtd_info *mtd)
{
	struct luna_nand_t *chip = (struct luna_nand_t *)mtd_to_nand(mtd);

	if (NAND_NUM_OF_BLOCK(NAND_INFO) > MAX_BLOCKS) {
		printk("Create BBT Error!, Block number > %d\n", MAX_BLOCKS);
	} else {
	nand_block_state.block_num = NAND_NUM_OF_BLOCK(NAND_INFO);
	nand_block_state.bad_block_num = 0;
#if CONFIG_MTD_NAND_MULTI_RTK
	uint32_t cs_no;
	for (cs_no = 0; cs_no < chip->chip_number; ++cs_no) {
		create_bbt(mtd, (uint8_t*)(chip->_bbt_table), NULL, cs_no);
	}

	nand_block_state.block_num = NAND_NUM_OF_BLOCK(NAND_INFO)*cs_no;
#else
		create_bbt(mtd, (uint8_t*)chip->_bbt_table, NULL, 0);
#endif
		//create bbt
#ifdef USE_BBT_SKIP
		create_logical_skip_bbt(mtd, chip->_bbt_table);
#endif
	}

	return 0;
}

static void
rtk_nand_bug(struct mtd_info *mtd)
{
	printk("Use an unsupported function in MTD NAND driver");
	BUG();
}

#if 0
static int
rtk_nand_check_blank(struct mtd_info *mtd)
{
	struct luna_nand_t *chip = (struct luna_nand_t *)mtd_to_nand(mtd);
	uint32_t *end_pos    = (uint32_t *)(chip->_page_buf+(chip->_writesize + chip->_spare_size));
	uint32_t *start_pos  = (uint32_t *)chip->_page_buf;
	uint32_t error_bit_n = 0;
	uint32_t val;
	do {
		val = 0xFFFFFFFF ^ (*start_pos);
		while (val) {
			error_bit_n += 1;
			if (error_bit_n > MAX_ALLOWED_ERR_IN_BLANK_PAGE)
				return -EBADMSG;
			val &= val - 1;
		}
	} while (++start_pos != end_pos);
	return 0;
}
#endif


static int
rtk_nand_check_blank2(struct mtd_info *mtd)
{
	struct luna_nand_t *chip = (struct luna_nand_t *)mtd_to_nand(mtd);
	uint32_t *start_pos;
	uint32_t error_bit_n;
	int32_t page_size;
	uint32_t val;
	int32_t chunk_idx;
	uint32_t *oob_start_pos;
	uint32_t *oob_end_pos;
	uint8_t *spare_data;
	uint8_t *syndrome_data;

	spare_data    = chip->_oob_poi;
	syndrome_data = chip->_oob_poi+NAND_OOB_SIZE(NAND_INFO);
	oob_end_pos   = (uint32_t*)(chip->_ecc_buf+(ECC_TAG_LEN+ECC_SYNDROME_LEN));
	start_pos     = (uint32_t *)chip->_page_buf;
	for (page_size = chip->_writesize;  page_size > 0; page_size -= ECC_CHUNK_LEN) {
		error_bit_n = 0;
		for ( chunk_idx = 0; chunk_idx < (ECC_CHUNK_LEN>>2); chunk_idx++ ) {
			val = 0xFFFFFFFF ^ (*start_pos);
			while (val) {
				error_bit_n += 1;
				if (error_bit_n > CONFIG_MAX_ALLOWED_ERR_IN_BLANK_PAGE)
					return -1;
				val &= val - 1;
			}
			start_pos++;
		}
		memcpy( chip->_ecc_buf, spare_data, ECC_TAG_LEN);
		memcpy( chip->_ecc_buf+ECC_TAG_LEN, syndrome_data, ECC_SYNDROME_LEN);
		spare_data    += ECC_TAG_LEN;
		syndrome_data += ECC_SYNDROME_LEN;

		oob_start_pos = (uint32_t*)chip->_ecc_buf;
		do {
			val = 0xFFFFFFFF ^ (*oob_start_pos);
			while (val) {
				error_bit_n += 1;
				if (error_bit_n > CONFIG_MAX_ALLOWED_ERR_IN_BLANK_PAGE)
					return -1;
				val &= val - 1;
			}
		} while (++oob_start_pos != oob_end_pos);

	}
	return 0;
}
#undef ECC_CHUNK_LEN
#undef ECC_TAG_LEN
#undef ECC_SYNDROME_LEN

#if CONFIG_ECC_ERR_SHOW_PAGE
#include <linux/ctype.h>
static void
rtk_show_page(struct mtd_info *mtd)
{
	int i;
	struct luna_nand_t *chip = (struct luna_nand_t *)mtd_to_nand(mtd);
	printk("\n");
	printk("chip->_writesize %d(%x), chip->_spare_size %d(%x)\n", chip->_writesize, chip->_writesize, chip->_spare_size, chip->_spare_size);
	for ( i = 0; i < chip->_writesize+chip->_spare_size; i+=16)
	{
		printk("%04X: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X | %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
				i,
				chip->_page_buf[i+0],
				chip->_page_buf[i+1],
				chip->_page_buf[i+2],
				chip->_page_buf[i+3],
				chip->_page_buf[i+4],
				chip->_page_buf[i+5],
				chip->_page_buf[i+6],
				chip->_page_buf[i+7],
				chip->_page_buf[i+8],
				chip->_page_buf[i+9],
				chip->_page_buf[i+10],
				chip->_page_buf[i+11],
				chip->_page_buf[i+12],
				chip->_page_buf[i+13],
				chip->_page_buf[i+14],
				chip->_page_buf[i+15],
				isprint(chip->_page_buf[i+0] )?chip->_page_buf[i+0]  : ' ',
				isprint(chip->_page_buf[i+1] )?chip->_page_buf[i+1]  : ' ',
				isprint(chip->_page_buf[i+2] )?chip->_page_buf[i+2]  : ' ',
				isprint(chip->_page_buf[i+3] )?chip->_page_buf[i+3]  : ' ',
				isprint(chip->_page_buf[i+4] )?chip->_page_buf[i+4]  : ' ',
				isprint(chip->_page_buf[i+5] )?chip->_page_buf[i+5]  : ' ',
				isprint(chip->_page_buf[i+6] )?chip->_page_buf[i+6]  : ' ',
				isprint(chip->_page_buf[i+7] )?chip->_page_buf[i+7]  : ' ',
				isprint(chip->_page_buf[i+8] )?chip->_page_buf[i+8]  : ' ',
				isprint(chip->_page_buf[i+9] )?chip->_page_buf[i+9]  : ' ',
				isprint(chip->_page_buf[i+10])?chip->_page_buf[i+10] : ' ',
				isprint(chip->_page_buf[i+11])?chip->_page_buf[i+11] : ' ',
				isprint(chip->_page_buf[i+12])?chip->_page_buf[i+12] : ' ',
				isprint(chip->_page_buf[i+13])?chip->_page_buf[i+13] : ' ',
				isprint(chip->_page_buf[i+14])?chip->_page_buf[i+14] : ' ',
				isprint(chip->_page_buf[i+15])?chip->_page_buf[i+15] : ' '
				);
	}
	printk("\n--------\n");
	printk("\n");
#if 0
	for ( i = 0; i < chip->_spare_size; i++ )
	{
		if (0 == i%SHOW_DATA_LEN){
			printk("\n%04X: ", i);
		}
		printk("%02X", chip->_oob_poi[i]);
	}
#endif
	printk("\n");
}
#undef SHOW_DATA_LEN
#endif

static int
rtk_nand_read_page( struct nand_chip *_chip,
					     uint8_t *buf, int oob_required, int page)
{
	struct luna_nand_t *chip = (struct luna_nand_t *)_chip;
	int ret;
	int blank_chk_ret;


	get_good_block(nand_to_mtd(_chip), &page);
	ret = NAND_MODEL->_page_read_ecc(NAND_INFO, chip->_page_buf, page, chip->_ecc_buf);
	if (IS_ECC_DECODE_FAIL(ret) == 1) {
		printk("[" RTK_MTD_INFO "] ECC ERROR(ret=%08X) page_addr = %04X! Do retry!\n", ret, page);
		luna_mtd_add_read(page_addr_to_bn(nand_to_mtd(_chip), page));
		ret = NAND_MODEL->_page_read_ecc(NAND_INFO, chip->_page_buf, page, chip->_ecc_buf);
	}
	memcpy( buf, chip->_page_buf, chip->_writesize) ;
	if (oob_required) {
		memcpy(_chip->oob_poi, chip->_oob_poi, chip->_spare_size);
	}

	if (IS_ECC_DECODE_FAIL(ret) == 1) {
		blank_chk_ret = rtk_nand_check_blank2(nand_to_mtd(_chip));
		if (blank_chk_ret) {
			printk("[" RTK_MTD_INFO "] ECC ERROR(ret=%08X) page_addr = %04X!\n", ret, page);
#if CONFIG_ECC_ERR_SHOW_PAGE
			rtk_show_page(nand_to_mtd(_chip));
#endif
			return -EBADMSG;
		} else {
			printk("[" RTK_MTD_INFO "] Fix bits error in blank page, page_addr = %04X\n", page);
			memset(buf, 0xFF, chip->_writesize);
			if (oob_required) {
				memset(_chip->oob_poi, 0xFF, chip->_spare_size);
			}
			return 0;
		}
	}
	return 0;
}

static int
rtk_nand_write_page( struct nand_chip *_chip,
					      const uint8_t *buf, int oob_required, int page)
{
	struct luna_nand_t *chip = (struct luna_nand_t*)_chip;
	get_good_block(nand_to_mtd(_chip), &page);
	if (chkAddrValid(nand_to_mtd(_chip), &(page))) {
		chip->status.b[0] = NAND_STATUS_FAIL;
		return -1;
	}
	memcpy(chip->_page_buf, buf, chip->_writesize) ;
	if (oob_required) {
		memcpy(chip->_oob_poi, _chip->oob_poi, chip->_spare_size);
	} else{
		memset(chip->_oob_poi, 0xFF, chip->_spare_size);
	}
	if (check_blank((uint32_t*)(chip->_oob_poi + chip->_spare_size), (uint32_t*)chip->_page_buf)) {
		NAND_MODEL->_page_write_ecc(NAND_INFO, chip->_page_buf, page, chip->_ecc_buf);
		chip->wdata_blank_flag = 0;
	} else {
		chip->wdata_blank_flag = 1;
	}
	return 0;
}

static int
rtk_nand_read_page_raw(struct nand_chip *_chip, uint8_t *buf, int oob_required,
		       int page)
{
	dump_stack();
	struct luna_nand_t *chip = (struct luna_nand_t*)_chip;
	struct mtd_info *mtd = nand_to_mtd(_chip);
	get_good_block(nand_to_mtd(_chip), &page);
	NAND_MODEL->_page_read(NAND_INFO, chip->_page_buf, page);
	memcpy(buf, chip->_page_buf, mtd->writesize);
	if (oob_required) {
		memcpy(_chip->oob_poi, chip->_oob_poi, chip->_spare_size);
	}

	return 0;
}

static int rtk_nand_write_page_raw(struct nand_chip *_chip, const uint8_t *buf,
			int oob_required, int page)
{
	dump_stack();
	struct luna_nand_t *chip = (struct luna_nand_t*)_chip;
	struct mtd_info *mtd = nand_to_mtd(_chip);
	int ret;
	get_good_block(nand_to_mtd(_chip), &page);
	if (chkAddrValid(nand_to_mtd(_chip), &(page))) {
		chip->status.b[0] = NAND_STATUS_FAIL;
		return -1;
	}
	memcpy(chip->_page_buf, buf, mtd->writesize);
	if (oob_required) {
		memcpy(chip->_oob_poi, _chip->oob_poi, chip->_spare_size);
	} else{
		memset(chip->_oob_poi, 0xFF, chip->_spare_size);
	}
	ret = NAND_MODEL->_page_write(NAND_INFO, chip->_page_buf, page);
	return ret;
}

static int
rtk_nand_read_oob(struct nand_chip *_chip, int page)
{
	dump_stack();
	struct luna_nand_t *chip = (struct luna_nand_t*)_chip;
	get_good_block(nand_to_mtd(_chip), &page);
	NAND_MODEL->_page_read(NAND_INFO, chip->_page_buf, page);
	memcpy(_chip->oob_poi, chip->_oob_poi, chip->_spare_size);
	return 0;
}

static int
rtk_nand_write_oob(struct nand_chip *_chip, int page)
{
	dump_stack();
	struct luna_nand_t *chip = (struct luna_nand_t*)_chip;
	struct mtd_info *mtd = nand_to_mtd(_chip);
	int ret;
	get_good_block(nand_to_mtd(_chip), &page);
	if (chkAddrValid(nand_to_mtd(_chip), &(page))) {
		chip->status.b[0] = NAND_STATUS_FAIL;
		return -1;
	}
	memset(chip->_page_buf, 0xFF, mtd->writesize);
	memcpy(chip->_oob_poi, _chip->oob_poi, chip->_spare_size);
	ret = NAND_MODEL->_page_write(NAND_INFO, chip->_page_buf, page);
	return ret;

}
/*************************************************************************
**  spi_nand_detect()
**	descriptions:
**	parameters:
**	return:
**  note:
*************************************************************************/
#if defined(CONFIG_MTD_SPI_NAND_RTK)

#if CONFIG_MTD_NAND_MULTI_RTK
static void
rtk_spi_nand_init_reset(struct luna_nand_t *chip)
{
	typedef int (init_rest_func_t)(void);
	extern init_rest_func_t *LS_start_of_snaf_init_rest_func[], *LS_end_of_snaf_init_rest_func;

	init_rest_func_t **init_rest_func = LS_start_of_snaf_init_rest_func;
	while (init_rest_func!=&LS_end_of_snaf_init_rest_func) {
		if(0!=(*init_rest_func)()) { chip->chip_number++; break;}
		++init_rest_func;
	}
}
#endif
extern const spi_nand_probe_t *LS_start_of_nand_spif_probe_func[];
extern const spi_nand_probe_t *LS_end_of_nand_spif_probe_func;

static int
rtk_spi_nand_detect (void)
{
	const spi_nand_probe_t **pf = LS_start_of_nand_spif_probe_func;
	void *flash_info = VZERO;


	while (pf != &LS_end_of_nand_spif_probe_func) {
		flash_info = (void *)(*pf)();
		if (flash_info != VZERO) {
			memcpy(&spi_nand_flash_info, flash_info, sizeof(struct spi_nand_flash_info_s));
			return 1;
		}
		pf++;
	}

	return 0;
}
#endif

#if defined(CONFIG_MTD_ONFI_NAND_RTK)

onfi_info_t * getOnfiInfo(void);

static int
rtk_onfi_nand_detect(void)
{
	onfi_info_t *onfi_chip = getOnfiInfo();
	if (NULL != onfi_chip) {
		memcpy(&onfi_nand_flash_info, onfi_chip, sizeof(onfi_info_t));
		return 1;
	} else {
		return 0;
	}
}
#endif

extern struct nand_flash_dev nand_flash_ids[];
#define LUNA_SPI_OPTION (NAND_NO_SUBPAGE_WRITE)
static void
rtk_nand_ids(struct mtd_info *mtd)
{
	struct luna_nand_t *chip = (struct luna_nand_t *)mtd_to_nand(mtd);
	nand_flash_ids[0].name = "\0";
	nand_flash_ids[0].mfr_id = NAND_INFO->man_id & 0xFF00?(NAND_INFO->man_id>>8)&0xFF : NAND_INFO->man_id & 0xFF;
	if (NAND_INFO->dev_id & 0xFF00) {
		nand_flash_ids[0].dev_id = ((NAND_INFO->dev_id)>>8)&0xFF;
	} else {
		nand_flash_ids[0].dev_id = (NAND_INFO->dev_id)&0xFF;
	}
	nand_flash_ids[0].pagesize = NAND_PAGE_SIZE(NAND_INFO);
	nand_flash_ids[0].chipsize = (NAND_NUM_OF_BLOCK(NAND_INFO)*NAND_NUM_OF_PAGE_PER_BLK(NAND_INFO)*NAND_PAGE_SIZE(NAND_INFO))>>20;
	nand_flash_ids[0].erasesize = NAND_NUM_OF_PAGE_PER_BLK(NAND_INFO)*NAND_PAGE_SIZE(NAND_INFO);
	nand_flash_ids[0].options = LUNA_SPI_OPTION;
	nand_flash_ids[0].oobsize = NAND_SPARE_SIZE(NAND_INFO);
	nand_flash_ids[0].ecc.strength_ds = 4;
	nand_flash_ids[0].ecc.step_ds = 512;
	// for the case that read back 8 byte full id info
	nand_flash_ids[0].id_len = 0;

}//static void rtk_nand_ids()


static void
rtk_nand_set(struct mtd_info *mtd)
{
	struct nand_chip *chip = (struct nand_chip *)mtd_to_nand(mtd);

	if (!chip->legacy.chip_delay)
		chip->legacy.chip_delay = 20;

	chip->legacy.cmdfunc = rtk_nand_cmdfunc;

	chip->legacy.waitfunc = rtk_nand_waitfunc;
	//chip->erase_cmd = rtk_nand_erase_cmd;

	chip->legacy.read_byte = rtk_nand_read_byte;
	chip->legacy.select_chip = rtk_nand_select_chip;
#ifdef USE_BBT_SKIP
	chip->legacy.block_bad = rtk_nand_block_bad;
	chip->legacy.block_markbad = rtk_nand_block_markbad;
	printk("Use RTK bb func\n");
#else
	printk("Use nandbase bb func\n");
#endif
	chip->legacy.write_buf = rtk_nand_write_buf;
	chip->legacy.read_buf = rtk_nand_read_buf;
#ifdef USE_BBT_SKIP
	chip->options |= NAND_SKIP_BBTSCAN;
	//chip->scan_bbt = rtk_nand_scan_bbt;
	printk("Use RTK bb scan\n");
#else
	printk("Use nandbase bb scan\n");
#endif

}

static void
rtk_luna_nand_set(struct mtd_info *mtd)
{
	struct luna_nand_t *chip = (struct luna_nand_t *)mtd_to_nand(mtd);
	chip->_writesize = NAND_PAGE_SIZE(NAND_INFO);
	chip->_oob_poi =  (chip->_page_buf) + (chip->_writesize);
	chip->_spare_size = NAND_SPARE_SIZE(NAND_INFO);
}

static void
rtk_luna_nand_chk(struct mtd_info *mtd)
{
	struct luna_nand_t *chip = (struct luna_nand_t *)mtd_to_nand(mtd);

	if (mtd->writesize != NAND_PAGE_SIZE(NAND_INFO)) {
		printk("[" RTK_MTD_INFO "] ERROR! Page size info not match, please check id table!\n");
		BUG();
	}

}

static int
rtk_nand_ecc_set(struct mtd_info *mtd)
{
	//struct luna_nand_t *chip = (struct luna_nand_t *)mtd_to_nand(mtd);
	struct nand_chip *_chip = mtd_to_nand(mtd);
	struct nand_ecc_ctrl *ecc = &(((struct nand_chip *)mtd_to_nand(mtd))->ecc);


	_chip->ecc.engine_type = NAND_ECC_ENGINE_TYPE_ON_HOST;
	ecc->engine_type = NAND_ECC_ENGINE_TYPE_ON_HOST;
	ecc->size = 512;
	ecc->bytes = 10;

	ecc->calculate = (void*)rtk_nand_bug;
	ecc->correct = (void*)rtk_nand_bug;

	ecc->read_page = rtk_nand_read_page;
	ecc->write_page = rtk_nand_write_page;
	ecc->read_page_raw = rtk_nand_read_page_raw;
	ecc->write_page_raw = rtk_nand_write_page_raw;
	ecc->read_oob = rtk_nand_read_oob;
	ecc->write_oob = rtk_nand_write_oob;
	ecc->read_subpage = (void*)rtk_nand_bug;
	ecc->write_subpage = (void*)rtk_nand_bug;
	ecc->strength = 6;
	return 0;
}

static int rtk_mtd_create_buffer(struct mtd_info *mtd);
static int luna_nand_attach_chip(struct nand_chip *chip)
{

	struct mtd_info *mtd = nand_to_mtd(chip);
	struct luna_nand_t *_chip = (struct luna_nand_t *)chip;
	of_dma_configure(&mtd->dev, mtd->dev.of_node, 1);
	if (!mtd->dev.dma_mask)
		mtd->dev.dma_mask = &_chip->dma_mask;
	if (!mtd->dev.coherent_dma_mask)
		mtd->dev.coherent_dma_mask = 0xffffffff;
	if (rtk_mtd_create_buffer(nand_to_mtd(chip))) {
		return -1;
	}
	return 0;
}
static const struct nand_controller_ops luna_nand_controller_ops = {
	.attach_chip = luna_nand_attach_chip,
	/*.setup_interface = luna_setup_interface,*/
	/*.exec_op = luna_nfc_exec_op,*/
};

static int rtk_nand_suspend(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	int ret = 0;

	mutex_lock(&chip->lock);
	if (chip->ops.suspend)
		ret = chip->ops.suspend(chip);
	if (!ret)
		chip->suspended = 1;

	return ret;
}

static void rtk_nand_resume(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd_to_nand(mtd);

	if (chip->suspended) {
		if (chip->ops.resume)
			chip->ops.resume(chip);
		chip->suspended = 0;
	} else {
		pr_err("%s called for a chip which is not in suspended state\n",
			__func__);
		return;
	}
	mutex_unlock(&chip->lock);
}

static void rtk_nand_shutdown(struct mtd_info *mtd)
{
	mtd->_suspend(mtd);
}

/*************************************************************************
**  rtk_mtd_nand_profile()
**	descriptions: rtk luna mtd nand driver init function
**	parameters:
**	return:
**  note:
*************************************************************************/
static int
rtk_mtd_nand_profile (void)
{
	char *ptype;
	//int pnum = 0;
	struct mtd_partition *mtd_parts;
	struct luna_nand_t *chip = (struct luna_nand_t *)mtd_to_nand(rtk_mtd);
	struct nand_chip *_chip = (struct nand_chip*)chip;


#if defined(CONFIG_MTD_SPI_NAND_RTK)
	if ((rtk_spi_nand_detect ()) == 0) {
		printk ("Warning: Lookup Table do not have this nand flash\n");
		return -1;
	}
	NAND_INFO = _spi_nand_info;
	chip->chip_number = 1;
#if CONFIG_MTD_NAND_MULTI_RTK
	rtk_spi_nand_init_reset(chip);
	if (chip->chip_number>1) {
		printk (KERN_INFO "Realtek " RTK_MTD_INFO " %d flash detected.\n", chip->chip_number);
	}
#endif
#endif

#if defined(CONFIG_MTD_ONFI_NAND_RTK)
	if ((rtk_onfi_nand_detect ()) == 0) {
		printk ("Warning: Lookup Table do not have this nand flash\n");
		return -1;
	}
	NAND_INFO = _onfi_nand_info;
	chip->chip_number = 1;
#endif


	rtk_luna_nand_set(rtk_mtd);

	rtk_nand_set(rtk_mtd);
	if (rtk_nand_ecc_set(rtk_mtd))
		return -1;

	nand_controller_init(&chip->base);
	chip->base.ops = &luna_nand_controller_ops ;
	_chip->controller = &chip->base;

	mtd_set_ooblayout(rtk_mtd, &luna_nand_ooblayout_ops);
	rtk_nand_ids(rtk_mtd);
#if CONFIG_MTD_NAND_MULTI_RTK
	if(nand_scan(rtk_mtd, chip->chip_number)){
		printk ("Warning: rtk nand scan error!\n");
		return -1;
	}
#else
	if(nand_scan(_chip, 1)){
		printk ("Warning: rtk nand scan error!\n");
		return -1;
	}
#endif
#ifdef USE_BBT_SKIP
	rtk_nand_scan_bbt(rtk_mtd);
#endif

	rtk_luna_nand_chk(rtk_mtd);

	/* check page size(write size) is 512/2048/4096.. must 512byte align */
	if (!(rtk_mtd->writesize & (0x200 - 1)))
		;//rtk_writel( rtk_mtd->oobblock >> 9, REG_PAGE_LEN);
	else {
		printk ("Error: pagesize is not 512Byte Multiple");
		return -1;
	}

	rtk_mtd->_suspend = rtk_nand_suspend;
	rtk_mtd->_resume  = rtk_nand_resume;
	rtk_mtd->_reboot  = rtk_nand_shutdown;
#ifdef CONFIG_MTD_CMDLINE_PARTS
	/* partitions from cmdline */
	ptype = (char *)ptypes[0];
	int ret = mtd_device_parse_register(rtk_mtd, ptypes, NULL, mtd_parts, 0);

	if (ret != 0) {
		printk (KERN_NOTICE "RTK: using the whole nand as a partitoin\n");
		if (add_mtd_device (rtk_mtd)) {
			printk (KERN_WARNING "RTK: Failed to register new nand device\n");
			return -EAGAIN;
		}
	} else {
		printk (KERN_NOTICE "RTK: using dynamic nand partition\n");
		struct mtd_info *boot_sec = get_mtd_device(NULL, 0);
		printk("Size of the first partition is " "%llx" "(%d blocks).\n", boot_sec->size, (uint32_t)boot_sec->size / rtk_mtd->erasesize);
		luna_mtd_set_blk_protection_top((uint32_t)boot_sec->size / rtk_mtd->erasesize);
	}
#else
	/* fixed partition ,modify rtl8686_parts table*/
	if (add_mtd_device (rtk_mtd)) {
		printk(KERN_WARNING "RTK: Failed to register new nand device\n");
		return -EAGAIN;
	}

	if (add_mtd_partitions (rtk_mtd, rtl8686_parts, ARRAY_SIZE(rtl8686_parts))) {
		printk("%s: Error, cannot add %s device\n",
				__FUNCTION__, rtk_mtd->name);
		rtk_mtd->size = 0;
		return -EAGAIN;
	}

	ROOT_DEV = MKDEV(MTD_BLOCK_MAJOR, 0);
#endif
	return 0;
}


static dma_addr_t _ecc_buf_addr;
static dma_addr_t _page_buf_addr;

static int
rtk_mtd_create_buffer(struct mtd_info *mtd)
{
	struct luna_nand_t *chip = (struct luna_nand_t *)mtd_to_nand(mtd);
	const int bbt_table_len = (sizeof(*_bbt_table))*(NUM_WORD(NAND_NUM_OF_BLOCK(NAND_INFO)));
#if CONFIG_MTD_NAND_MULTI_RTK
#define BBT_TABLE_LEN (bbt_table_len*chip->chip_number)
#else
#define BBT_TABLE_LEN (bbt_table_len)
#endif

	if (RTK_NAND_PAGE_BUF_SIZE < (NAND_PAGE_SIZE(NAND_INFO) + NAND_SPARE_SIZE(NAND_INFO))) {
		RTK_NAND_PAGE_BUF_SIZE = (NAND_PAGE_SIZE(NAND_INFO) + NAND_SPARE_SIZE(NAND_INFO));
	}

	_ecc_buf   = dma_alloc_coherent(&(mtd->dev), (sizeof(*_ecc_buf))*MAX_ECC_BUF_SIZE + 32, &_ecc_buf_addr,GFP_DMA);
	_page_buf  = dma_alloc_coherent(&(mtd->dev),  (sizeof(*_page_buf))*RTK_NAND_PAGE_BUF_SIZE + 32, &_page_buf_addr,GFP_DMA);
	_bbt       = kmalloc (NAND_NUM_OF_BLOCK(NAND_INFO)*chip->chip_number, GFP_KERNEL);
	_bbt_table = kmalloc(BBT_TABLE_LEN, GFP_KERNEL);

	chip->_ecc_buf   = (__typeof__(*_ecc_buf)*)(((uintptr_t)_ecc_buf+31) & ~ (uintptr_t)0x1F);
	chip->_page_buf  = (__typeof__(*_page_buf)*)(((uintptr_t)_page_buf+31) & ~ (uintptr_t)0x1F);
	chip->_bbt       = (uint8_t*)_bbt;
	chip->_bbt_table = (uint32_t*)_bbt_table;
	chip->_oob_poi =  (chip->_page_buf) + (chip->_writesize);


#if USE_SECOND_BBT
	_bbt_table2            = kmalloc(BBT_TABLE_LEN, GFP_KERNEL);
	chip->_bbt_table2      = _bbt_table2;
	if (!_bbt_table2 ) {
		return -ENOMEM;
	} else {
		memset(_bbt_table2, '\0', BBT_TABLE_LEN);
	}
#endif

	if (!_ecc_buf || !_page_buf || !_bbt_table || !_bbt) {
		printk ("%s: Error, no enough memory for buffer\n", __FUNCTION__);
		return -ENOMEM;
	} else {
		memset(_bbt_table, '\0', BBT_TABLE_LEN);
		//chip->bbt default all 0.
		memset(chip->_bbt, 0x0, NAND_NUM_OF_BLOCK(NAND_INFO)*chip->chip_number);
		return 0;
	}
#undef BBT_TABLE_LEN
}

static void
rtk_mtd_remove_buffer(void)
{
	if (_ecc_buf) {
		kfree(_ecc_buf);
		_ecc_buf = NULL;
	}
	if (_page_buf) {
		kfree(_page_buf);
		_page_buf = NULL;
	}
	if (_bbt_table) {
		kfree(_bbt_table);
		_bbt_table = NULL;
	}
#if USE_SECOND_BBT
	if (_bbt_table2) {
		kfree(_bbt_table2);
		_bbt_table2 = NULL;
	}
#endif
}

static void
rtk_display_version(void)
{
	const __u8 *revision;
	const __u8 *date;
	char *running = (__u8 *)VERSION;
	strsep (&running, " ");
	strsep (&running, " ");
	revision = strsep (&running, " ");
	date = strsep (&running, " ");
	printk (BANNER " Rev:%s (%s)\n", revision, date);
	printk ("[" RTK_MTD_INFO "] Blocks number under %d are protected\n", luna_mtd_get_blk_protection_top());
	printk ("[" RTK_MTD_INFO "] Allow max %d bits flip in blank page!\n", CONFIG_MAX_ALLOWED_ERR_IN_BLANK_PAGE);
}


static int
rtk_read_proc_nandinfo(struct seq_file *m, void *v)
{
	struct luna_nand_t *chip = (struct luna_nand_t *)mtd_to_nand(rtk_mtd);

	seq_printf(m, "%-15s: %u\n", "nand_size"   , (NAND_NUM_OF_BLOCK(NAND_INFO)*NAND_NUM_OF_PAGE_PER_BLK(NAND_INFO)*NAND_PAGE_SIZE(NAND_INFO)*chip->chip_number));
	seq_printf(m, "%-15s: %u\n", "chip_size"   , (NAND_NUM_OF_BLOCK(NAND_INFO)*NAND_NUM_OF_PAGE_PER_BLK(NAND_INFO)*NAND_PAGE_SIZE(NAND_INFO))>>20);
	seq_printf(m, "%-15s: %u\n" , "block_size"  , rtk_mtd->erasesize);
	seq_printf(m, "%-15s: %u\n" , "chunk_size"  , rtk_mtd->writesize);
	seq_printf(m, "%-15s: %u\n" , "oob_size"    , rtk_mtd->oobsize);
	seq_printf(m, "%-15s: %u\n" , "oob_avail"   , rtk_mtd->oobavail);
	seq_printf(m, "%-15s: %u\n" , "PagePerBlock", NAND_NUM_OF_PAGE_PER_BLK(NAND_INFO));

	return 0;
}


static int
rtk_open_proc_nandinfo(struct inode *inode, struct file *file)
{
	return single_open(file, rtk_read_proc_nandinfo, NULL);
}

static const struct proc_ops rtk_nand_proc_ops = {
	.proc_open		= rtk_open_proc_nandinfo,
	.proc_read		= seq_read,
	.proc_lseek		= seq_lseek,
	.proc_release	= single_release,
};


#define BUFFER_SIZE 100
static int
rtk_snaf_proc_w(struct file *file, const char __user *buffer,
				size_t count, loff_t *data)
{
	int flag = 0;
	char tmpbuf[BUFFER_SIZE];
	int32_t read_len = count > (BUFFER_SIZE-1) ? (BUFFER_SIZE-1) : count;
	if (buffer && !copy_from_user(tmpbuf, buffer, read_len)) {
		tmpbuf[read_len] = '\0';
		if (0 != sscanf(tmpbuf, "%d", &flag)) {
			if (flag) {
				g_ecc_select = SNAF_HARDWARE_ECC;
			} else {
				g_ecc_select = SNAF_SOFTWARE_ECC;
			}
		}
	}
	return read_len;
}
#undef BUFFER_SIZE

static int
rtk_snaf_proc_r(struct seq_file *m, void *v)
{
	seq_printf(m, "SNAF ECC select = %d (%s)\n", g_ecc_select, g_ecc_select == 1 ? "Hardware" : "Software");
	return 0;
}

static int
rtk_snaf_open_proc_hw_ecc(struct inode *inode, struct file *file){
	return single_open(file, rtk_snaf_proc_r, NULL);
}

static const struct proc_ops _snaf_proc_ops = {
	.proc_write   = rtk_snaf_proc_w,
	.proc_open    = rtk_snaf_open_proc_hw_ecc,
	.proc_read    = seq_read,
	.proc_lseek  = seq_lseek,
	.proc_release = single_release,
};


static struct proc_dir_entry *_snaf_dir = NULL;
static void
rtk_create_snaf_proc_file(void)
{
	struct proc_dir_entry *proc_file1;
	struct proc_dir_entry *proc_file_mtd_protect;
	struct proc_dir_entry *proc_file_mtd_usage_count;
#if USE_SECOND_BBT
	struct proc_dir_entry *proc_file_mtd_bbt2;
#endif
	_snaf_dir = proc_mkdir(NAND_PROC_NAME, NULL);

	proc_file1 = proc_create("hw_ecc", 0644, _snaf_dir, &_snaf_proc_ops);
	if (proc_file1 == NULL) {
		printk("[" RTK_MTD_INFO "] can't create proc: hw_ecc\r\n");
	}

	proc_file_mtd_protect = proc_create(LUNA_MTD_PROTECTED_NAME, 0644, _snaf_dir, get_mtd_protect_proc());
	if (NULL == proc_file_mtd_protect) {
		printk("[" RTK_MTD_INFO "] can't create proc:" LUNA_MTD_PROTECTED_NAME "\n");
	}

#if CONFIG_MTD_USAGE_PROC
	proc_file_mtd_usage_count = proc_create(LUNA_MTD_USAGE_COUNT_NAME, 0644, _snaf_dir, get_mtd_usage_count_proc());
#endif

#if USE_SECOND_BBT
	proc_file_mtd_bbt2 = proc_create(LUNA_MTD_BBT2_NAME, 0644, _snaf_dir, get_mtd_bbt2_proc());
#endif
}



#define MTDSIZE	(sizeof (struct mtd_info) + sizeof (struct luna_nand_t))
#define LUNA_NAND_SIZE (sizeof(struct luna_nand_t))
static int __init
rtk_mtd_nand_init (void)
{
	struct nand_chip *this = NULL;
	int rc = 0;

	rtk_display_version();

	rtk_nand_chip = this = kzalloc (LUNA_NAND_SIZE, GFP_KERNEL); // struct luna_nand_t contains struct nand_chip
	if (!this) {
		printk ("%s: Error, no enough memory for nand_chip\n", __FUNCTION__);
		rc = -ENOMEM;
		goto EXIT;
	}
	rtk_mtd = &this->base.mtd;
	if (!rtk_mtd) {
		printk ("%s: Error, no enough memory for rtk_mtd\n", __FUNCTION__);
		rc = -ENOMEM;
		goto EXIT;
	}
	rtk_mtd->name = NAND_MTD_PARTS_NAME;


	/* init nand flash */
	if (rtk_mtd_nand_profile () < 0) {
		rc = -1;
		goto EXIT;
	}
	proc_create("nandinfo", 0, NULL, &rtk_nand_proc_ops);
	rtk_create_snaf_proc_file();


EXIT:
	if (rc < 0) {
		if (rtk_mtd) {
			del_mtd_partitions (rtk_mtd);
			kfree(this);
			rtk_mtd = NULL;
		}
		rtk_mtd_remove_buffer();
		remove_proc_entry("nandinfo", NULL);
#if USE_SECOND_BBT
		remove_proc_entry(LUNA_MTD_BBT2_NAME, _snaf_dir);
#endif
#if CONFIG_MTD_USAGE_PROC
		remove_proc_entry(LUNA_MTD_USAGE_COUNT_NAME, _snaf_dir);
#endif
		remove_proc_entry(LUNA_MTD_PROTECTED_NAME, _snaf_dir);
		remove_proc_entry("hw_ecc", _snaf_dir);
		remove_proc_entry(NAND_PROC_NAME, NULL);

	} else {
		printk (KERN_INFO "Realtek " RTK_MTD_INFO " Flash Driver is successfully installing.\n");
	}
	return rc;

}

static void __exit
rtk_mtd_nand_exit (void)
{
	if (rtk_nand_chip) {
		del_mtd_partitions (rtk_mtd);
		kfree (rtk_nand_chip);
	}
	rtk_mtd_remove_buffer();
	remove_proc_entry("nandinfo", NULL);
#if USE_SECOND_BBT
	remove_proc_entry(LUNA_MTD_BBT2_NAME, _snaf_dir);
#endif
#if CONFIG_MTD_USAGE_PROC
	remove_proc_entry(LUNA_MTD_USAGE_COUNT_NAME, _snaf_dir);
#endif
	remove_proc_entry(LUNA_MTD_PROTECTED_NAME, _snaf_dir);
	remove_proc_entry("hw_ecc", _snaf_dir);
	remove_proc_entry(NAND_PROC_NAME, NULL);
}
module_init(rtk_mtd_nand_init);
module_exit(rtk_mtd_nand_exit);
MODULE_AUTHOR("CYYang<chaoyuan@realtek.com>");
MODULE_DESCRIPTION("Realtek Luna " RTK_MTD_INFO " Flash Controller Driver");

