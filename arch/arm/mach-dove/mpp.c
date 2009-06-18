/*
 * arch/arm/mach-dove/mpp.c
 *
 * MPP functions for Marvell Dove SoCs
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mbus.h>
#include <asm/gpio.h>
//#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/setup.h>
#include <asm/mach/arch.h>
#include "common.h"
#include "mpp.h"

/*
 * we have 24 single pin mpps, and 6 groups of mpps, each group controlled
 * by one bit in the GPIO functional select register. we treat each group
 * as one mpp, so we have here 24 + 6 mpps
 */

static int __init determine_type_encoding(int mpp, enum dove_mpp_type type)
{
	switch (type) {
	case MPP_UNUSED:
	case MPP_GPIO:
		return (mpp < 24) ? 0 : 1;
	case MPP_SATA_LED:
		return 0x2;
	case MPP_FUNCTIONAL:
		return 0x0;
	case MPP_SPI:
		return 0x02;
	}
	
	printk(KERN_INFO "unknown MPP type %d\n", type);
	
	return -1;
}

static int __init dove_get_gpios(int mpp, int *num)
{
	switch (mpp) {
	case 0 ... 23:
		*num = 1;
		return mpp;
	case 24: /* SDO */ 
		*num = 6;
		return 40;
	case 25: /* SD1 */
		*num = 6;
		return 46;
	case 26: /* CAM */
		*num = 16;
		return 24;
	case 27: /* AU1 */
		*num = 6;
		return 52;
	case 28: /* UA1 */
		*num = 2;
		return 62;
	case 29: /* SPI */
		*num = 4;
		return 58;
	}
	
	printk(KERN_INFO "unknown MPP num %d\n", mpp);
	
	return -1;
}

void __init dove_config_gpios(struct dove_mpp_mode *mode)
{
	int gpio;
	int num = -1;
	int i;
	
	gpio = dove_get_gpios(mode->mpp, &num);
	
	if (num < 0)
		return;
	
	for(i = 0; i < num; i++){
		if (mode->type == MPP_UNUSED)
			orion_gpio_set_unused(gpio + i);
		
		orion_gpio_set_valid(gpio + i, !!(mode->type == MPP_GPIO));
	}
}
void __init dove_mpp_conf(struct dove_mpp_mode *mode)
{
	u32 mpp_ctrl0 = readl(DOVE_MPP_VIRT_BASE);
	u32 mpp_ctrl1 = readl(DOVE_MPP_VIRT_BASE + 4);
	u32 mpp_ctrl2 = readl(DOVE_MPP_VIRT_BASE + 8);
	u32 func_gpio_select = readl(DOVE_FUNC_GPIO_SELECT_VIRT_BASE);

	while (mode->mpp >= 0) {
		u32 *reg;
		int num_type;
		int shift;


		if (mode->mpp >= 0 && mode->mpp <= 7)
			reg = &mpp_ctrl0;
		else if (mode->mpp >= 8 && mode->mpp <= 15)
			reg = &mpp_ctrl1;
		else if (mode->mpp >= 16 && mode->mpp <= 23)
			reg = &mpp_ctrl2;
		else if (mode->mpp >= 24 && mode->mpp <= 30)
			reg = &func_gpio_select;
		else {
			printk(KERN_ERR "%s: invalid MPP "
			       "(%d)\n", __func__, mode->mpp);
			continue;
		}

		num_type = determine_type_encoding(mode->mpp, mode->type);
		if (num_type < 0) {
			printk(KERN_ERR "%s: invalid MPP "
			       "combination (%d, %d)\n", __func__, mode->mpp,
			       mode->type);
			continue;
		}
		if(mode->mpp < 24) {
			shift = (mode->mpp & 7) << 2;
			*reg &= ~(0xf << shift);
			*reg |= (num_type & 0xf) << shift;
		}else{
			shift = (mode->mpp - 24);
			*reg &= ~(0x1 << shift);
			*reg |= (num_type & 0x1) << shift;
		}
		dove_config_gpios(mode);
		mode++;
	}

	writel(mpp_ctrl0, DOVE_MPP_VIRT_BASE);
	writel(mpp_ctrl1, DOVE_MPP_VIRT_BASE + 4);
	writel(mpp_ctrl2, DOVE_MPP_VIRT_BASE + 8);
	writel(func_gpio_select, DOVE_FUNC_GPIO_SELECT_VIRT_BASE);
}
