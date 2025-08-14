#if defined(__LUNA_KERNEL__)
#include <naf_kernel_util.h>
#include <linux/delay.h>
#include <linux/slab.h>

#include <spi_nand_ctrl.h>
#include <spi_nand_common.h>
#ifdef CONFIG_SPI_NAND_FLASH_INIT_FIRST
    #include <spi_nand_util.h>
#endif
#if defined(CONFIG_UNDER_UBOOT) && !defined(CONFIG_SPI_NAND_FLASH_INIT_FIRST)
    #include <spi_nand_blr_util.h>
    #include <spi_nand_symb_func.h>
#endif
extern struct spi_nand_flash_info_s *_spi_nand_info;
#else
#include <spi_nand/spi_nand_ctrl.h>
#include <spi_nand/spi_nand_common.h>
#include <util.h>
#ifdef CONFIG_SPI_NAND_FLASH_INIT_FIRST
    #include <spi_nand/spi_nand_util.h>
#endif
#if defined(CONFIG_UNDER_UBOOT) && !defined(CONFIG_SPI_NAND_FLASH_INIT_FIRST)
    #include <spi_nand/spi_nand_blr_util.h>
    #include <spi_nand/spi_nand_symb_func.h>        
#endif
#endif

//#define  NSU_DHC_USING_QIO 1

spi_nand_model_info_t snaf_dhc_ode_model;
static void
snaf_dhc_pio_read_data(spi_nand_flash_info_t *info, void *ram_addr, u32_t wr_bytes, u32_t blk_pge_addr, u32_t col_addr)
{
	unsigned char *buf;
	unsigned int page_size = 1024*(1<<info->_page_size)+32*(1<<info->_spare_size);
	
	buf = kmalloc(page_size, GFP_ATOMIC);
	if (!buf || (col_addr > page_size)) {
		printk("%s(%d): buf=%p %d>%d\n",__func__,__LINE__,buf,col_addr,page_size);
		return;
	}
	
	info->_model_info->_page_read(info, buf, blk_pge_addr);	
	memcpy( ram_addr, &buf[col_addr], wr_bytes);
	kfree(buf);
}




/***********************************************
  *  DHC's ID Definition
  ***********************************************/
#define MID_DHC (0x23)
#define DID_1Gb (0x12)
#define DID_2Gb (0x22)
#define DID_4Gb (0x32)

// policy decision
    //input: #define NSU_PROHIBIT_QIO, or NSU_PROHIBIT_DIO  (in project/info.in)
    //          #define NSU_DHC_USING_QIO, NSU_DHC_USING_DIO, NSU_DHC_USING_SIO  (in project/info.in)
    //          #define NSU_DRIVER_IN_ROM, IS_RECYCLE_SECTION_EXIST (in template/info.in)
    //          #define NSU_USING_SYMBOL_TABLE_FUNCTION (in project/info.in)  

    //output: #define __DEVICE_REASSIGN, __DEVICE_USING_SIO, __DEVICE_USING_DIO, and __DEVICE_USING_QIO
    //            #define __SECTION_INIT_PHASE, __SECTION_INIT_PHASE_DATA
    //            #define __SECTION_RUNTIME, __SECTION_RUNTIME_DATA

#ifdef NSU_DRIVER_IN_ROM
    #define __SECTION_INIT_PHASE      SECTION_SPI_NAND
    #define __SECTION_INIT_PHASE_DATA SECTION_SPI_NAND_DATA
    #define __SECTION_RUNTIME         SECTION_SPI_NAND
    #define __SECTION_RUNTIME_DATA    SECTION_SPI_NAND_DATA
    #if defined(NSU_DHC_USING_QIO) || defined(NSU_DHC_USING_DIO)
        #error 'lplr should not run at ...'
    #endif
    #ifdef IS_RECYCLE_SECTION_EXIST
        #error 'lplr should not have recycle section ...'
    #endif
    #define __DEVICE_USING_SIO 0
    #define __DEVICE_USING_DIO 0
    #define __DEVICE_USING_QIO 1
