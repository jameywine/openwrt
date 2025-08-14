#if defined(__LUNA_KERNEL__)
#include <naf_kernel_util.h>
#include <linux/delay.h>
#include <spi_nand_ctrl.h>
#include <spi_nand_common.h>
#ifdef CONFIG_SPI_NAND_FLASH_INIT_FIRST
    #include <spi_nand_util.h>
#endif
#if defined(CONFIG_UNDER_UBOOT) && !defined(CONFIG_SPI_NAND_FLASH_INIT_FIRST)
    #include <spi_nand_blr_util.h>
    #include <spi_nand_symb_func.h>        
#endif
#undef __DEVICE_REASSIGN
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

/***********************************************
  *  XTX's ID Definition
  ***********************************************/
#define MID_XTX      (0x0B)
#define DID_XT26G01B (0xF1) //2048+64, SIO=90MHz, QIO=50MHz, ECC=8b/528B
#define DID_XT26G01C (0x11) //2048+128, QIO=50MHz, ECC=8b/528B
#define DID_XT26G02B (0xF2) //2048+64, QIO=50MHz, ECC=4b/528B
#define DID_XT26G12DWSIGA (0x35) //2048+128, QIO=60MHz, ECC=8b/528B

// policy decision
    //input: #define NSU_PROHIBIT_QIO, or NSU_PROHIBIT_DIO  (in project/info.in)
    //       #define NSU_XTX_USING_QIO, NSU_XTX_USING_DIO, NSU_XTX_USING_SIO  (in project/info.in)
    //       #define NSU_DRIVER_IN_ROM, IS_RECYCLE_SECTION_EXIST (in template/info.in)
    //       #define NSU_USING_SYMBOL_TABLE_FUNCTION (in project/info.in)

    //output: #define __DEVICE_REASSIGN, __DEVICE_USING_SIO, __DEVICE_USING_DIO, and __DEVICE_USING_QIO
    //        #define __SECTION_INIT_PHASE, __SECTION_INIT_PHASE_DATA
    //        #define __SECTION_RUNTIME, __SECTION_RUNTIME_DATA

#ifdef NSU_DRIVER_IN_ROM
    #define __SECTION_INIT_PHASE      SECTION_SPI_NAND
    #define __SECTION_INIT_PHASE_DATA SECTION_SPI_NAND_DATA
    #define __SECTION_RUNTIME         SECTION_SPI_NAND
    #define __SECTION_RUNTIME_DATA    SECTION_SPI_NAND_DATA
    #if defined(NSU_XTX_USING_QIO) || defined(NSU_XTX_USING_DIO)
        #error 'lplr should not run at ...'
    #endif
    #ifdef IS_RECYCLE_SECTION_EXIST
        #error 'lplr should not have recycle section ...'
    #endif
    #define __DEVICE_USING_SIO 1
    #define __DEVICE_USING_DIO 0
    #define __DEVICE_USING_QIO 0
#else
    #ifdef NSU_USING_SYMBOL_TABLE_FUNCTION
        #define __DEVICE_REASSIGN 1
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

    #ifdef NSU_XTX_USING_QIO
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
    #elif defined(NSU_XTX_USING_DIO)
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

#ifdef CONFIG_SPI_NAND_FLASH_INIT_FIRST
#if __DEVICE_USING_QIO
__SECTION_INIT_PHASE_DATA
spi_nand_cmd_info_t xtx_qio_cmd_info = {
    .w_cmd = PROGRAM_LOAD_X4_OP,
    .w_addr_io = SIO_WIDTH,
    .w_data_io = QIO_WIDTH,
    .r_cmd = FAST_READ_QIO_OP,
    .r_addr_io = QIO_WIDTH,
    .r_data_io = QIO_WIDTH,
    .r_dummy_cycles  = 8,
};
#endif

extern void xtx_ecc_encode(u32_t ecc_ability, void *dma_addr, void *fake_ptr_cs);
extern s32_t xtx26g01b_ecc_decode(u32_t ecc_ability, void *dma_addr, void *fake_ptr_cs);
extern s32_t xtx26g01c_ecc_decode(u32_t ecc_ability, void *dma_addr, void *fake_ptr_cs);
s32_t xtx26g1xd_ecc_decode(u32_t ecc_ability, void *dma_addr, void *fake_ptr_cs);

