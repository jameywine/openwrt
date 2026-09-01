/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_MACH_RTL_OTTO_SPACES_H
#define __ASM_MACH_RTL_OTTO_SPACES_H

#ifdef CONFIG_RTL960X
/*
 * Double space for RTL9607C - for two PCIe ports
 */
#define IO_SPACE_LIMIT 0x1ffff
#endif /* CONFIG_RTL960X */

#include <asm/mach-generic/spaces.h>

#endif /* __ASM_MACH_RTL_OTTO_SPACES_H */
