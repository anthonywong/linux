/*
 *  linux/arch/arm/mach-dove/clock.c
 */

/* TODO: Implement the functions below...	*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/clk.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <asm/clkdev.h>

#include <mach/pm.h>
#include <mach/hardware.h>
#include "ctrlEnv/mvCtrlEnvSpec.h"
#include <audio/mvAudioRegs.h>
#include "clock.h"

/* downstream clocks*/
void ds_clks_disable_all(int include_pci0, int include_pci1)
{
	u32 ctrl = readl(CLOCK_GATING_CONTROL);
	
	ctrl &= ~(CLOCK_GATING_USB0_MASK |
		  CLOCK_GATING_USB1_MASK |
		  CLOCK_GATING_GBE_MASK  | CLOCK_GATING_GIGA_PHY_MASK | 
#ifndef CONFIG_MV_HAL_DRIVERS_SUPPORT
		  CLOCK_GATING_SATA_MASK |
#endif
		  /* CLOCK_GATING_PCIE0_MASK | */
		  /* CLOCK_GATING_PCIE1_MASK | */
		  CLOCK_GATING_SDIO0_MASK |
		  CLOCK_GATING_SDIO1_MASK |
		  CLOCK_GATING_NAND_MASK |
		  CLOCK_GATING_CAMERA_MASK |
		  CLOCK_GATING_I2S0_MASK |
		  CLOCK_GATING_I2S1_MASK |
		  /* CLOCK_GATING_CRYPTO_MASK |*/
		  CLOCK_GATING_AC97_MASK |
		  /* CLOCK_GATING_PDMA_MASK |*/
#ifndef CONFIG_MV_HAL_DRIVERS_SUPPORT
		  CLOCK_GATING_XOR0_MASK |
#endif
		  CLOCK_GATING_XOR1_MASK
		);

	if (include_pci0)
		ctrl &= ~CLOCK_GATING_PCIE0_MASK;

	if (include_pci1)
		ctrl &= ~CLOCK_GATING_PCIE1_MASK;

	writel(ctrl, CLOCK_GATING_CONTROL);
}

static void __ds_clk_enable(struct clk *clk)
{
	u32 ctrl;

	if (clk->flags & ALWAYS_ENABLED)
		return;

	ctrl = readl(CLOCK_GATING_CONTROL);
	ctrl |= clk->mask;
	writel(ctrl, CLOCK_GATING_CONTROL);
	return;
}

static void __ds_clk_disable(struct clk *clk)
{
	u32 ctrl;

	if (clk->flags & ALWAYS_ENABLED)
		return;

	ctrl = readl(CLOCK_GATING_CONTROL);
	ctrl &= ~clk->mask;
	writel(ctrl, CLOCK_GATING_CONTROL);
}

const struct clkops ds_clk_ops = {
	.enable		= __ds_clk_enable,
	.disable	= __ds_clk_disable,
};

static void __ac97_clk_enable(struct clk *clk)
{
	u32 reg, ctrl;


	__ds_clk_enable(clk);

	/*
	 * change BPB to use DCO0
	 */
	reg = readl(DOVE_SSP_CTRL_STATUS_1);
	reg &= ~DOVE_SSP_BPB_CLOCK_SRC_SSP;
	writel(reg, DOVE_SSP_CTRL_STATUS_1);

	/* Set DCO clock to 24.576		*/
	/* make sure I2S Audio 0 is not gated off */
	ctrl = readl(CLOCK_GATING_CONTROL);
	if (!(ctrl & CLOCK_GATING_I2S0_MASK))
		writel(ctrl | CLOCK_GATING_I2S0_MASK, CLOCK_GATING_CONTROL);

	/* update the DCO clock frequency */
	reg = readl(DOVE_SB_REGS_VIRT_BASE + MV_AUDIO_DCO_CTRL_REG(0));
	reg = (reg & ~0x3) | 0x2;
	writel(reg, DOVE_SB_REGS_VIRT_BASE + MV_AUDIO_DCO_CTRL_REG(0));

	/* disable back I2S 0 */
	if (!(ctrl & CLOCK_GATING_I2S0_MASK))
		writel(ctrl, CLOCK_GATING_CONTROL);

	return;
}

static void __ac97_clk_disable(struct clk *clk)
{
	u32 ctrl;

	/* 
	 * change BPB to use PLL clock instead of DCO0
	 */
	ctrl = readl(DOVE_SSP_CTRL_STATUS_1);
	ctrl |= DOVE_SSP_BPB_CLOCK_SRC_SSP;
	writel(ctrl, DOVE_SSP_CTRL_STATUS_1);

	__ds_clk_disable(clk);
}