__SECTION_INIT_PHASE_DATA
spi_nand_flash_info_t xtx_chip_info[] = {
    {
        .man_id              = MID_XTX,
        .dev_id              = DID_XT26G01B,
        ._num_block          = SNAF_MODEL_NUM_BLK_1024,
        ._num_page_per_block = SNAF_MODEL_NUM_PAGE_64,
        ._page_size          = SNAF_MODEL_PAGE_SIZE_2048B,
        ._spare_size         = SNAF_MODEL_SPARE_SIZE_64B,
        ._oob_size           = SNAF_MODEL_OOB_SIZE(24),
        ._ecc_ability        = ECC_USE_ODE,
        #ifdef __DEVICE_REASSIGN
            ._ecc_encode     = VZERO,
            ._ecc_decode     = VZERO,
            ._reset          = VZERO,
            ._cmd_info       = VZERO,
            ._model_info     = VZERO,
        #else
            ._ecc_encode     = ecc_encode_bch,
            ._ecc_decode     = xtx26g01b_ecc_decode,
            ._reset          = nsu_reset_spi_nand_chip,
            ._model_info     = &snaf_ode_model,
            #if __DEVICE_USING_SIO
                ._cmd_info   = &nsc_sio_cmd_info,
            #elif __DEVICE_USING_DIO
                ._cmd_info   = &nsc_dio_cmd_info,
            #elif __DEVICE_USING_QIO
                ._cmd_info   = &xtx_qio_cmd_info,
            #endif
        #endif
    },
    {
        .man_id              = MID_XTX,
        .dev_id              = DID_XT26G01C,
        ._num_block          = SNAF_MODEL_NUM_BLK_1024,
        ._num_page_per_block = SNAF_MODEL_NUM_PAGE_64,
        ._page_size          = SNAF_MODEL_PAGE_SIZE_2048B,
        ._spare_size         = SNAF_MODEL_SPARE_SIZE_64B,
        ._oob_size           = SNAF_MODEL_OOB_SIZE(24),
        ._ecc_ability        = ECC_USE_ODE,
        #ifdef __DEVICE_REASSIGN
            ._ecc_encode     = VZERO,
            ._ecc_decode     = VZERO,
            ._reset          = VZERO,
            ._cmd_info       = VZERO,
            ._model_info     = VZERO,
        #else
            ._ecc_encode     = xtx_ecc_encode,
            ._ecc_decode     = xtx26g01c_ecc_decode,
            ._reset          = nsu_reset_spi_nand_chip,
            ._model_info     = &snaf_ode_model,
            #if __DEVICE_USING_SIO
                ._cmd_info   = &nsc_sio_cmd_info,
            #elif __DEVICE_USING_DIO
                ._cmd_info   = &nsc_dio_cmd_info,
            #elif __DEVICE_USING_QIO
                ._cmd_info   = &xtx_qio_cmd_info,
            #endif
        #endif
    },
    {
        .man_id              = MID_XTX,
        .dev_id              = DID_XT26G02B,
        ._num_block          = SNAF_MODEL_NUM_BLK_2048,
        ._num_page_per_block = SNAF_MODEL_NUM_PAGE_64,
        ._page_size          = SNAF_MODEL_PAGE_SIZE_2048B,
        ._spare_size         = SNAF_MODEL_SPARE_SIZE_64B,
        ._oob_size           = SNAF_MODEL_OOB_SIZE(24),
        ._ecc_ability        = ECC_MODEL_6T,
        #ifdef __DEVICE_REASSIGN
            ._ecc_encode     = VZERO,
            ._ecc_decode     = VZERO,
            ._reset          = VZERO,
            ._cmd_info       = VZERO,
            ._model_info     = VZERO,
        #else
            ._ecc_encode     = ecc_encode_bch,
            ._ecc_decode     = ecc_decode_bch,
            ._reset          = nsu_reset_spi_nand_chip,
            ._model_info     = &snaf_rom_general_model,
            #if __DEVICE_USING_SIO
                ._cmd_info   = &nsc_sio_cmd_info,
            #elif __DEVICE_USING_DIO
                ._cmd_info   = &nsc_dio_cmd_info,
            #elif __DEVICE_USING_QIO
                ._cmd_info   = &xtx_qio_cmd_info,
            #endif
        #endif
    },
    {
        .man_id              = MID_XTX,
        .dev_id              = DID_XT26G12DWSIGA,
        ._num_block          = SNAF_MODEL_NUM_BLK_2048,
        ._num_page_per_block = SNAF_MODEL_NUM_PAGE_64,
        ._page_size          = SNAF_MODEL_PAGE_SIZE_2048B,
        ._spare_size         = SNAF_MODEL_SPARE_SIZE_64B,
        ._oob_size           = SNAF_MODEL_OOB_SIZE(24),
        ._ecc_ability        = ECC_USE_ODE,
#ifdef __DEVICE_REASSIGN
        ._ecc_encode     = VZERO,
        ._ecc_decode     = VZERO,
        ._reset          = VZERO,
        ._cmd_info       = VZERO,
        ._model_info     = VZERO,
#else
        ._ecc_encode     = xtx_ecc_encode,
        ._ecc_decode     = xtx26g1xd_ecc_decode,
        ._reset          = nsu_reset_spi_nand_chip,
        ._model_info     = &snaf_rom_general_model,
#if __DEVICE_USING_SIO
        ._cmd_info   = &nsc_sio_cmd_info,
#elif __DEVICE_USING_DIO
        ._cmd_info   = &nsc_dio_cmd_info,
#elif __DEVICE_USING_QIO
        ._cmd_info   = &xtx_qio_cmd_info,
#endif
#endif
    },

};
#endif // CONFIG_SPI_NAND_FLASH_INIT_FIRST

