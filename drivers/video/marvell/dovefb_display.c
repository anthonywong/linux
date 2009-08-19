#include <linux/module.h>

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/smp_lock.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/vt.h>
#include <linux/init.h>
#include <linux/linux_logo.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/console.h>
#include <linux/kmod.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/efi.h>
#include <linux/fb.h>

#include <video/dovefb.h>
#include <video/dovefbreg.h>
#include <video/dovedcon.h>
#include <video/dovefb_display.h>

/*
 * LCD controll register physical address
 */
static unsigned int lcd_regbase;
//module_param(lcd_regbase, uint, 0xf1810000);
module_param(lcd_regbase, uint, 0);
MODULE_PARM_DESC(lcd_regbase, "LCD controller register base");

/*
 * 0 means lcd0 = regbase + 0x0, lcd1 = regbase + 0x10000;
 * 1 means lcd0 = regbase + 0x10000, lcd1 = regbase + 0x0;
 */
static unsigned int lcdseq;
module_param(lcdseq, uint, 1);
MODULE_PARM_DESC(lcdseq, "LCD sequence");

extern struct display_settings lcd_config;
extern struct class *fb_class;
static void *lcd0_regbase;
static void *lcd1_regbase;
static void *dcon_regbase;

static void set_graphics_start(struct fb_info *fi, int xoffset, int yoffset,
	struct fb_info *newfi)
{
	struct dovefb_layer_info *dfli = fi->par;
	struct fb_var_screeninfo *var = &fi->var;
	int pixel_offset;
	unsigned long addr;
	unsigned int x;

	if (newfi) {
		fi->var.xres_virtual = newfi->var.xres_virtual;

		x = readl(dfli->reg_base + LCD_CFG_GRA_PITCH);
		x = (x & ~0xFFFF) | ((var->xres_virtual * var->bits_per_pixel) >> 3);
		writel(x, dfli->reg_base + LCD_CFG_GRA_PITCH);
	}
	pixel_offset = (yoffset * var->xres_virtual) + xoffset;

	if (newfi) {
		struct dovefb_layer_info *newdfli = newfi->par;
		addr = newdfli->fb_start_dma + (pixel_offset * (var->bits_per_pixel >> 3));
	} else
		addr = dfli->fb_start_dma + (pixel_offset * (var->bits_per_pixel >> 3));

	writel(addr, dfli->reg_base + LCD_CFG_GRA_START_ADDR0);
}

static void dcon_portb(uint mode)
{
	unsigned int ctrl0;

	/* enable lcd0 pass to PortB */
	ctrl0 = readl(dcon_regbase+DCON_CTRL0);
	ctrl0 &= ~(0x3 << 8);
	ctrl0 |= (mode << 8);
	writel(ctrl0, dcon_regbase+DCON_CTRL0);

}

static void set_back(void)
{
	//uint x;
	struct dovefb_layer_info *dfli0_gfx = lcd_config.lcd0_gfx->par;
	//struct dovefb_layer_info *dfli0_vid = lcd_config.lcd0_vid->par;
	struct dovefb_layer_info *dfli1_gfx = lcd_config.lcd1_gfx->par;
	//struct dovefb_layer_info *dfli1_vid = lcd_config.lcd1_vid->par;

	struct fb_var_screeninfo *var0_gfx = &lcd_config.lcd0_gfx->var;
	//struct fb_var_screeninfo *var0_vid = &lcd_config.lcd0_vid->var;
	struct fb_var_screeninfo *var1_gfx = &lcd_config.lcd1_gfx->var;
	//struct fb_var_screeninfo *var1_vid = &lcd_config.lcd1_vid->var;

	/* set lcd0 src scan line */
	writel((var0_gfx->yres << 16) | (var0_gfx->xres),
		dfli0_gfx->reg_base + LCD_SPU_GRA_HPXL_VLN);

	/* set lcd1 src scan line */
	writel((var1_gfx->yres << 16) | (var1_gfx->xres),
		dfli1_gfx->reg_base + LCD_SPU_GRA_HPXL_VLN);
	
	/* set lcd1 refresh whole area. */
	set_graphics_start(lcd_config.lcd1_gfx,
		var1_gfx->xoffset,
		var1_gfx->yoffset,
		lcd_config.lcd0_gfx);
}