const struct clkops ac97_clk_ops = {
	.enable		= __ac97_clk_enable,
	.disable	= __ac97_clk_disable,
};

/*****************************************************************************
 * GPU and AXI clocks
 ****************************************************************************/
static u32 dove_clocks_get_bits(u32 addr, u32 start_bit, u32 end_bit)
{
	u32 mask;
	u32 value;

	value = readl(addr);
	mask = ((1 << (end_bit + 1 - start_bit)) - 1) << start_bit;
	value = (value & mask) >> start_bit;
	return value;
}

static void dove_clocks_set_bits(u32 addr, u32 start_bit, u32 end_bit,
				 u32 value)
{
	u32 mask;
	u32 new_value;
	u32 old_value;


	old_value = readl(addr);

	mask = ((1 << (end_bit + 1 - start_bit)) -1) << start_bit;
	new_value = old_value & (~mask);
	new_value |= (mask & (value << start_bit));
	writel(new_value, addr);
}

static u32 dove_clocks_divide(u32 dividend, u32 divisor)
{
	u32 result = dividend / divisor;
	u32 r      = dividend % divisor;

	if ((r << 1) >= divisor)
		result++;
	return result;
}

static void dove_clocks_set_gpu_clock(u32 divider)
{
	dove_clocks_set_bits(DOVE_SB_REGS_VIRT_BASE + 0x000D0068, 10, 10, 1);
	dove_clocks_set_bits(DOVE_SB_REGS_VIRT_BASE + 0x000D0064, 8, 13,
			     divider);
	dove_clocks_set_bits(DOVE_SB_REGS_VIRT_BASE + 0x000D0064, 14, 14, 1);
	udelay(1);
	dove_clocks_set_bits(DOVE_SB_REGS_VIRT_BASE + 0x000D0064, 14, 14, 0);
}

#ifndef CONFIG_DOVE_REV_Z0
static void dove_clocks_set_lcd_clock(u32 divider)
{
	dove_clocks_set_bits(DOVE_SB_REGS_VIRT_BASE + 0x000D0068, 10, 10, 1);
	dove_clocks_set_bits(DOVE_SB_REGS_VIRT_BASE + 0x000D0064, 22, 27,
			     divider);
	dove_clocks_set_bits(DOVE_SB_REGS_VIRT_BASE + 0x000D0064, 28, 28, 1);
	udelay(1);
	dove_clocks_set_bits(DOVE_SB_REGS_VIRT_BASE + 0x000D0064, 28, 28, 0);
}
#endif

static void dove_clocks_set_axi_clock(u32 divider)
{
	dove_clocks_set_bits(DOVE_SB_REGS_VIRT_BASE + 0x000D0068, 10, 10, 1);
	dove_clocks_set_bits(DOVE_SB_REGS_VIRT_BASE + 0x000D0064, 1, 6, 
			     divider);
	dove_clocks_set_bits(DOVE_SB_REGS_VIRT_BASE + 0x000D0064, 7, 7, 1);
	udelay(1);
	dove_clocks_set_bits(DOVE_SB_REGS_VIRT_BASE + 0x000D0064, 7, 7, 0);
}

static unsigned long gpu_get_clock(struct clk *clk)
{
	u32 divider;
	u32 c;

	divider = dove_clocks_get_bits(DOVE_SB_REGS_VIRT_BASE + 0x000D0064, 8, 13);
	c = dove_clocks_divide(2000, divider);

	return c * 1000000UL;
}

static int gpu_set_clock(struct clk *clk, unsigned long rate)
{
	u32 divider;

	divider = dove_clocks_divide(2000, rate/1000000);
	printk(KERN_INFO "Setting gpu clock to %lu (divider: %u)\n",
	       rate, divider);
	dove_clocks_set_gpu_clock(divider);
	return 0;
}

#ifndef CONFIG_DOVE_REV_Z0
 int lcd_set_clock(struct clk *clk, unsigned long rate)
{
	u32 divider;

	divider = dove_clocks_divide(2000, rate/1000000);
	printk(KERN_INFO "Setting LCD clock to %lu (divider: %u)\n",
		 rate, divider);
	dove_clocks_set_lcd_clock(divider);
	return 0;
}
#endif
static void __lcd_clk_enable(struct clk *clk)
{
#ifndef CONFIG_DOVE_REV_Z0
	dove_clocks_set_lcd_clock(1);
#endif
	return;
}

