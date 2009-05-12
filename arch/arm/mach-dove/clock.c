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

#include "clock.h"

void clks_disable_all(int include_pci0, int include_pci1)
{
	u32 ctrl = readl(CLOCK_GATING_CONTROL);
	
	ctrl &= ~(CLOCK_GATING_USB0_MASK |
		  CLOCK_GATING_USB1_MASK |
		  /* CLOCK_GATING_GBE_MASK |*/
		  CLOCK_GATING_SATA_MASK |
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
		  CLOCK_GATING_XOR0_MASK |
		  CLOCK_GATING_XOR1_MASK
		);

	if (include_pci0)
		ctrl &= ~CLOCK_GATING_PCIE0_MASK;

	if (include_pci1)
		ctrl &= ~CLOCK_GATING_PCIE1_MASK;

	writel(ctrl, CLOCK_GATING_CONTROL);
}

static int __clk_enable(struct clk *clk)
{
	u32 ctrl;

	if (clk->flags & ALWAYS_ENABLED)
		return 0;
	
	ctrl = readl(CLOCK_GATING_CONTROL);
	ctrl |= clk->mask;
	writel(ctrl, CLOCK_GATING_CONTROL);
	return 0;
}

static void __clk_disable(struct clk *clk)
{
	u32 ctrl;

	if (clk->flags & ALWAYS_ENABLED)
		return;

	ctrl = readl(CLOCK_GATING_CONTROL);
	ctrl &= ~clk->mask;
	writel(ctrl, CLOCK_GATING_CONTROL);
}

int clk_enable(struct clk *clk)
{
	int ret = 0;
	
	if (clk == NULL || IS_ERR(clk))
		return -EINVAL;

	if (clk->usecount++ == 0)
		ret = __clk_enable(clk);

	return ret;
}
EXPORT_SYMBOL(clk_enable);

void clk_disable(struct clk *clk)
{
	if (clk == NULL || IS_ERR(clk))
		return;

	if (clk->usecount > 0 && !(--clk->usecount))
		__clk_disable(clk);
}
EXPORT_SYMBOL(clk_disable);

unsigned long clk_get_rate(struct clk *clk)
{
	if (clk == NULL || IS_ERR(clk))
		return -EINVAL;

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

	/* changing the clk rate is not supported */
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
	.rate	= &tclk_rate,
	.flags	= ALWAYS_ENABLED,
};

static struct clk clk_usb0 = {
	.rate	= &tclk_rate,
	.mask	= CLOCK_GATING_USB0_MASK,
};

static struct clk clk_usb1 = {
	.rate	= &tclk_rate,
	.mask	= CLOCK_GATING_USB1_MASK,
};

static struct clk clk_gbe = {
	.rate	= &tclk_rate,
	.mask	= CLOCK_GATING_GBE_MASK | CLOCK_GATING_GIGA_PHY_MASK,
};

static struct clk clk_sata = {
	.rate	= &tclk_rate,
	.mask	= CLOCK_GATING_SATA_MASK,
};

static struct clk clk_pcie0 = {
	.rate	= &tclk_rate,
	.mask	= CLOCK_GATING_PCIE0_MASK,
};

static struct clk clk_pcie1 = {
	.rate	= &tclk_rate,
	.mask	= CLOCK_GATING_PCIE1_MASK,
};

static struct clk clk_sdio0 = {
	.rate	= &tclk_rate,
	.mask	= CLOCK_GATING_SDIO0_MASK,
};

static struct clk clk_sdio1 = {
	.rate	= &tclk_rate,
	.mask	= CLOCK_GATING_SDIO1_MASK,
};

static struct clk clk_nand = {
	.rate	= &tclk_rate,
	.mask	= CLOCK_GATING_NAND_MASK,
};

static struct clk clk_camera = {
	.rate	= &tclk_rate,
	.mask	= CLOCK_GATING_CAMERA_MASK,
};

static struct clk clk_i2s0 = {
	.rate	= &tclk_rate,
	.mask	= CLOCK_GATING_I2S0_MASK,
};

static struct clk clk_i2s1 = {
	.rate	= &tclk_rate,
	.mask	= CLOCK_GATING_I2S1_MASK,
};

static struct clk clk_crypto = {
	.rate	= &tclk_rate,
	.mask	= CLOCK_GATING_CRYPTO_MASK,
};

static struct clk clk_ac97 = {
	.rate	= &tclk_rate,
	.mask	= CLOCK_GATING_AC97_MASK,
};

static struct clk clk_pdma = {
	.rate	= &tclk_rate,
	.mask	= CLOCK_GATING_PDMA_MASK,
};

static struct clk clk_xor0 = {
	.rate	= &tclk_rate,
	.mask	= CLOCK_GATING_XOR0_MASK,
};

static struct clk clk_xor1 = {
	.rate	= &tclk_rate,
	.mask	= CLOCK_GATING_XOR1_MASK,
};

//static struct clk clk_giga_phy = {
//	.rate	= &tclk_rate,
//	.mask	= CLOCK_GATING_GIGA_PHY_MASK,
//};

#define INIT_CK(dev,con,ck)			\
	{ .dev_id = dev, .con_id = con, .clk = ck }

static struct clk_lookup dove_clocks[] = {
	INIT_CK(NULL, "tclk", &clk_core),
	INIT_CK("orion-ehci.0", NULL, &clk_usb0),
	INIT_CK("orion-ehci.1", NULL, &clk_usb1),
	INIT_CK("mv_netdev.0", NULL, &clk_gbe),
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
};

int __init dove_devclks_init(void)
{
	int i;

	tclk_rate = dove_tclk_get();

	/* disable the clocks of all peripherals */
	//__clks_disable_all();

	for (i = 0; i < ARRAY_SIZE(dove_clocks); i++)
                clkdev_add(&dove_clocks[i]);

//	__clk_disable(&clk_usb0);
//	__clk_disable(&clk_usb1);
//	__clk_disable(&clk_ac97);
	return 0;
}