#if __DEVICE_USING_QIO
__SECTION_INIT_PHASE static void
xtx_quad_enable(u32_t cs)
{
    u32_t feature_addr=0xB0;
    u32_t value = 1;
    value |= nsu_get_feature_reg(cs, feature_addr);
    nsu_set_feature_reg(cs, feature_addr, value);
}
#endif

__SECTION_INIT_PHASE static void
xt26g02b_disable_on_die_ecc(u32_t cs)
{
    u32_t feature_addr=0x90;
    u32_t value = nsu_get_feature_reg(cs, feature_addr);
    value &= ~(1<<4);
    nsu_set_feature_reg(cs, feature_addr,value);
}

__SECTION_RUNTIME void
xtx_ecc_encode(u32_t ecc_ability, void *dma_addr, void *fake_ptr_cs)
{
    return;
}

__SECTION_RUNTIME s32_t
xtx26g01b_ecc_decode(u32_t ecc_ability, void *dma_addr, void *fake_ptr_cs)
{
    u32_t cs = 0;
    u8_t eccs = (nsu_get_feature_reg(cs, 0xC0) >> 2)&0xF;
    if (eccs == 0x8) {
        return ECC_USE_ODE_ERR;
    } else if (eccs == 0xC) {
        return 8;
    } else {
        return eccs;
    }
}

__SECTION_RUNTIME s32_t
xtx26g01c_ecc_decode(u32_t ecc_ability, void *dma_addr, void *fake_ptr_cs)
{
    u32_t cs = 0;
    u8_t eccs = (nsu_get_feature_reg(cs, 0xC0) >> 4)&0xF;
    return (eccs == 0xF)? ECC_USE_ODE_ERR:eccs;
}

__SECTION_RUNTIME s32_t
xtx26g1xd_ecc_decode(u32_t ecc_ability, void *dma_addr, void *fake_ptr_cs)
{
    u32_t cs = 0;
    u8_t eccs = (nsu_get_feature_reg(cs, 0xC0) & 0xF0) >> 4;

    u32_t eccs10 = (eccs & 0x3);
    u32_t eccs32 = (eccs >> 2) & 0x3;

    if (eccs10 == 2) {
        return ECC_USE_ODE_ERR;
    } else if (eccs10 == 3) {
        return 8;
    } else if (eccs10 == 0) {
        return 0;
    } else {
        return eccs32 + 4;
    }
}


__SECTION_INIT_PHASE static u32_t
xtx_read_id(u32_t cs)
{
    u32_t dummy = 0x00;
    u32_t w_io_len = IO_WIDTH_LEN(SIO_WIDTH,CMR_LEN(2));
    u32_t r_io_len = IO_WIDTH_LEN(SIO_WIDTH,CMR_LEN(2));

    u32_t ret = nsu_read_spi_nand_id(cs, dummy, w_io_len, r_io_len);
    return ((ret>>16)&0xFFFF);
}