static void __lcd_clk_disable(struct clk *clk)
{
#ifndef CONFIG_DOVE_REV_Z0
	dove_clocks_set_lcd_clock(0);
#endif
	return;
}

static unsigned long axi_get_clock(struct clk *clk)
{
	u32 divider;
	u32 c;

	divider = dove_clocks_get_bits(DOVE_SB_REGS_VIRT_BASE + 0x000D0064, 1, 6);
	c = dove_clocks_divide(2000, divider);

        return c * 1000000UL;
}

static int axi_set_clock(struct clk *clk, unsigned long rate)
{
	u32 divider;

	divider = dove_clocks_divide(2000, rate/1000000);
	printk(KERN_INFO "Setting axi clock to %lu (divider: %u)\n",
		 rate, divider);
	dove_clocks_set_axi_clock(divider);
	return 0;
}


static unsigned long ssp_get_clock(struct clk *clk)
{
	u32 divider;
	u32 c;

	divider = dove_clocks_get_bits(DOVE_SSP_CTRL_STATUS_1, 2, 7);
	c = dove_clocks_divide(1000, divider);

	return c * 1000000UL;
}

static int ssp_set_clock(struct clk *clk, unsigned long rate)
{
	u32 divider;

	divider = dove_clocks_divide(1000, rate/1000000);
	printk(KERN_INFO "Setting ssp clock to %lu (divider: %u)\n",
		rate, divider);

	dove_clocks_set_bits(DOVE_SSP_CTRL_STATUS_1, 2, 7, divider);

	return 0;
}


const struct clkops ssp_clk_ops = {
	.getrate	= ssp_get_clock,
	.setrate	= ssp_set_clock,
};





#ifdef CONFIG_SYSFS
static struct platform_device dove_clocks_sysfs = {
	.name		= "dove_clocks_sysfs",
	.id		= 0,
	.num_resources  = 0,
	.resource       = NULL,
};

static ssize_t dove_clocks_axi_show(struct kobject *kobj,
				   struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%lu \n", axi_get_clock(NULL));
}

static ssize_t dove_clocks_axi_store(struct kobject *kobj,
				    struct kobj_attribute *attr,
				    const char *buf, size_t n)
{
	unsigned long value;

	if (sscanf(buf, "%lu", &value) != 1)
		return -EINVAL;
	axi_set_clock(NULL, value);
	return n;
}

static ssize_t dove_clocks_gpu_show(struct kobject *kobj,
				   struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%lu\n", gpu_get_clock(NULL));
}

static ssize_t dove_clocks_gpu_store(struct kobject *kobj,
				struct kobj_attribute *attr,
				const char *buf, size_t n)
{
	unsigned long value;

	if (sscanf(buf, "%lu", &value) != 1)
		return -EINVAL;
	gpu_set_clock(NULL, value);
	return n;
}

static struct kobj_attribute dove_clocks_axi_attr =
	__ATTR(axi, 0644, dove_clocks_axi_show, dove_clocks_axi_store);

static struct kobj_attribute dove_clocks_gpu_attr =
	__ATTR(gpu, 0644, dove_clocks_gpu_show, dove_clocks_gpu_store);

static int __init dove_upstream_clocks_sysfs_setup(void)
{
	platform_device_register(&dove_clocks_sysfs);

	if (sysfs_create_file(&dove_clocks_sysfs.dev.kobj,
			&dove_clocks_axi_attr.attr))
		printk(KERN_ERR "%s: sysfs_create_file failed!", __func__);
	if (sysfs_create_file(&dove_clocks_sysfs.dev.kobj,
			&dove_clocks_gpu_attr.attr))
		printk(KERN_ERR "%s: sysfs_create_file failed!", __func__);

	return 0;
}
#endif
const struct clkops gpu_clk_ops = {
	.getrate	= gpu_get_clock,
	.setrate	= gpu_set_clock,
};

const struct clkops axi_clk_ops = {
	.getrate	= axi_get_clock,
	.setrate	= axi_set_clock,
};

const struct clkops lcd_clk_ops = {
	.enable		= __lcd_clk_enable,
	.disable	= __lcd_clk_disable,
};

int clk_enable(struct clk *clk)
{
	if (clk == NULL || IS_ERR(clk))
		return -EINVAL;

	if (clk->usecount++ == 0)
		if (clk->ops->enable)
			clk->ops->enable(clk);

	return 0;
}
EXPORT_SYMBOL(clk_enable);

