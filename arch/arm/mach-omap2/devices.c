/*
 * linux/arch/arm/mach-omap2/devices.c
 *
 * OMAP2 platform device setup/initialization
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/err.h>

#include <mach/hardware.h>
#include <asm/mach-types.h>
#include <asm/mach/map.h>

#include <plat/control.h>
#include <plat/tc.h>
#include <plat/board.h>
#include <plat/mux.h>
#include <mach/gpio.h>
#include <plat/mmc.h>

#include "mux.h"

#include <plat/omap_device.h>
#include <plat/omap_hwmod.h>

#if defined(CONFIG_VIDEO_OMAP2) || defined(CONFIG_VIDEO_OMAP2_MODULE)

static struct resource cam_resources[] = {
	{
		.start		= OMAP24XX_CAMERA_BASE,
		.end		= OMAP24XX_CAMERA_BASE + 0xfff,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= INT_24XX_CAM_IRQ,
		.flags		= IORESOURCE_IRQ,
	}
};

static struct platform_device omap_cam_device = {
	.name		= "omap24xxcam",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(cam_resources),
	.resource	= cam_resources,
};

static inline void omap_init_camera(void)
{
	platform_device_register(&omap_cam_device);
}

#elif defined(CONFIG_VIDEO_OMAP3) || defined(CONFIG_VIDEO_OMAP3_MODULE)

static struct resource omap3isp_resources[] = {
	{
		.start		= OMAP3430_ISP_BASE,
		.end		= OMAP3430_ISP_END,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= OMAP3430_ISP_CBUFF_BASE,
		.end		= OMAP3430_ISP_CBUFF_END,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= OMAP3430_ISP_CCP2_BASE,
		.end		= OMAP3430_ISP_CCP2_END,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= OMAP3430_ISP_CCDC_BASE,
		.end		= OMAP3430_ISP_CCDC_END,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= OMAP3430_ISP_HIST_BASE,
		.end		= OMAP3430_ISP_HIST_END,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= OMAP3430_ISP_H3A_BASE,
		.end		= OMAP3430_ISP_H3A_END,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= OMAP3430_ISP_PREV_BASE,
		.end		= OMAP3430_ISP_PREV_END,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= OMAP3430_ISP_RESZ_BASE,
		.end		= OMAP3430_ISP_RESZ_END,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= OMAP3430_ISP_SBL_BASE,
		.end		= OMAP3430_ISP_SBL_END,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= OMAP3430_ISP_CSI2A_BASE,
		.end		= OMAP3430_ISP_CSI2A_END,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= OMAP3430_ISP_CSI2PHY_BASE,
		.end		= OMAP3430_ISP_CSI2PHY_END,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= INT_34XX_CAM_IRQ,
		.flags		= IORESOURCE_IRQ,
	}
};

static struct platform_device omap3isp_device = {
	.name		= "omap3isp",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(omap3isp_resources),
	.resource	= omap3isp_resources,
};

static inline void omap_init_camera(void)
{
	platform_device_register(&omap3isp_device);
}
#else
static inline void omap_init_camera(void)
{
}
#endif

#if defined(CONFIG_OMAP_MBOX_FWK) || defined(CONFIG_OMAP_MBOX_FWK_MODULE)

#define MBOX_REG_SIZE   0x120

#ifdef CONFIG_ARCH_OMAP2
static struct resource omap2_mbox_resources[] = {
	{
		.start		= OMAP24XX_MAILBOX_BASE,
		.end		= OMAP24XX_MAILBOX_BASE + MBOX_REG_SIZE - 1,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= INT_24XX_MAIL_U0_MPU,
		.flags		= IORESOURCE_IRQ,
	},
	{
		.start		= INT_24XX_MAIL_U3_MPU,
		.flags		= IORESOURCE_IRQ,
	},
};
static int omap2_mbox_resources_sz = ARRAY_SIZE(omap2_mbox_resources);
#else
#define omap2_mbox_resources		NULL
#define omap2_mbox_resources_sz		0
#endif

#ifdef CONFIG_ARCH_OMAP3
static struct resource omap3_mbox_resources[] = {
	{
		.start		= OMAP34XX_MAILBOX_BASE,
		.end		= OMAP34XX_MAILBOX_BASE + MBOX_REG_SIZE - 1,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= INT_24XX_MAIL_U0_MPU,
		.flags		= IORESOURCE_IRQ,
	},
};
static int omap3_mbox_resources_sz = ARRAY_SIZE(omap3_mbox_resources);
#else
#define omap3_mbox_resources		NULL
#define omap3_mbox_resources_sz		0
#endif

#ifdef CONFIG_ARCH_OMAP4

#define OMAP4_MBOX_REG_SIZE	0x130
static struct resource omap4_mbox_resources[] = {
	{
		.start          = OMAP44XX_MAILBOX_BASE,
		.end            = OMAP44XX_MAILBOX_BASE +
					OMAP4_MBOX_REG_SIZE - 1,
		.flags          = IORESOURCE_MEM,
	},
	{
		.start          = OMAP44XX_IRQ_MAIL_U0,
		.flags          = IORESOURCE_IRQ,
	},
};
static int omap4_mbox_resources_sz = ARRAY_SIZE(omap4_mbox_resources);
#else
#define omap4_mbox_resources		NULL
#define omap4_mbox_resources_sz		0
#endif

static struct platform_device mbox_device = {
	.name		= "omap2-mailbox",
	.id		= -1,
};

static inline void omap_init_mbox(void)
{
	if (cpu_is_omap24xx()) {
		mbox_device.resource = omap2_mbox_resources;
		mbox_device.num_resources = omap2_mbox_resources_sz;
	} else if (cpu_is_omap34xx()) {
		mbox_device.resource = omap3_mbox_resources;
		mbox_device.num_resources = omap3_mbox_resources_sz;
	} else if (cpu_is_omap44xx()) {
		mbox_device.resource = omap4_mbox_resources;
		mbox_device.num_resources = omap4_mbox_resources_sz;
	} else {
		pr_err("%s: platform not supported\n", __func__);
		return;
	}
	platform_device_register(&mbox_device);
}
#else
static inline void omap_init_mbox(void) { }
#endif /* CONFIG_OMAP_MBOX_FWK */

