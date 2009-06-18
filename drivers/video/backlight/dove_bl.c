/*
 * linux/drivers/backlight/dove_bl.c -- Marvell DOVE LCD Backlight driver.
 *
 * Copyright (C) Marvell Semiconductor Company.  All rights reserved.
 *
 * Written by Shadi Ammouri <shadi@marvell.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of this archive for
 * more details.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/fb.h>
#include <linux/backlight.h>
#include <linux/lcd.h>
#include <linux/clk.h>
#include <video/dovefbreg.h>
#include <mach/dove_bl.h>



extern int enable_ebook;

struct dove_backlight {
	int powermode;          /* blacklight */
	int lcd_powermode;      /* lcd panel */
	int current_intensity;
	struct clk *clk;
	void *reg_base;
	struct device *dev;
	struct dovebl_platform_data platform_data;
};


#define DOVEBL_BL_DIV	     0x146	/* 100Hz */

static int dovebl_update_status(struct backlight_device *dev)
{
	struct dove_backlight *bl = dev_get_drvdata(&dev->dev);
	u32 reg;
	u32 bl_val;
	u32 brightness = dev->props.brightness;
	u32 bl_div = DOVEBL_BL_DIV;

	if ((dev->props.power != FB_BLANK_UNBLANK) ||
		(dev->props.fb_blank != FB_BLANK_UNBLANK) ||
		(bl->powermode == FB_BLANK_POWERDOWN)) {
		brightness = 0;
		bl_div = 0;
		bl_val = 0;
	} else {
		/*
		 * brightness = 0 is 1 in the duty cycle in order not to
		 * fully shutdown the backlight.
		 */
		bl_val = ((brightness+1) << 28) | (bl_div << 16);
	}
	
	if (bl->current_intensity != brightness) {
		reg = readl(bl->reg_base + LCD_CFG_GRA_PITCH);
		reg &= 0xFFFF;
		writel(reg | bl_val, bl->reg_base + LCD_CFG_GRA_PITCH);
		bl->current_intensity = brightness;
	}

	return 0;
}


static int dovebl_get_brightness(struct backlight_device *dev)
{
	struct dove_backlight *bl = dev_get_drvdata(&dev->dev);

	return bl->current_intensity;
}


static struct backlight_ops dovebl_ops = {
	.get_brightness = dovebl_get_brightness,
	.update_status  = dovebl_update_status,
};

#ifdef CONFIG_PM
static int dovebl_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct backlight_device *bd = platform_get_drvdata(pdev);
	struct dove_backlight *bl = bl_get_data(bd);

	if (enable_ebook)
		return 0;

	bl->powermode = FB_BLANK_POWERDOWN;
	backlight_update_status(bd);
	clk_disable(bl->clk);

	return 0;
}

static int dovebl_resume(struct platform_device *pdev)
{
	struct backlight_device *bd = platform_get_drvdata(pdev);
	struct dove_backlight *bl = bl_get_data(bd);

	if (enable_ebook)
		return 0;

	clk_enable(bl->clk);

	bl->powermode = FB_BLANK_UNBLANK;
	backlight_update_status(bd);

	return 0;
}
#endif

static int dove_lcd_get_power(struct lcd_device *ld)
{
	struct dove_backlight *bl = lcd_get_data(ld);

	return bl->lcd_powermode;
}