void clk_disable(struct clk *clk)
{
	if (clk == NULL || IS_ERR(clk))
		return;

	if (clk->usecount > 0 && !(--clk->usecount))
		if (clk->ops->disable)
			clk->ops->disable(clk);
}
EXPORT_SYMBOL(clk_disable);

unsigned long clk_get_rate(struct clk *clk)
{
	if (clk == NULL || IS_ERR(clk))
		return -EINVAL;

	if (clk->ops->getrate)
		return clk->ops->getrate(clk);

	return *(clk->rate);
}
EXPORT_SYMBOL(clk_get_rate);

long clk_round_rate(struct clk *clk, unsigned long rate)
{
	if (clk == NULL || IS_ERR(clk))
		return -EINVAL;

	return *(clk->rate);
}
EXPORT_SYMBOL(clk_round_rate);

int clk_set_rate(struct clk *clk, unsigned long rate)
{
	if (clk == NULL || IS_ERR(clk))
		return -EINVAL;

	if (clk->ops->setrate)
		return clk->ops->setrate(clk, rate);

	return -EINVAL;
}

EXPORT_SYMBOL(clk_set_rate);

unsigned int  dove_tclk_get(void)
{
	/* tzachi: use DOVE_RESET_SAMPLE_HI/LO to detect tclk
	 * wait for spec, currently use hard code */
	return 166666667;
}

static unsigned long tclk_rate;

static struct clk clk_core = {
	.ops	= &ds_clk_ops,
	.rate	= &tclk_rate,
	.flags	= ALWAYS_ENABLED,
};

static struct clk clk_usb0 = {
	.ops	= &ds_clk_ops,
	.rate	= &tclk_rate,
	.mask	= CLOCK_GATING_USB0_MASK,
};

static struct clk clk_usb1 = {
	.ops	= &ds_clk_ops,
	.rate	= &tclk_rate,
	.mask	= CLOCK_GATING_USB1_MASK,
};

static struct clk clk_gbe = {
	.ops	= &ds_clk_ops,
	.rate	= &tclk_rate,
	.mask	= CLOCK_GATING_GBE_MASK | CLOCK_GATING_GIGA_PHY_MASK,
};

static struct clk clk_sata = {
	.ops	= &ds_clk_ops,
	.rate	= &tclk_rate,
	.mask	= CLOCK_GATING_SATA_MASK,
};

static struct clk clk_pcie0 = {
	.ops	= &ds_clk_ops,
	.rate	= &tclk_rate,
	.mask	= CLOCK_GATING_PCIE0_MASK,
};

static struct clk clk_pcie1 = {
	.ops	= &ds_clk_ops,
	.rate	= &tclk_rate,
	.mask	= CLOCK_GATING_PCIE1_MASK,
};

static struct clk clk_sdio0 = {
	.ops	= &ds_clk_ops,
	.rate	= &tclk_rate,
	.mask	= CLOCK_GATING_SDIO0_MASK,
};

static struct clk clk_sdio1 = {
	.ops	= &ds_clk_ops,
	.rate	= &tclk_rate,
	.mask	= CLOCK_GATING_SDIO1_MASK,
};

static struct clk clk_nand = {
	.ops	= &ds_clk_ops,
	.rate	= &tclk_rate,
	.mask	= CLOCK_GATING_NAND_MASK,
};

static struct clk clk_camera = {
	.ops	= &ds_clk_ops,
	.rate	= &tclk_rate,
	.mask	= CLOCK_GATING_CAMERA_MASK,
};

static struct clk clk_i2s0 = {
	.ops	= &ds_clk_ops,
	.rate	= &tclk_rate,
	.mask	= CLOCK_GATING_I2S0_MASK,
};

static struct clk clk_i2s1 = {
	.ops	= &ds_clk_ops,
	.rate	= &tclk_rate,
	.mask	= CLOCK_GATING_I2S1_MASK,
};

static struct clk clk_crypto = {
	.ops	= &ds_clk_ops,
	.rate	= &tclk_rate,
	.mask	= CLOCK_GATING_CRYPTO_MASK,
};

static struct clk clk_ac97 = {
	.ops	= &ac97_clk_ops,
	.rate	= &tclk_rate,
	.mask	= CLOCK_GATING_AC97_MASK,
};