#if defined(CONFIG_OMAP_STI)

#if defined(CONFIG_ARCH_OMAP2)

#define OMAP2_STI_BASE		0x48068000
#define OMAP2_STI_CHANNEL_BASE	0x54000000
#define OMAP2_STI_IRQ		4

static struct resource sti_resources[] = {
	{
		.start		= OMAP2_STI_BASE,
		.end		= OMAP2_STI_BASE + 0x7ff,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= OMAP2_STI_CHANNEL_BASE,
		.end		= OMAP2_STI_CHANNEL_BASE + SZ_64K - 1,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= OMAP2_STI_IRQ,
		.flags		= IORESOURCE_IRQ,
	}
};
#elif defined(CONFIG_ARCH_OMAP3)

#define OMAP3_SDTI_BASE		0x54500000
#define OMAP3_SDTI_CHANNEL_BASE	0x54600000

static struct resource sti_resources[] = {
	{
		.start		= OMAP3_SDTI_BASE,
		.end		= OMAP3_SDTI_BASE + 0xFFF,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= OMAP3_SDTI_CHANNEL_BASE,
		.end		= OMAP3_SDTI_CHANNEL_BASE + SZ_1M - 1,
		.flags		= IORESOURCE_MEM,
	}
};

#endif

static struct platform_device sti_device = {
	.name		= "sti",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(sti_resources),
	.resource	= sti_resources,
};

static inline void omap_init_sti(void)
{
	platform_device_register(&sti_device);
}
#else
static inline void omap_init_sti(void) {}
#endif

#if defined(CONFIG_SPI_OMAP24XX) || defined(CONFIG_SPI_OMAP24XX_MODULE)

#include <plat/mcspi.h>

#define OMAP2_MCSPI1_BASE		0x48098000
#define OMAP2_MCSPI2_BASE		0x4809a000
#define OMAP2_MCSPI3_BASE		0x480b8000
#define OMAP2_MCSPI4_BASE		0x480ba000

#define OMAP4_MCSPI1_BASE		0x48098100
#define OMAP4_MCSPI2_BASE		0x4809a100
#define OMAP4_MCSPI3_BASE		0x480b8100
#define OMAP4_MCSPI4_BASE		0x480ba100

