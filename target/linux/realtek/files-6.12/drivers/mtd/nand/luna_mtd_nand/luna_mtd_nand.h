


#include <linux/mtd/rawnand.h>

#ifndef CONFIG_MTD_NAND_MULTI_RTK
#define CONFIG_MTD_NAND_MULTI_RTK 0
#endif

#ifndef CONFIG_MAX_ALLOWED_ERR_IN_BLANK_PAGE
#define CONFIG_MAX_ALLOWED_ERR_IN_BLANK_PAGE 4
#endif

#ifndef _LUNA_MTD_NAND_H
	#define _LUNA_MTD_NAND_H

#define RTK_XSTR(x) STR(x)
#define RTK_STR(x) #x

#ifndef CONFIG_UBI_MTD_RTK_PATCH
#define CONFIG_UBI_MTD_RTK_PATCH 0
#endif

#if IS_ENABLED(CONFIG_MTD_SPI_NAND_RTK)
#include "spi_nand_ctrl.h"
#include "spi_nand_common.h"
#include "spi_nand_struct.h"
#pragma message "CONFIG_MTD_SPI_NAND_RTK is " RTK_XSTR(CONFIG_MTD_SPI_NAND_RTK)
#endif


#include "ecc_struct.h"
struct luna_nand_t;

#define MAX_ALLOWED_ERR_IN_BLANK_PAGE (4)
#define USE_SECOND_BBT 1
#pragma message "USE_SECOND_BBT is " RTK_XSTR(USE_SECOND_BBT)
#define USE_BBT_SKIP 1
#define RTK_MAX_FLASH_NUMBER 2

#if IS_ENABLED(CONFIG_MTD_SPI_NAND_RTK)
#define NAND_INFO (((struct luna_nand_t*)chip)->_spi_nand_info)
#if CONFIG_MTD_NAND_MULTI_RTK || IS_ENABLED(CONFIG_RTK_SPI_NAND_GEN3)
#define NAND_GET_STATUS() (nsc_get_feature_register(((struct luna_nand_t*)chip)->selected_chip, 0xC0) | 0x80)
#else
#define NAND_GET_STATUS() (nsc_get_feature_register(0xC0) | 0x80)
#endif
#define NAND_NUM_OF_BLOCK SNAF_NUM_OF_BLOCK
#define NAND_PAGE_SIZE SNAF_PAGE_SIZE
#define NAND_NUM_OF_PAGE_PER_BLK SNAF_NUM_OF_PAGE_PER_BLK
#define NAND_SPARE_SIZE SNAF_SPARE_SIZE
#define NAND_OOB_SIZE SNAF_OOB_SIZE
#define BANNER  "Realtek Luna SPI NAND Flash Driver"
#define VERSION  "$Id: luna_mtd_nand.c,2017/02/17 00:00:00 ChaoYuan_Yang Exp $"
#define RTK_MTD_INFO "SPINAND"
#define NAND_PROC_NAME "spi_nand"
#define NAND_MTD_PARTS_NAME "spinand"
#endif

#define NAND_MODEL (NAND_INFO->_model_info)


struct luna_nand_t {
	struct nand_chip chip;
	union {
		struct spi_nand_flash_info_s *_spi_nand_info;
		struct onfi_info_s *_onfi_nand_info;
	};
	struct luna_nand_model_t *model;
	uint8_t *_oob_poi;
	uint32_t _writesize;
	uint32_t _spare_size;
	uint32_t readIndex;
	uint32_t writeIndex;
	uint32_t pre_command;
	uint32_t address_shift; // block number shift for different nand chip
	uint32_t static_shift;  // static block number chip for use different nand chip
	int32_t selected_chip;
	int32_t chip_number;
	uint32_t column;
	uint32_t page_addr;
	union {
		uint32_t v;
		uint8_t b[4];
		uint16_t w[2];
	}status;
	uint32_t wdata_blank_flag; // flag if data in write buffer are all 0xFF
	uint8_t *_bbt;
	uint32_t *_bbt_table;
	uint8_t *_ecc_buf;
	uint8_t *_page_buf;
#if USE_SECOND_BBT
	u32_t *_bbt_table2;
#endif
	struct nand_controller  base;
	u64 dma_mask;

};

#define BLK_TO_BPADDR(bk_num) ((bk_num)<<PAGE_SHF)                            // block number to block page address
#define BPADDR_GET_BLK(block_page_addr) ((block_page_addr)>>PAGE_SHF)         // get block number from block page address
#define BPADDR_GET_PGE(block_page_addr) ((block_page_addr)&((1<<PAGE_SHF)-1)) // get page number from block page address
#define BLK_PGE_TO_BPADDR(bk_num, pge_num) (((bk_num)<<PAGE_SHF )|(pge_num))  // block number and page number to block page address


#endif