static struct clk clk_pdma = {
	.ops	= &ds_clk_ops,
	.rate	= &tclk_rate,
	.mask	= CLOCK_GATING_PDMA_MASK,
};

static struct clk clk_xor0 = {
	.ops	= &ds_clk_ops,
	.rate	= &tclk_rate,
	.mask	= CLOCK_GATING_XOR0_MASK,
};

static struct clk clk_xor1 = {
	.ops	= &ds_clk_ops,
	.rate	= &tclk_rate,
	.mask	= CLOCK_GATING_XOR1_MASK,
};

//static struct clk clk_giga_phy = {
//	.rate	= &tclk_rate,
//	.mask	= CLOCK_GATING_GIGA_PHY_MASK,
//};

static struct clk clk_gpu = {
	.ops	= &gpu_clk_ops,
};

static struct clk clk_axi = {
	.ops	= &axi_clk_ops,
};

static struct clk clk_ssp = {
	.ops	= &ssp_clk_ops,
};

static struct clk clk_lcd = {
	.ops	= &lcd_clk_ops,
};

#define INIT_CK(dev,con,ck)			\
	{ .dev_id = dev, .con_id = con, .clk = ck }

static struct clk_lookup dove_clocks[] = {
	INIT_CK(NULL, "tclk", &clk_core),
	INIT_CK("orion-ehci.0", NULL, &clk_usb0),
	INIT_CK("orion-ehci.1", NULL, &clk_usb1),
	INIT_CK(NULL, "usb0", &clk_usb0), /* for udc device mode */
	INIT_CK(NULL, "usb1", &clk_usb1), /* for udc device mode */
	INIT_CK("mv_netdev.0", NULL, &clk_gbe),
	INIT_CK("mv643xx_eth.0", NULL, &clk_gbe),
	INIT_CK("sata_mv.0", NULL, &clk_sata),
	INIT_CK(NULL, "PCI0", &clk_pcie0),
	INIT_CK(NULL, "PCI1", &clk_pcie1),
	INIT_CK("sdhci-mv.0", NULL, &clk_sdio0),
	INIT_CK("sdhci-mv.1", NULL, &clk_sdio1),
	INIT_CK("dove-nand", NULL, &clk_nand),
	INIT_CK("cafe1000-ccic.0", NULL, &clk_camera),
	INIT_CK("mv88fx_snd.0", NULL, &clk_i2s0),
	INIT_CK("mv88fx_snd.1", NULL, &clk_i2s1),
	INIT_CK("crypto", NULL, &clk_crypto),
	INIT_CK(NULL, "AC97CLK", &clk_ac97),
	INIT_CK(NULL, "PDMA", &clk_pdma),
	INIT_CK("mv_xor_shared.0", NULL, &clk_xor0),
	INIT_CK("mv_xor_shared.1", NULL, &clk_xor1),
//	INIT_CK(NULL, "GIGA_PHY", &clk_giga_phy),
	INIT_CK(NULL, "GCCLK", &clk_gpu),
	INIT_CK(NULL, "AXICLK", &clk_axi),
	INIT_CK(NULL, "LCDCLK", &clk_lcd),
	INIT_CK(NULL, "ssp", &clk_ssp),
};

int __init dove_clk_config(struct device *dev, const char *id, unsigned long rate)
{
	struct clk *clk;
	int ret = 0;

	clk = clk_get(dev, id);
	if (IS_ERR(clk)) {
		printk(KERN_ERR "failed to get clk %s\n", dev ? dev_name(dev):
		       id);
		return PTR_ERR(clk);
	}
	ret = clk_set_rate(clk, rate); 
	if (ret < 0) 
		printk(KERN_ERR "failed to set %s clk to %lu \n", 
		       dev?dev_name(dev):id, rate);
	return ret;
}

int __init dove_devclks_init(void)
{
	int i;
	
	tclk_rate = dove_tclk_get();

	/* disable the clocks of all peripherals */
	//__clks_disable_all();

	for (i = 0; i < ARRAY_SIZE(dove_clocks); i++)
                clkdev_add(&dove_clocks[i]);

#ifdef CONFIG_SYSFS
	dove_upstream_clocks_sysfs_setup();
#endif
#ifdef CONFIG_PM
	/* ask the pm to save & restore this register */
	pm_registers_add_single(CLOCK_GATING_CONTROL);
#endif
//	__clk_disable(&clk_usb0);
//	__clk_disable(&clk_usb1);
//	__clk_disable(&clk_ac97);
	return 0;
}