#else
    #ifdef NSU_USING_SYMBOL_TABLE_FUNCTION
        #define __DEVICE_REASSIGN 1
    #else
        #error 'plr should use symbol table'
    #endif
    #ifdef IS_RECYCLE_SECTION_EXIST
        #define __SECTION_INIT_PHASE        SECTION_RECYCLE
        #define __SECTION_INIT_PHASE_DATA   SECTION_RECYCLE_DATA
        #define __SECTION_RUNTIME           SECTION_UNS_TEXT
        #define __SECTION_RUNTIME_DATA      SECTION_UNS_RO
    #else
        #define __SECTION_INIT_PHASE
        #define __SECTION_INIT_PHASE_DATA
        #define __SECTION_RUNTIME
        #define __SECTION_RUNTIME_DATA
    #endif

    #ifdef NSU_DHC_USING_QIO
        #if defined(NSU_PROHIBIT_QIO) && defined(NSU_PROHIBIT_DIO)
            #define __DEVICE_USING_SIO 1
            #define __DEVICE_USING_DIO 0
            #define __DEVICE_USING_QIO 0
        #elif defined(NSU_PROHIBIT_QIO) 
            #define __DEVICE_USING_SIO 0
            #define __DEVICE_USING_DIO 1
            #define __DEVICE_USING_QIO 0
        #else
            #define __DEVICE_USING_SIO 0
            #define __DEVICE_USING_DIO 0
            #define __DEVICE_USING_QIO 1
        #endif
    #elif defined(NSU_DHC_USING_DIO)
        #if defined(NSU_PROHIBIT_DIO)
            #define __DEVICE_USING_SIO 1
            #define __DEVICE_USING_DIO 0
            #define __DEVICE_USING_QIO 0
        #else
            #define __DEVICE_USING_SIO 0
            #define __DEVICE_USING_DIO 1
            #define __DEVICE_USING_QIO 0
        #endif
    #else
        #define __DEVICE_USING_SIO 1
        #define __DEVICE_USING_DIO 0
        #define __DEVICE_USING_QIO 0
    #endif
#endif

//The Toshiba specific function
void dhc_ecc_encode(u32_t ecc_ability, void *dma_addr, void *fake_ptr_cs);
s32_t dhc_ecc_decode(u32_t ecc_ability, void *dma_addr, void *fake_ptr_cs);

#ifdef CONFIG_SPI_NAND_FLASH_INIT_FIRST
#if __DEVICE_USING_QIO
__SECTION_INIT_PHASE_DATA
spi_nand_cmd_info_t dhc_x4_cmd_info = {
    .w_cmd = PROGRAM_LOAD_OP,
    .w_addr_io = SIO_WIDTH,
    .w_data_io = SIO_WIDTH,
    .r_cmd = FAST_READ_X4_OP,
    .r_addr_io = SIO_WIDTH,
    .r_data_io = QIO_WIDTH,
    .r_dummy_cycles  = 8,
};
#endif

__SECTION_INIT_PHASE_DATA
spi_nand_flash_info_t dhc_chip_info[] = {
    {
        .man_id              = MID_DHC, 
        .dev_id              = DID_1Gb,
        ._num_block          = SNAF_MODEL_NUM_BLK_1024,
        ._num_page_per_block = SNAF_MODEL_NUM_PAGE_64,
        ._page_size          = SNAF_MODEL_PAGE_SIZE_2048B,
        ._spare_size         = SNAF_MODEL_SPARE_SIZE_64B,
        ._oob_size           = SNAF_MODEL_OOB_SIZE(24),
        ._ecc_ability        = ECC_USE_ODE,        
        #if __DEVICE_REASSIGN
            ._ecc_encode     = VZERO,
            ._ecc_decode     = VZERO,
            ._reset          = VZERO,
            ._cmd_info       = VZERO,
            ._model_info     = VZERO,
        #else
            ._ecc_encode     = dhc_ecc_encode,
            ._ecc_decode     = dhc_ecc_decode,
            ._reset          = nsu_reset_spi_nand_chip,
            ._model_info     = &snaf_dhc_ode_model,
            #if __DEVICE_USING_SIO
                ._cmd_info   = &nsc_sio_cmd_info,
                #elif __DEVICE_USING_DIO
                ._cmd_info   = &nsc_x2_cmd_info,
            #elif __DEVICE_USING_QIO
                ._cmd_info   = &dhc_x4_cmd_info,
            #endif
        #endif
    },
    {
        .man_id              = MID_DHC, 
        .dev_id              = DID_2Gb,
        ._num_block          = SNAF_MODEL_NUM_BLK_2048,
        ._num_page_per_block = SNAF_MODEL_NUM_PAGE_64,
        ._page_size          = SNAF_MODEL_PAGE_SIZE_2048B,
        ._spare_size         = SNAF_MODEL_SPARE_SIZE_64B,
        ._oob_size           = SNAF_MODEL_OOB_SIZE(24),
        ._ecc_ability        = ECC_USE_ODE,        
        #if __DEVICE_REASSIGN
            ._ecc_encode     = VZERO,
            ._ecc_decode     = VZERO,
            ._reset          = VZERO,
            ._cmd_info       = VZERO,
            ._model_info     = VZERO,
        #else
            ._ecc_encode     = dhc_ecc_encode,
            ._ecc_decode     = dhc_ecc_decode,
            ._reset          = nsu_reset_spi_nand_chip,
            ._model_info     = &snaf_dhc_ode_model,
            #if __DEVICE_USING_SIO
                ._cmd_info   = &nsc_sio_cmd_info,
            #elif __DEVICE_USING_DIO
                ._cmd_info   = &nsc_x2_cmd_info,
            #elif __DEVICE_USING_QIO
                ._cmd_info   = &dhc_x4_cmd_info,
            #endif
        #endif
    },
    {
        .man_id              = MID_DHC, 
        .dev_id              = DID_4Gb,
        ._num_block          = SNAF_MODEL_NUM_BLK_2048,
        ._num_page_per_block = SNAF_MODEL_NUM_PAGE_64,
        ._page_size          = SNAF_MODEL_PAGE_SIZE_4096B,
        ._spare_size         = SNAF_MODEL_SPARE_SIZE_128B,
        ._oob_size           = SNAF_MODEL_OOB_SIZE(48),
        ._ecc_ability        = ECC_USE_ODE,        
        #if __DEVICE_REASSIGN
            ._ecc_encode     = VZERO,
            ._ecc_decode     = VZERO,
            ._reset          = VZERO,
            ._cmd_info       = VZERO,
            ._model_info     = VZERO,
        #else
            ._ecc_encode     = dhc_ecc_encode,
            ._ecc_decode     = dhc_ecc_decode,
            ._reset          = nsu_reset_spi_nand_chip,
            ._model_info     = &snaf_dhc_ode_model,
            #if __DEVICE_USING_SIO
                ._cmd_info   = &nsc_sio_cmd_info,
            #elif __DEVICE_USING_DIO
                ._cmd_info   = &nsc_x2_cmd_info,
            #elif __DEVICE_USING_QIO
                ._cmd_info   = &dhc_x4_cmd_info,
            #endif
        #endif
    },
};
#endif // CONFIG_SPI_NAND_FLASH_INIT_FIRST

