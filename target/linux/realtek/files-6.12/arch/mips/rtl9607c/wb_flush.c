// SPDX-License-Identifier: GPL-2.0-only

#include <linux/export.h>
#include <linux/init.h>

#include <asm/bootinfo.h>
#include <asm/wbflush.h>
#include <asm/barrier.h>
uint32_t wbtarget;
void rtk_wbflush(void);
 void rtk_wbflush(void) {
        asm volatile(".set push\n\t"
                        ".set noreorder\n\t"
                        "la     $8, wbtarget\n\t"
                        "lui    $9, 0x2000\n\t"
                        "or     $8, $8, $9\n\t"
                        "sw     $0, 0($8)\n\t"
                        "nop    \n\t"
                        "lw     $9, 0($8)\n\t"
                        ".set pop\n\t"
                        :
                        :
                        : "$8", "$9"
                        );
}
void (*__wbflush)(void) = rtk_wbflush;
EXPORT_SYMBOL(__wbflush);

