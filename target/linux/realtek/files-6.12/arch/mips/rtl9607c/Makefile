# SPDX-License-Identifier: GPL-2.0-only
# Makefile for the RTL9607C specific parts of the kernel
#

obj-y := setup.o prom.o dma.o init_l2cache.o

obj-$(CONFIG_CPU_HAS_WB) += wb_flush.o
obj-$(CONFIG_PCI) += pci.o
obj-$(CONFIG_USB) += usb.o
obj-$(CONFIG_SMP) += smp.o