static struct omap2_mcspi_platform_config omap2_mcspi1_config = {
	.num_cs		= 4,
	.force_cs_mode	= 1,
};

static struct resource omap2_mcspi1_resources[] = {
	{
		.start		= OMAP2_MCSPI1_BASE,
		.end		= OMAP2_MCSPI1_BASE + 0xff,
		.flags		= IORESOURCE_MEM,
	},
};

static struct platform_device omap2_mcspi1 = {
	.name		= "omap2_mcspi",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(omap2_mcspi1_resources),
	.resource	= omap2_mcspi1_resources,
	.dev		= {
		.platform_data = &omap2_mcspi1_config,
	},
};

static struct omap2_mcspi_platform_config omap2_mcspi2_config = {
	.num_cs		= 2,
	.mode		= OMAP2_MCSPI_MASTER,
	.dma_mode	= 1,
	.force_cs_mode	= 0,
	.fifo_depth	= 0,
};

static struct resource omap2_mcspi2_resources[] = {
	{
		.start		= OMAP2_MCSPI2_BASE,
		.end		= OMAP2_MCSPI2_BASE + 0xff,
		.flags		= IORESOURCE_MEM,
	},
};

static struct platform_device omap2_mcspi2 = {
	.name		= "omap2_mcspi",
	.id		= 2,
	.num_resources	= ARRAY_SIZE(omap2_mcspi2_resources),
	.resource	= omap2_mcspi2_resources,
	.dev		= {
		.platform_data = &omap2_mcspi2_config,
	},
};

#if defined(CONFIG_ARCH_OMAP2430) || defined(CONFIG_ARCH_OMAP3) || \
	defined(CONFIG_ARCH_OMAP4)
static struct omap2_mcspi_platform_config omap2_mcspi3_config = {
	.num_cs		= 2,
};

static struct resource omap2_mcspi3_resources[] = {
	{
	.start		= OMAP2_MCSPI3_BASE,
	.end		= OMAP2_MCSPI3_BASE + 0xff,
	.flags		= IORESOURCE_MEM,
	},
};

static struct platform_device omap2_mcspi3 = {
	.name		= "omap2_mcspi",
	.id		= 3,
	.num_resources	= ARRAY_SIZE(omap2_mcspi3_resources),
	.resource	= omap2_mcspi3_resources,
	.dev		= {
		.platform_data = &omap2_mcspi3_config,
	},
};
#endif

#if defined(CONFIG_ARCH_OMAP3) || defined(CONFIG_ARCH_OMAP4)
static struct omap2_mcspi_platform_config omap2_mcspi4_config = {
	.num_cs		= 1,
};

static struct resource omap2_mcspi4_resources[] = {
	{
		.start		= OMAP2_MCSPI4_BASE,
		.end		= OMAP2_MCSPI4_BASE + 0xff,
		.flags		= IORESOURCE_MEM,
	},
};

static struct platform_device omap2_mcspi4 = {
	.name		= "omap2_mcspi",
	.id		= 4,
	.num_resources	= ARRAY_SIZE(omap2_mcspi4_resources),
	.resource	= omap2_mcspi4_resources,
	.dev		= {
		.platform_data = &omap2_mcspi4_config,
	},
};
#endif

#ifdef CONFIG_ARCH_OMAP4
static inline void omap4_mcspi_fixup(void)
{
	omap2_mcspi1_resources[0].start	= OMAP4_MCSPI1_BASE;
	omap2_mcspi1_resources[0].end	= OMAP4_MCSPI1_BASE + 0xff;
	omap2_mcspi2_resources[0].start	= OMAP4_MCSPI2_BASE;
	omap2_mcspi2_resources[0].end	= OMAP4_MCSPI2_BASE + 0xff;
	omap2_mcspi3_resources[0].start	= OMAP4_MCSPI3_BASE;
	omap2_mcspi3_resources[0].end	= OMAP4_MCSPI3_BASE + 0xff;
	omap2_mcspi4_resources[0].start	= OMAP4_MCSPI4_BASE;
	omap2_mcspi4_resources[0].end	= OMAP4_MCSPI4_BASE + 0xff;
}
#else
static inline void omap4_mcspi_fixup(void)
{
}
#endif

