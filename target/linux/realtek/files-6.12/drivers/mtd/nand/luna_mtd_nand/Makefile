
SPI_NAND_PATH  := spi_nand_gen3
ECC_PATH       := ecc_gen2

ccflags-y := -I$(src) -I$(src)/$(ECC_PATH) -I../ -D__LUNA_KERNEL__
ccflags-$(CONFIG_MTD_SPI_NAND_RTK) += -I$(src)/$(SPI_NAND_PATH)

spin_flist := spi_nand_common.o
spin_flist += spi_nand_ctrl.o
spin_flist += spi_nand_esmt.o
spin_flist += spi_nand_winbond.o
spin_flist += spi_nand_gd.o
spin_flist += spi_nand_zentel.o
spin_flist += spi_nand_toshiba.o
spin_flist += spi_nand_mxic.o
spin_flist += spi_nand_micron.o
spin_flist += spi_nand_ato.o
spin_flist += spi_nand_heyangtek.o
spin_flist += spi_nand_longsys.o
spin_flist += spi_nand_dosilicon.o
spin_flist += spi_nand_etron.o
spin_flist += spi_nand_fmsh.o
spin_flist += spi_nand_xtx.o
spin_flist += spi_nand_9Chapters.o
spin_flist += spi_nand_umtek.o
ifeq ($(RTL9607C), y)
spin_flist += spi_nand_dhc.o
endif
spin_flist := $(addprefix $(SPI_NAND_PATH)/,$(spin_flist))


ecc_flist := ecc_ctrl.o
ecc_flist := $(addprefix $(ECC_PATH)/,$(ecc_flist))

obj-y += $(ecc_flist)
obj-$(CONFIG_MTD_SPI_NAND_RTK)  += $(spin_flist)
obj-y += luna_mtd_protect.o
obj-y += luna_mtd_usage_count.o
obj-y += luna_mtd_proc_bbt2.o
obj-y += block_state_proc.o
obj-y += luna_mtd_nand.o