/* Enable or disable power to the LCD (0: on; 4: off, see FB_BLANK_XXX) */
static int dove_lcd_set_power(struct lcd_device *ld, int power)
{
	struct dove_backlight *bl = lcd_get_data(ld);
	unsigned int x, x_bk;

	x = x_bk = readl(bl->reg_base + LCD_SPU_DUMB_CTRL);

	switch (power) {
	case FB_BLANK_UNBLANK:          /* 0 */
		/* Set backlight power on. */
		x |= 0x1 << 20;         /* set bit on */
		x |= 0x1 << 12;         /* set bitmask */
		/* Set LCD panel power on. */
		x |= 0x1 << 21;         /* set bit on */
		x |= 0x1 << 13;         /* set bitmask */
		break;
	case FB_BLANK_POWERDOWN:        /* 4 */
		/* Set power down. */
		/* Set backlight power down. */
		x &= ~(0x1 << 20);      /* set bit on */
		x |= 0x1 << 12;         /* set bitmask */
		/* Set LCD panel power down. */
		x &= ~(0x1 << 21);      /* set bit on */
		x |= 0x1 << 13;         /* set bitmask */
		break;
	case FB_BLANK_NORMAL:
	case FB_BLANK_VSYNC_SUSPEND:
	case FB_BLANK_HSYNC_SUSPEND:
		/* Set backlight power down. */
		x &= ~(0x1 << 20);      /* set bit on */
		x |= 0x1 << 12;         /* set bitmask */
		/* Set LCD panel power on. */
		x |= 0x1 << 21;         /* set bit on */
		x |= 0x1 << 13;         /* set bitmask */
		break;

	default:
		return -EIO;
	}

	bl->lcd_powermode = power;

	if (x != x_bk)
		writel(x, bl->reg_base + LCD_SPU_DUMB_CTRL);
	return 0;
}

static struct lcd_ops dove_lcd_ops = {
	.get_power      = dove_lcd_get_power,
	.set_power      = dove_lcd_set_power,
};

static int dovebl_probe(struct platform_device *pdev)
{
	struct backlight_device *dev;
	struct dove_backlight *bl;
	struct lcd_device *ldp;
	struct dovebl_platform_data *dbm = pdev->dev.platform_data;
	struct resource *res;

	if (!dbm)
		return -ENXIO;

	bl = kzalloc(sizeof(struct dove_backlight), GFP_KERNEL);
	if (bl == NULL)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		kfree(bl);
		return -EINVAL;
	}

	bl->reg_base = ioremap_nocache(res->start, res->end - res->start);
	if (bl->reg_base == NULL) {
		kfree(bl);
		return -ENOMEM;
	}

	dev = backlight_device_register("dove-bl", &pdev->dev, bl, &dovebl_ops);
	if (IS_ERR(dev)) {
		iounmap(bl->reg_base);
		kfree(bl);
		return PTR_ERR(dev);
	}

	if (dbm->gpio_pm_control) {
		ldp = lcd_device_register("dove-lcd", &pdev->dev, bl,
					  &dove_lcd_ops);
		if (IS_ERR(ldp))
			return PTR_ERR(ldp);
	}

	bl->powermode = FB_BLANK_UNBLANK;
	bl->lcd_powermode = FB_BLANK_UNBLANK;
	bl->platform_data = *dbm;
	bl->dev = &pdev->dev;

	platform_set_drvdata(pdev, dev);

	/* Get LCD clock information. */
	bl->clk = clk_get(&pdev->dev, "LCD");

	dev->props.fb_blank = FB_BLANK_UNBLANK;
	dev->props.max_brightness = dbm->max_brightness;
	dev->props.brightness = dbm->default_intensity;
	dovebl_update_status(dev);

	printk(KERN_INFO "Dove Backlight Driver Initialized.\n");
	return 0;
}

static int dovebl_remove(struct platform_device *pdev)
{
	struct backlight_device *dev = platform_get_drvdata(pdev);
	struct dove_backlight *bl = dev_get_drvdata(&dev->dev);

	backlight_device_unregister(dev);
	kfree(bl);

	printk(KERN_INFO "Dove Backlight Driver Unloaded\n");
	return 0;
}

static struct platform_driver dovebl_driver = {
	.probe		= dovebl_probe,
	.remove		= dovebl_remove,
#ifdef CONFIG_PM
	.suspend	= dovebl_suspend,
	.resume		= dovebl_resume,
#endif
	.driver		= {
		.name	= "dove-bl",
	},
};

static int __init dovebl_init(void)
{
	return platform_driver_register(&dovebl_driver);
}

static void __exit dovebl_exit(void)
{
	platform_driver_unregister(&dovebl_driver);
}

module_init(dovebl_init);
module_exit(dovebl_exit);

MODULE_AUTHOR("Shadi Ammouri <shadi@marvell.com>");
MODULE_DESCRIPTION("Dove Backlight Driver");
MODULE_LICENSE("GPL");