#if defined(CONFIG_ARCH_OMAP2430) || defined(CONFIG_ARCH_OMAP3) || \
	defined(CONFIG_ARCH_OMAP4)
static inline void omap2_mcspi3_init(void)
{
	platform_device_register(&omap2_mcspi3);
}
#else
static inline void omap2_mcspi3_init(void)
{
}
#endif

#if defined(CONFIG_ARCH_OMAP3) || defined(CONFIG_ARCH_OMAP4)
static inline void omap2_mcspi4_init(void)
{
	platform_device_register(&omap2_mcspi4);
}
#else
static inline void omap2_mcspi4_init(void)
{
}
#endif

static void omap_init_mcspi(void)
{
	if (cpu_is_omap44xx())
		omap4_mcspi_fixup();

	platform_device_register(&omap2_mcspi1);
	platform_device_register(&omap2_mcspi2);

	if (cpu_is_omap2430() || cpu_is_omap343x() || cpu_is_omap44xx())
		omap2_mcspi3_init();

	if (cpu_is_omap343x() || cpu_is_omap44xx())
		omap2_mcspi4_init();
}

#else
static inline void omap_init_mcspi(void) {}
#endif

#ifdef CONFIG_OMAP_SHA1_MD5
static struct resource sha1_md5_resources[] = {
	{
		.start	= OMAP24XX_SEC_SHA1MD5_BASE,
		.end	= OMAP24XX_SEC_SHA1MD5_BASE + 0x64,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= INT_24XX_SHA1MD5,
		.flags	= IORESOURCE_IRQ,
	}
};

static struct platform_device sha1_md5_device = {
	.name		= "OMAP SHA1/MD5",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(sha1_md5_resources),
	.resource	= sha1_md5_resources,
};

static void omap_init_sha1_md5(void)
{
	platform_device_register(&sha1_md5_device);
}
#else
static inline void omap_init_sha1_md5(void) { }
#endif

/*-------------------------------------------------------------------------*/

#if defined(CONFIG_MMC_OMAP) || defined(CONFIG_MMC_OMAP_MODULE) || \
	defined(CONFIG_MMC_OMAP_HS) || defined(CONFIG_MMC_OMAP_HS_MODULE)

static struct omap_device_pm_latency omap_hsmmc_latency[] = {
	[0] = {
		.deactivate_func = omap_device_idle_hwmods,
		.activate_func	 = omap_device_enable_hwmods,
		.flags		 = OMAP_DEVICE_LATENCY_AUTO_ADJUST,
	},
	/*
	 * XXX There should also be an entry here to power off/on the
	 * MMC regulators/PBIAS cells, etc.
	 */
};

