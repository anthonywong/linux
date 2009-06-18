/*
 * linux/drivers/video/dovedcon.c -- Marvell DCON driver for DOVE
 *
 *
 * Copyright (C) Marvell Semiconductor Company.  All rights reserved.
 *
 * Written by:
 *	Green Wan <gwan@marvell.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of this archive for
 * more details.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include <video/dovedcon.h>

extern int enable_ebook;

static int dovedcon_enable(struct dovedcon_info *ddi)
{
	unsigned int channel_ctrl;

	channel_ctrl = 0x90C78;
	writel(channel_ctrl, ddi->reg_base+DCON_VGA_DAC_CHANNEL_A_CTRL);
	writel(channel_ctrl, ddi->reg_base+DCON_VGA_DAC_CHANNEL_B_CTRL);
	writel(channel_ctrl, ddi->reg_base+DCON_VGA_DAC_CHANNEL_C_CTRL);

	return 0;
}

#ifdef CONFIG_PM

/* suspend and resume support */
static int dovedcon_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct dovedcon_info *ddi = platform_get_drvdata(pdev);

	if (enable_ebook) 
		return 0;

	printk(KERN_INFO "dovedcon_suspend().\n");

	clk_disable(ddi->clk);

	return 0;
}

static int dovedcon_resume(struct platform_device *pdev)
{
	struct dovedcon_info *ddi = platform_get_drvdata(pdev);

	if (enable_ebook) 
		return 0;

	printk(KERN_INFO "dovedcon_resume().\n");
	clk_enable(ddi->clk);
	dovedcon_enable(ddi);

	return 0;
}

#endif

/* Initialization */
static int __init dovedcon_probe(struct platform_device *pdev)
{
	struct dovedcon_mach_info *ddmi;
	struct dovedcon_info *ddi;
	struct resource *res;

	ddmi = pdev->dev.platform_data;
	if (!ddmi)
		return -EINVAL;

	ddi = kzalloc(sizeof(struct dovedcon_info), GFP_KERNEL);
	if (ddi == NULL)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		kfree(ddi);
		return -EINVAL;
	}

	if (!request_mem_region(res->start, res->end - res->start,
	    "MRVL DCON Regs")) {
		printk(KERN_INFO "Cannot reserve DCON memory mapped"
			" area 0x%lx @ 0x%lx\n",
			(unsigned long)res->start,
			(unsigned long)res->end - res->start);
		kfree(ddi);
		return -ENXIO;
	}

	ddi->reg_base = ioremap_nocache(res->start, res->end - res->start);

	if (!ddi->reg_base) {
		kfree(ddi);
		return -EINVAL;
	}

	platform_set_drvdata(pdev, ddi);

	ddi->clk = clk_get(&pdev->dev, "LCD");

	/* Initialize DCON hardware */
	dovedcon_enable(ddi);

	printk(KERN_INFO "dovedcon has been initialized.\n");

	return 0;
}

/*
 *  Cleanup
 */
static int dovedcon_remove(struct platform_device *pdev)
{
	struct dovedcon_info *ddi = platform_get_drvdata(pdev);

	iounmap(ddi->reg_base);
	kfree(ddi);

	return 0;
}

static struct platform_driver dovedcon_pdriver = {
	.probe		= dovedcon_probe,
	.remove		= dovedcon_remove,
#ifdef CONFIG_PM
	.suspend	= dovedcon_suspend,
	.resume		= dovedcon_resume,
#endif /* CONFIG_PM */
	.driver		=	{
		.name	=	"dovedcon",
		.owner	=	THIS_MODULE,
	},
};
static int __init dovedcon_init(void)
{
	return platform_driver_register(&dovedcon_pdriver);
}

static void __exit dovedcon_exit(void)
{
	platform_driver_unregister(&dovedcon_pdriver);
}

module_init(dovedcon_init);
module_exit(dovedcon_exit);

MODULE_AUTHOR("Green Wan <gwan@marvell.com>");
MODULE_DESCRIPTION("DCON driver for Dove LCD unit");
MODULE_LICENSE("GPL");
