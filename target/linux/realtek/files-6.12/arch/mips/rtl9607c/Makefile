# SPDX-License-Identifier: GPL-2.0-only
# Makefile for the RTL9607C specific parts of the kernel
#

obj-y := setup.o irq.o prom.o timer.o promlib.o dma.o init_l2cache.o

obj-$(CONFIG_EARLY_PRINTK) += early-printk.o
obj-$(CONFIG_CPU_HAS_WB) += wb_flush.o
obj-$(CONFIG_OF)  += of.o
obj-$(CONFIG_PCI) += pci.o
obj-$(CONFIG_USB) += usb.o
obj-$(CONFIG_SMP) += smp.o