static inline void omap2_mmc_mux(struct omap_mmc_platform_data *mmc_controller,
			int controller_nr)
{
	if (cpu_is_omap2420() && controller_nr == 0) {
		omap_cfg_reg(H18_24XX_MMC_CMD);
		omap_cfg_reg(H15_24XX_MMC_CLKI);
		omap_cfg_reg(G19_24XX_MMC_CLKO);
		omap_cfg_reg(F20_24XX_MMC_DAT0);
		omap_cfg_reg(F19_24XX_MMC_DAT_DIR0);
		omap_cfg_reg(G18_24XX_MMC_CMD_DIR);
		if (mmc_controller->slots[0].wires == 4) {
			omap_cfg_reg(H14_24XX_MMC_DAT1);
			omap_cfg_reg(E19_24XX_MMC_DAT2);
			omap_cfg_reg(D19_24XX_MMC_DAT3);
			omap_cfg_reg(E20_24XX_MMC_DAT_DIR1);
			omap_cfg_reg(F18_24XX_MMC_DAT_DIR2);
			omap_cfg_reg(E18_24XX_MMC_DAT_DIR3);
		}

		/*
		 * Use internal loop-back in MMC/SDIO Module Input Clock
		 * selection
		 */
		if (mmc_controller->slots[0].internal_clock) {
			u32 v = omap_ctrl_readl(OMAP2_CONTROL_DEVCONF0);
			v |= (1 << 24);
			omap_ctrl_writel(v, OMAP2_CONTROL_DEVCONF0);
		}
	}

	if (cpu_is_omap34xx()) {
		if (controller_nr == 0) {
			omap_mux_init_signal("sdmmc1_clk",
				OMAP_PIN_INPUT_PULLUP);
			omap_mux_init_signal("sdmmc1_cmd",
				OMAP_PIN_INPUT_PULLUP);
			omap_mux_init_signal("sdmmc1_dat0",
				OMAP_PIN_INPUT_PULLUP);
			if (mmc_controller->slots[0].wires == 4 ||
				mmc_controller->slots[0].wires == 8) {
				omap_mux_init_signal("sdmmc1_dat1",
					OMAP_PIN_INPUT_PULLUP);
				omap_mux_init_signal("sdmmc1_dat2",
					OMAP_PIN_INPUT_PULLUP);
				omap_mux_init_signal("sdmmc1_dat3",
					OMAP_PIN_INPUT_PULLUP);
			}
			if (mmc_controller->slots[0].wires == 8) {
				omap_mux_init_signal("sdmmc1_dat4",
					OMAP_PIN_INPUT_PULLUP);
				omap_mux_init_signal("sdmmc1_dat5",
					OMAP_PIN_INPUT_PULLUP);
				omap_mux_init_signal("sdmmc1_dat6",
					OMAP_PIN_INPUT_PULLUP);
				omap_mux_init_signal("sdmmc1_dat7",
					OMAP_PIN_INPUT_PULLUP);
			}
		}
		if (controller_nr == 1) {
			/* MMC2 */
			omap_mux_init_signal("sdmmc2_clk",
				OMAP_PIN_INPUT_PULLUP);
			omap_mux_init_signal("sdmmc2_cmd",
				OMAP_PIN_INPUT_PULLUP);
			omap_mux_init_signal("sdmmc2_dat0",
				OMAP_PIN_INPUT_PULLUP);

			/*
			 * For 8 wire configurations, Lines DAT4, 5, 6 and 7 need to be muxed
			 * in the board-*.c files
			 */
			if (mmc_controller->slots[0].wires == 4 ||
				mmc_controller->slots[0].wires == 8) {
				omap_mux_init_signal("sdmmc2_dat1",
					OMAP_PIN_INPUT_PULLUP);
				omap_mux_init_signal("sdmmc2_dat2",
					OMAP_PIN_INPUT_PULLUP);
				omap_mux_init_signal("sdmmc2_dat3",
					OMAP_PIN_INPUT_PULLUP);
			}
			if (mmc_controller->slots[0].wires == 8) {
				omap_mux_init_signal("sdmmc2_dat4.sdmmc2_dat4",
					OMAP_PIN_INPUT_PULLUP);
				omap_mux_init_signal("sdmmc2_dat5.sdmmc2_dat5",
					OMAP_PIN_INPUT_PULLUP);
				omap_mux_init_signal("sdmmc2_dat6.sdmmc2_dat6",
					OMAP_PIN_INPUT_PULLUP);
				omap_mux_init_signal("sdmmc2_dat7.sdmmc2_dat7",
					OMAP_PIN_INPUT_PULLUP);
			}
		}

		/*
		 * For MMC3 the pins need to be muxed in the board-*.c files
		 */
	}
}

#define MAX_OMAP_MMC_HWMOD_NAME_LEN		16

void __init omap2_init_mmc(struct omap_mmc_platform_data **mmc_data,
			   int nr_controllers)
{
	struct omap_hwmod *oh;
	struct omap_device *od;
	struct omap_device_pm_latency *ohl;
	char oh_name[MAX_OMAP_MMC_HWMOD_NAME_LEN];
	char *name;
	int i, l;
	int ohl_cnt = 0;

	if (cpu_is_omap2420()) {
		name = "mmci-omap";
	} else {
		name = "mmci-omap-hs";
		ohl = omap_hsmmc_latency;
		ohl_cnt = ARRAY_SIZE(omap_hsmmc_latency);
	}

	/*
	 * XXX This isn't a good way to set these up.  What if a board
	 * uses MMC2 but not MMC1?
	 */
	for (i = 1; i <= nr_controllers; i++) {
		int idx = i - 1;

		l = snprintf(oh_name, MAX_OMAP_MMC_HWMOD_NAME_LEN,
			     "mmc%d", i);
		WARN(l >= MAX_OMAP_MMC_HWMOD_NAME_LEN,
		     "String buffer overflow in MMC%d device setup\n", i);
		oh = omap_hwmod_lookup(oh_name);
		if (!oh) {
			pr_err("Could not look up %s\n", oh_name);
			continue;
		}

		mmc_data[idx]->dev_attr = oh->dev_attr;

		omap2_mmc_mux(mmc_data[idx], idx);
		od = omap_device_build(name, idx, oh, mmc_data[idx],
				       sizeof(struct omap_mmc_platform_data),
				       ohl, ohl_cnt, false);
		WARN(IS_ERR(od), "Could not build omap_device for %s %s\n",
		     name, oh_name);

		/*
		 * return device handle to board setup code
		 * XXX Can this be removed?
		 */
		mmc_data[idx]->dev = &od->pdev.dev;
	};
}