#ifdef CONFIG_SPI_NAND_FLASH_INIT_FIRST
__SECTION_INIT_PHASE static spi_nand_flash_info_t *
probe_xtx_spi_nand_chip(void)
{
    nsu_reset_spi_nand_chip(0);
    u32_t rdid = xtx_read_id(0);

    if(MID_XTX != (rdid>>8)) return VZERO;

    u16_t did8  = rdid&0xFF;
    u32_t i;
    for(i=0 ; i<ELEMENT_OF_SNAF_INFO(xtx_chip_info) ; i++){
        if(xtx_chip_info[i].dev_id == did8){
            if((xtx_chip_info[i].dev_id == DID_XT26G01B) || (xtx_chip_info[i].dev_id == DID_XT26G01C) || (xtx_chip_info[i].dev_id == DID_XT26G12DWSIGA)){
                nsu_enable_on_die_ecc(0);
            }else if(xtx_chip_info[i].dev_id == DID_XT26G02B){
                xt26g02b_disable_on_die_ecc(0);
            }

#ifdef __DEVICE_REASSIGN
#if __DEVICE_USING_SIO
            xtx_chip_info[i]._cmd_info = _nsu_cmd_info_ptr;
#elif __DEVICE_USING_DIO
            xtx_chip_info[i]._cmd_info = _nsu_dio_cmd_info_ptr;
#elif __DEVICE_USING_QIO
            xtx_chip_info[i]._cmd_info = &xtx_qio_cmd_info;
#endif
            xtx_chip_info[i]._model_info= &nsu_model_info;
            xtx_chip_info[i]._reset     = _nsu_reset_ptr;

            if(xtx_chip_info[i].dev_id == DID_XT26G01B){
                xtx_chip_info[i]._ecc_encode= xtx_ecc_encode;
                xtx_chip_info[i]._ecc_decode= xtx26g01b_ecc_decode;
                xtx_chip_info[i]._model_info->_page_read_ecc = _nsu_page_read_with_ode_ptr;
                xtx_chip_info[i]._model_info->_page_write_ecc = _nsu_page_write_with_ode_ptr;
            }else if(xtx_chip_info[i].dev_id == DID_XT26G01C){
                xtx_chip_info[i]._ecc_encode= xtx_ecc_encode;
                xtx_chip_info[i]._ecc_decode= xtx26g01c_ecc_decode;
                xtx_chip_info[i]._model_info->_page_read_ecc = _nsu_page_read_with_ode_ptr;
                xtx_chip_info[i]._model_info->_page_write_ecc = _nsu_page_write_with_ode_ptr;
            }else if(xtx_chip_info[i].dev_id == DID_XT26G02B){
                xtx_chip_info[i]._ecc_encode= _nsu_ecc_encode_ptr;
                xtx_chip_info[i]._ecc_decode= _nsu_ecc_decode_ptr;
            }else if(xtx_chip_info[i].dev_id == DID_XT26G12DWSIGA){
                xtx_chip_info[i]._ecc_encode= xtx_ecc_encode;
                xtx_chip_info[i]._ecc_decode= xtx26g1xd_ecc_decode;
            }else{
                xtx_chip_info[i]._ecc_encode= _nsu_ecc_encode_ptr;
                xtx_chip_info[i]._ecc_decode= _nsu_ecc_decode_ptr;
            }
#endif /* #ifdef __DEVICE_REASSIGN */

            nsu_block_unprotect(0);
            #if __DEVICE_USING_QIO
            xtx_quad_enable(0);
            #endif
            return &xtx_chip_info[i];
        }
    }
    return VZERO;
}
REG_SPI_NAND_PROBE_FUNC(probe_xtx_spi_nand_chip);
#endif   // CONFIG_SPI_NAND_FLASH_INIT_FIRST
#ifdef CONFIG_SPI_NAND_FLASH_INIT_REST
static int
xtx_init_rest(void)
{
    u32_t cs=1;

    // check ID, cs0 and cs1 should be identical
    u32_t rdid = xtx_read_id(cs);
    //u32_t cs0_id = (_spi_nand_info->man_id<<8) | _spi_nand_info->dev_id;
    if(((rdid>>8)!=MID_XTX) || ((rdid&0xFF)!=_spi_nand_info->dev_id))
      { return 0; }

    // reset
    nsu_reset_spi_nand_chip(cs);

    nsu_disable_on_die_ecc(cs);
    nsu_block_unprotect(cs);
    return 1;
}
REG_SPI_NAND_INIT_REST_FUNC(xtx_init_rest);
#endif // CONFIG_SPI_NAND_FLASH_INIT_REST