__SECTION_RUNTIME static void
dhc_ecc_encode(u32_t ecc_ability, void *dma_addr, void *fake_ptr_cs)
{
    return;
}


__SECTION_RUNTIME static s32_t
dhc_ecc_decode(u32_t ecc_ability, void *dma_addr, void *fake_ptr_cs)
{
    const u8_t check_ode_reg = 0xC0;
    const u32_t cs = 0;
    u32_t sts = (nsu_get_feature_reg(cs, check_ode_reg) >> 4)& 0x7;
    if(sts == 2){
        return  (ECC_CTRL_ERR|0xF);
    }

    return 0;
}

__SECTION_INIT_PHASE static u32_t
dhc_read_id(u32_t cs)
{
    u32_t w_io_len = IO_WIDTH_LEN(SIO_WIDTH,CMR_LEN(2));
    u32_t r_io_len = IO_WIDTH_LEN(SIO_WIDTH,CMR_LEN(2));
    u32_t ret = nsu_read_spi_nand_id(cs, 0, w_io_len, r_io_len);
    return ((ret>>16)&0xFFFF);
}

#ifndef CONFIG_UNDER_UBOOT
__SECTION_INIT_PHASE static spi_nand_flash_info_t *
probe_dhc_spi_nand_chip(void)
{
    nsu_reset_spi_nand_chip(0);
    snaf_dhc_ode_model           = snaf_ode_model;
    snaf_dhc_ode_model._pio_read = snaf_dhc_pio_read_data;

    #ifndef NSU_DRIVER_IN_ROM
    udelay(1100);
    #endif
    
    u32_t rdid = dhc_read_id(0);
    if(MID_DHC != ((rdid >>8)&0xFF)) return VZERO;

    u16_t did8 = rdid &0xFF;
    u32_t i;   
    for(i=0 ; i<ELEMENT_OF_SNAF_INFO(dhc_chip_info) ; i++){
        if(dhc_chip_info[i].dev_id == did8){
            #if __DEVICE_REASSIGN
                #if __DEVICE_USING_SIO
                    dhc_chip_info[i]._cmd_info = _nsu_cmd_info_ptr;
                #elif __DEVICE_USING_DIO
                    dhc_chip_info[i]._cmd_info = _nsu_dio_cmd_info_ptr;
                #elif __DEVICE_USING_QIO
                    dhc_chip_info[i]._cmd_info = &dhc_x4_cmd_info;
                #endif                 
                dhc_chip_info[i]._model_info = &nsu_model_info;
                dhc_chip_info[i]._reset = _nsu_reset_ptr;
                dhc_chip_info[i]._ecc_encode= dhc_ecc_encode;
                dhc_chip_info[i]._ecc_decode= dhc_ecc_decode;
                dhc_chip_info[i]._model_info->_page_read_ecc = _nsu_page_read_with_ode_ptr;
                dhc_chip_info[i]._model_info->_page_write_ecc = _nsu_page_write_with_ode_ptr;
            #endif //__DEVICE_REASSIGN

            return &dhc_chip_info[i];
        }
    }
    return VZERO;
}
REG_SPI_NAND_PROBE_FUNC(probe_dhc_spi_nand_chip);
#endif   // CONFIG_SPI_NAND_FLASH_INIT_FIRST