#endif

/*-------------------------------------------------------------------------*/

#if defined(CONFIG_HDQ_MASTER_OMAP) || defined(CONFIG_HDQ_MASTER_OMAP_MODULE)
#if defined(CONFIG_ARCH_OMAP2430) || defined(CONFIG_ARCH_OMAP3430)
#define OMAP_HDQ_BASE	0x480B2000
#endif
static struct resource omap_hdq_resources[] = {
	{
		.start		= OMAP_HDQ_BASE,
		.end		= OMAP_HDQ_BASE + 0x1C,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= INT_24XX_HDQ_IRQ,
		.flags		= IORESOURCE_IRQ,
	},
};
static struct platform_device omap_hdq_dev = {
	.name = "omap_hdq",
	.id = 0,
	.dev = {
		.platform_data = NULL,
	},
	.num_resources	= ARRAY_SIZE(omap_hdq_resources),
	.resource	= omap_hdq_resources,
};
static inline void omap_hdq_init(void)
{
	(void) platform_device_register(&omap_hdq_dev);
}
#else
static inline void omap_hdq_init(void) {}
#endif

/*---------------------------------------------------------------------------*/

#if defined(CONFIG_VIDEO_OMAP2_VOUT) || \
	defined(CONFIG_VIDEO_OMAP2_VOUT_MODULE)
#if defined(CONFIG_FB_OMAP2) || defined(CONFIG_FB_OMAP2_MODULE)
#ifdef CONFIG_ARCH_OMAP4
static struct resource omap_vout_resource[4 - CONFIG_FB_OMAP2_NUM_FBS] = {
#else
static struct resource omap_vout_resource[3 - CONFIG_FB_OMAP2_NUM_FBS] = {
#endif
};
#else
#ifdef CONFIG_ARCH_OMAP4
static struct resource omap_vout_resource[3] = {
#else
static struct resource omap_vout_resource[2] = {
#endif
};
#endif

static struct platform_device omap_vout_device = {
	.name		= "omap_vout",
	.num_resources	= ARRAY_SIZE(omap_vout_resource),
	.resource	= &omap_vout_resource[0],
	.id		= -1,
};
static void omap_init_vout(void)
{
	if (platform_device_register(&omap_vout_device) < 0)
		printk(KERN_ERR "Unable to register OMAP-VOUT device\n");
}

static struct resource sdp4430_wb_resource[1] = {
};

static struct platform_device sdp4430_wb_device = {
	.name		= "omap_wb",
	.num_resources	= ARRAY_SIZE(sdp4430_wb_resource),
	.resource	= &sdp4430_wb_resource[0],
	.id		= -1,
};

static void omap_init_wb(void)
{
		(void) platform_device_register(&sdp4430_wb_device);
}

#else
static inline void omap_init_vout(void) {}
static void omap_init_wb(void) {}
#endif

/*-------------------------------------------------------------------------*/

static int __init omap2_init_devices(void)
{
	/* please keep these calls, and their implementations above,
	 * in alphabetical order so they're easier to sort through.
	 */
	omap_init_camera();
	omap_init_mbox();
	omap_init_mcspi();
	omap_hdq_init();
	omap_init_sti();
	omap_init_sha1_md5();
	omap_init_vout();
	if (cpu_is_omap44xx())
		omap_init_wb();

	return 0;
}
arch_initcall(omap2_init_devices);