static int setup_display(struct display_settings *config)
{
	//uint x;
	//struct dovefb_layer_info *dfli0_gfx = lcd_config.lcd0_gfx->par;
	//struct dovefb_layer_info *dfli0_vid = lcd_config.lcd0_vid->par;
	//struct dovefb_layer_info *dfli1_gfx = lcd_config.lcd1_gfx->par;
	//struct dovefb_layer_info *dfli1_vid = lcd_config.lcd1_vid->par;

	//struct fb_var_screeninfo *var0_gfx = &lcd_config.lcd0_gfx->var;
	//struct fb_var_screeninfo *var0_vid = &lcd_config.lcd0_vid->var;
	struct fb_var_screeninfo *var1_gfx = &lcd_config.lcd1_gfx->var;
	//struct fb_var_screeninfo *var1_vid = &lcd_config.lcd1_vid->var;

	if (!config)
		return -1;

	switch (config->display_mode) {
	case DISPLAY_EXTENDED:
		dcon_portb(0);
		printk(KERN_INFO "configure to EXTENDED Mode\n");
		/* set lcd0 src scan line */
//		writel((var0_gfx->yres << 16) | (var0_gfx->xres*lcd_config.extend_ratio/4),
//				dfli0_gfx->reg_base + LCD_SPU_GRA_HPXL_VLN);

		/* set lcd1 src scan line */
//		writel((var1_gfx->yres << 16) | (var1_gfx->xres*lcd_config.extend_ratio/4),
//				dfli1_gfx->reg_base + LCD_SPU_GRA_HPXL_VLN);
	
		/* set lcd1 refresh second half portion. */
		set_graphics_start(lcd_config.lcd1_gfx,
			var1_gfx->xoffset+(var1_gfx->xres),
			var1_gfx->yoffset, lcd_config.lcd0_gfx);
		break;
	case DISPLAY_NORMAL:
		printk(KERN_INFO "configure to NORMAL Mode\n");
		dcon_portb(0);
		set_back();
		set_graphics_start(lcd_config.lcd1_gfx,
			var1_gfx->xoffset,
			var1_gfx->yoffset, 0);
	
		/* switch lcd1's buffer addr back. */
		break;
	case DISPLAY_CLONE:
		printk(KERN_INFO "configure to CLONE Mode\n");
		dcon_portb(1);
		set_back();
#if 0 // test code, set lcd0 to dump16bit mode.
#endif
		break;
	case DISPLAY_DUALVIEW:
		printk(KERN_INFO "configure to DUALVIEW Mode\n");
		dcon_portb(0);
		set_back();
		break;
	default:
		;
	}


	return 0;
}

static long display_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct display_settings config;
	switch (cmd) {
	case 0:	/* configure display mode. */
		printk(KERN_INFO "driver set display config.\n");

		/* Get data from user space. */
		if (copy_from_user(&config, (void *)arg, sizeof(struct display_settings)))
			return -EFAULT;

		lcd_config.display_mode = config.display_mode;
		lcd_config.extend_ratio = config.extend_ratio;

#ifdef MTL_DEBUG
		printk("Get mode = %d\n", lcd_config.display_mode);
		printk("Get ratio = %d\n", lcd_config.extend_ratio);
#endif
		/*
		 * set up lcd0
		 */
		//printk(KERN_INFO "case 0 .... driver set display config.\n");
		setup_display(&lcd_config);
		break;
	case 1:
		/* get display configuration. */
		printk(KERN_INFO "get display config.\n");
		if (copy_to_user((void *)arg, &lcd_config, sizeof(struct display_settings)))
			return -EFAULT;
		break;
	default:
		printk(KERN_ERR "Unknown command\n");
	}

	return 0;
}

static const struct file_operations display_fops = {
	.owner =	THIS_MODULE,
	.unlocked_ioctl = display_ioctl,
};


static int __init
dovefb_display_init(void)
{
#ifdef MTL_DEBUG
	uint x;

	printk(KERN_INFO "lcd_regbase = 0x%08x\n", lcd_regbase);
	printk(KERN_INFO "lcdseq = %d, 1^lcdseq = %d\n", lcdseq, 1^lcdseq);
	printk(KERN_INFO "lcdseq = %d, 0^lcdseq = %d\n", lcdseq, 0^lcdseq);
#endif
	printk(KERN_WARNING "dovefb_display_init\n");

	/* register character. */
	if (register_chrdev(30, "display tools", &display_fops))
		printk("unable to get major %d for fb devs\n", 30);

	if (lcd_regbase) {
		/* remap to ctrl registers. */
		lcd0_regbase = ioremap_nocache( lcd_regbase + (0x10000*(0^lcdseq)), (0x10000 - 1));
		lcd1_regbase = ioremap_nocache( lcd_regbase + (0x10000*(1^lcdseq)), (0x10000 - 1));
		dcon_regbase = ioremap_nocache( lcd_regbase + 0x20000,		    (0x10000 - 1));
#ifdef MTL_DEBUG
		x = readl( lcd0_regbase + 0x104 );
		printk(KERN_INFO "debug lcd0 reg 0x104 = 0x%08x\n", x);
		x = readl( lcd1_regbase + 0x108 );
		printk(KERN_INFO "debug lcd0 reg 0x104 = 0x%08x\n", x);
		x = readl( dcon_regbase + 0x000 );
		printk(KERN_INFO "debug dcon reg 0x000 = 0x%08x\n", x);
#endif
	}

	printk(KERN_WARNING "dovefb_display driver init ok.\n");
	return 0;
}

static void __exit
dovefb_display_exit(void)
{
	iounmap(lcd0_regbase);
	iounmap(lcd1_regbase);
	iounmap(dcon_regbase);
	unregister_chrdev(30, "display tools");
	printk(KERN_WARNING "dovefb_display driver unload OK.\n");
}

module_init(dovefb_display_init);
module_exit(dovefb_display_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Display mode driver");

