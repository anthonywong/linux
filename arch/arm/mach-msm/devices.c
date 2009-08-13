/* linux/arch/arm/mach-msm/devices.c
 *
 * Copyright (C) 2008 Google, Inc.
 * Copyright (c) 2008-2009, Code Aurora Forum. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/platform_device.h>

#include <linux/dma-mapping.h>
#include <mach/irqs.h>
#include <mach/msm_iomap.h>
#include <mach/dma.h>
#include <mach/board.h>

#include "devices.h"

#include <asm/mach/flash.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>

#include <asm/mach/mmc.h>

static struct resource resources_uart1[] = {
	{
		.start	= INT_UART1,
		.end	= INT_UART1,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= MSM_UART1_PHYS,
		.end	= MSM_UART1_PHYS + MSM_UART1_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
};

static struct resource resources_uart2[] = {
	{
		.start	= INT_UART2,
		.end	= INT_UART2,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= MSM_UART2_PHYS,
		.end	= MSM_UART2_PHYS + MSM_UART2_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
};

static struct resource resources_uart3[] = {
	{
		.start	= INT_UART3,
		.end	= INT_UART3,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= MSM_UART3_PHYS,
		.end	= MSM_UART3_PHYS + MSM_UART3_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
};

struct platform_device msm_device_uart1 = {
	.name	= "msm_serial",
	.id	= 0,
	.num_resources	= ARRAY_SIZE(resources_uart1),
	.resource	= resources_uart1,
};

struct platform_device msm_device_uart2 = {
	.name	= "msm_serial",
	.id	= 1,
	.num_resources	= ARRAY_SIZE(resources_uart2),
	.resource	= resources_uart2,
};

struct platform_device msm_device_uart3 = {
	.name	= "msm_serial",
	.id	= 2,
	.num_resources	= ARRAY_SIZE(resources_uart3),
	.resource	= resources_uart3,
};

#if defined(CONFIG_ARCH_MSM_SCORPION)
#define MSM_UART1DM_PHYS      0xA0200000
#define MSM_UART2DM_PHYS      0xA0900000
#else
#define MSM_UART1DM_PHYS      0xA0200000
#define MSM_UART2DM_PHYS      0xA0300000
#endif

static struct resource resources_i2c[] = {
	{
		.start	= MSM_I2C_PHYS,
		.end	= MSM_I2C_PHYS + MSM_I2C_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= INT_PWB_I2C,
		.end	= INT_PWB_I2C,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device msm_device_i2c = {
	.name		= "msm_i2c",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(resources_i2c),
	.resource	= resources_i2c,
};

static struct resource resources_hsusb[] = {
	{
		.start	= MSM_HSUSB_PHYS,
		.end	= MSM_HSUSB_PHYS + MSM_HSUSB_SIZE,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= INT_USB_HS,
		.end	= INT_USB_HS,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device msm_device_hsusb_peripheral = {
#ifdef CONFIG_USB_ANDROID
	.name		= "msm_hsusb",
#else
	.name		= "msm_hsusb_peripheral",
#endif
	.id		= -1,
	.num_resources	= ARRAY_SIZE(resources_hsusb_peripheral),
	.resource	= resources_hsusb_peripheral,
	.dev		= {
		.dma_mask 		= &dma_mask,
		.coherent_dma_mask	= 0xffffffffULL,
	},
};

static struct resource resources_hsusb_host[] = {
	{
		.start	= MSM_HSUSB_PHYS,
		.end	= MSM_HSUSB_PHYS + MSM_HSUSB_SIZE,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= INT_USB_HS,
		.end	= INT_USB_HS,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device msm_device_hsusb_host = {
	.name		= "msm_hsusb_host",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(resources_hsusb_host),
	.resource	= resources_hsusb_host,
	.dev		= {
		.dma_mask 		= &dma_mask,
		.coherent_dma_mask	= 0xffffffffULL,
	},
};

struct flash_platform_data msm_nand_data = {
	.parts		= NULL,
	.nr_parts	= 0,
};

static struct resource resources_nand[] = {
	[0] = {
		.start	= 7,
		.end	= 7,
		.flags	= IORESOURCE_DMA,
	},
};

struct platform_device msm_device_nand = {
	.name		= "msm_nand",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(resources_nand),
	.resource	= resources_nand,
	.dev		= {
		.platform_data	= &msm_nand_data,
	},
};

struct platform_device msm_device_smd = {
	.name	= "msm_smd",
	.id	= -1,
};

#if defined(CONFIG_ARCH_MSM_SCORPION)
#define MSM_SDC1_BASE         0xA0300000
#define MSM_SDC2_BASE         0xA0400000
#define MSM_SDC3_BASE         0xA0500000
#define MSM_SDC4_BASE         0xA0600000
#else
#define MSM_SDC1_BASE         0xA0400000
#define MSM_SDC2_BASE         0xA0500000
#define MSM_SDC3_BASE         0xA0600000
#define MSM_SDC4_BASE         0xA0700000
#endif

static struct resource resources_sdc1[] = {
	{
		.start	= MSM_SDC1_BASE,
		.end	= MSM_SDC1_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= INT_SDC1_0,
		.end	= INT_SDC1_1,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= 8,
		.end	= 8,
		.flags	= IORESOURCE_DMA,
	},
};

static struct resource resources_sdc2[] = {
	{
		.start	= MSM_SDC2_BASE,
		.end	= MSM_SDC2_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= INT_SDC2_0,
		.end	= INT_SDC2_1,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= 8,
		.end	= 8,
		.flags	= IORESOURCE_DMA,
	},
};

static struct resource resources_sdc3[] = {
	{
		.start	= MSM_SDC3_BASE,
		.end	= MSM_SDC3_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= INT_SDC3_0,
		.end	= INT_SDC3_1,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= 8,
		.end	= 8,
		.flags	= IORESOURCE_DMA,
	},
};

static struct resource resources_sdc4[] = {
	{
		.start	= MSM_SDC4_BASE,
		.end	= MSM_SDC4_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= INT_SDC4_0,
		.end	= INT_SDC4_1,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= 8,
		.end	= 8,
		.flags	= IORESOURCE_DMA,
	},
};

struct platform_device msm_device_sdc1 = {
	.name		= "msm_sdcc",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(resources_sdc1),
	.resource	= resources_sdc1,
	.dev		= {
		.coherent_dma_mask	= 0xffffffff,
	},
};

struct platform_device msm_device_sdc2 = {
	.name		= "msm_sdcc",
	.id		= 2,
	.num_resources	= ARRAY_SIZE(resources_sdc2),
	.resource	= resources_sdc2,
	.dev		= {
		.coherent_dma_mask	= 0xffffffff,
	},
};

struct platform_device msm_device_sdc3 = {
	.name		= "msm_sdcc",
	.id		= 3,
	.num_resources	= ARRAY_SIZE(resources_sdc3),
	.resource	= resources_sdc3,
	.dev		= {
		.coherent_dma_mask	= 0xffffffff,
	},
};

struct platform_device msm_device_sdc4 = {
	.name		= "msm_sdcc",
	.id		= 4,
	.num_resources	= ARRAY_SIZE(resources_sdc4),
	.resource	= resources_sdc4,
	.dev		= {
		.coherent_dma_mask	= 0xffffffff,
	},
};

static struct platform_device *msm_sdcc_devices[] __initdata = {
	&msm_device_sdc1,
	&msm_device_sdc2,
	&msm_device_sdc3,
	&msm_device_sdc4,
};

int __init msm_add_sdcc(unsigned int controller, struct mmc_platform_data *plat)
{
	struct platform_device	*pdev;

	if (controller < 1 || controller > 4)
		return -EINVAL;

	pdev = msm_sdcc_devices[controller-1];
	pdev->dev.platform_data = plat;
	return platform_device_register(pdev);
}

#define CLOCK(clk_name, clk_id, clk_dev, clk_flags, clk_arch) {	\
	.name = clk_name, \
	.id = clk_id, \
	.flags = (clk_flags) | ((clk_arch) & CLKFLAG_ARCH_ALL), \
	.dev = clk_dev, \
	}

static struct platform_device msm_tvenc_device = {
	.name   = "tvenc",
	.id     = 0,
	.num_resources  = ARRAY_SIZE(msm_tvenc_resources),
	.resource       = msm_tvenc_resources,
};

static void __init msm_register_device(struct platform_device *pdev, void *data)
{
	int ret;

	pdev->dev.platform_data = data;

	ret = platform_device_register(pdev);
	if (ret)
		dev_err(&pdev->dev,
			  "%s: platform_device_register() failed = %d\n",
			  __func__, ret);
}

void __init msm_fb_register_device(char *name, void *data)
{
	if (!strncmp(name, "mdp", 3))
		msm_register_device(&msm_mdp_device, data);
	else if (!strncmp(name, "pmdh", 4))
		msm_register_device(&msm_mddi_device, data);
	else if (!strncmp(name, "emdh", 4))
		msm_register_device(&msm_mddi_ext_device, data);
	else if (!strncmp(name, "ebi2", 4))
		msm_register_device(&msm_ebi2_lcd_device, data);
	else if (!strncmp(name, "tvenc", 5))
		msm_register_device(&msm_tvenc_device, data);
	else if (!strncmp(name, "lcdc", 4))
		msm_register_device(&msm_lcdc_device, data);
	else
		printk(KERN_ERR "%s: unknown device! %s\n", __func__, name);
}

static struct platform_device msm_camera_device = {
	.name	= "msm_camera",
	.id	= 0,
};

void __init msm_camera_register_device(void *res, uint32_t num,
	void *data)
{
	msm_camera_device.num_resources = num;
	msm_camera_device.resource = res;

	msm_register_device(&msm_camera_device, data);
}

struct clk msm_clocks_7x01a[] = {
	CLK_PCOM("adm_clk",	ADM_CLK,	NULL, 0),
	CLK_PCOM("adsp_clk",	ADSP_CLK,	NULL, 0),
	CLK_PCOM("ebi1_clk",	EBI1_CLK,	NULL, CLK_MIN),
	CLK_PCOM("ebi2_clk",	EBI2_CLK,	NULL, 0),
	CLK_PCOM("ecodec_clk",	ECODEC_CLK,	NULL, 0),
	CLK_PCOM("emdh_clk",	EMDH_CLK,	NULL, OFF | CLK_MINMAX),
	CLK_PCOM("gp_clk",	GP_CLK,		NULL, 0),
	CLK_PCOM("grp_clk",	GRP_CLK,	NULL, OFF),
	CLK_PCOM("i2c_clk",	I2C_CLK,	&msm_device_i2c.dev, 0),
	CLK_PCOM("icodec_rx_clk",	ICODEC_RX_CLK,	NULL, 0),
	CLK_PCOM("icodec_tx_clk",	ICODEC_TX_CLK,	NULL, 0),
	CLK_PCOM("imem_clk",	IMEM_CLK,	NULL, OFF),
	CLK_PCOM("mdc_clk",	MDC_CLK,	NULL, 0),
	CLK_PCOM("mdp_clk",	MDP_CLK,	NULL, OFF),
	CLK_PCOM("pbus_clk",	PBUS_CLK,	NULL, CLK_MIN),
	CLK_PCOM("pcm_clk",	PCM_CLK,	NULL, 0),
	CLK_PCOM("mddi_clk",	PMDH_CLK,	NULL, OFF | CLK_MINMAX),
	CLK_PCOM("sdac_clk",	SDAC_CLK,	NULL, OFF),
	CLK_PCOM("sdc_clk",	SDC1_CLK,	&msm_device_sdc1.dev, OFF),
	CLK_PCOM("sdc_pclk",	SDC1_PCLK,	&msm_device_sdc1.dev, OFF),
	CLK_PCOM("sdc_clk",	SDC2_CLK,	&msm_device_sdc2.dev, OFF),
	CLK_PCOM("sdc_pclk",	SDC2_PCLK,	&msm_device_sdc2.dev, OFF),
	CLK_PCOM("sdc_clk",	SDC3_CLK,	&msm_device_sdc3.dev, OFF),
	CLK_PCOM("sdc_pclk",	SDC3_PCLK,	&msm_device_sdc3.dev, OFF),
	CLK_PCOM("sdc_clk",	SDC4_CLK,	&msm_device_sdc4.dev, OFF),
	CLK_PCOM("sdc_pclk",	SDC4_PCLK,	&msm_device_sdc4.dev, OFF),
	CLK_PCOM("tsif_clk",	TSIF_CLK,	NULL, 0),
	CLK_PCOM("tsif_ref_clk",	TSIF_REF_CLK,	NULL, 0),
	CLK_PCOM("tv_dac_clk",	TV_DAC_CLK,	NULL, 0),
	CLK_PCOM("tv_enc_clk",	TV_ENC_CLK,	NULL, 0),
	CLK_PCOM("uart_clk",	UART1_CLK,	&msm_device_uart1.dev, OFF),
	CLK_PCOM("uart_clk",	UART2_CLK,	&msm_device_uart2.dev, 0),
	CLK_PCOM("uart_clk",	UART3_CLK,	&msm_device_uart3.dev, OFF),
	CLK_PCOM("uartdm_clk",	UART1DM_CLK,	&msm_device_uart_dm1.dev, OFF),
	CLK_PCOM("uartdm_clk",	UART2DM_CLK,	&msm_device_uart_dm2.dev, 0),
	CLK_PCOM("usb_hs_clk",	USB_HS_CLK,	NULL, OFF),
	CLK_PCOM("usb_hs_pclk",	USB_HS_PCLK,	NULL, OFF),
	CLK_PCOM("usb_otg_clk",	USB_OTG_CLK,	NULL, 0),
	CLK_PCOM("vdc_clk",	VDC_CLK,	NULL, OFF | CLK_MIN),
	CLK_PCOM("vfe_clk",	VFE_CLK,	NULL, OFF),
	CLK_PCOM("vfe_mdc_clk",	VFE_MDC_CLK,	NULL, OFF),
	CLK_PCOM("grp_pclk",	GRP_PCLK,	NULL, 0),
};

unsigned msm_num_clocks_7x01a = ARRAY_SIZE(msm_clocks_7x01a);

struct clk msm_clocks_7x25[] = {
	CLK_PCOM("adm_clk",	ADM_CLK,	NULL, 0),
	CLK_PCOM("adsp_clk",	ADSP_CLK,	NULL, 0),
	CLK_PCOM("ebi1_clk",	EBI1_CLK,	NULL, CLK_MIN),
	CLK_PCOM("ebi2_clk",	EBI2_CLK,	NULL, 0),
	CLK_PCOM("ecodec_clk",	ECODEC_CLK,	NULL, 0),
	CLK_PCOM("gp_clk",	GP_CLK,		NULL, 0),
	CLK_PCOM("i2c_clk",	I2C_CLK,	&msm_device_i2c.dev, 0),
	CLK_PCOM("icodec_rx_clk",	ICODEC_RX_CLK,	NULL, 0),
	CLK_PCOM("icodec_tx_clk",	ICODEC_TX_CLK,	NULL, 0),
	CLK_PCOM("imem_clk",	IMEM_CLK,	NULL, OFF),
	CLK_PCOM("mdc_clk",	MDC_CLK,	NULL, 0),
	CLK_PCOM("mdp_clk",	MDP_CLK,	NULL, OFF),
	CLK_PCOM("mdp_lcdc_pclk_clk", MDP_LCDC_PCLK_CLK, NULL, 0),
	CLK_PCOM("mdp_lcdc_pad_pclk_clk", MDP_LCDC_PAD_PCLK_CLK, NULL, 0),
	CLK_PCOM("mdp_vsync_clk",	MDP_VSYNC_CLK,  NULL, 0),
	CLK_PCOM("pbus_clk",	PBUS_CLK,	NULL, CLK_MIN),
	CLK_PCOM("pcm_clk",	PCM_CLK,	NULL, 0),
	CLK_PCOM("mddi_clk",	PMDH_CLK,	NULL, OFF | CLK_MINMAX),
	CLK_PCOM("sdac_clk",	SDAC_CLK,	NULL, OFF),
	CLK_PCOM("sdc_clk",	SDC1_CLK,	&msm_device_sdc1.dev, OFF),
	CLK_PCOM("sdc_pclk",	SDC1_PCLK,	&msm_device_sdc1.dev, OFF),
	CLK_PCOM("sdc_clk",	SDC2_CLK,	&msm_device_sdc2.dev, OFF),
	CLK_PCOM("sdc_pclk",	SDC2_PCLK,	&msm_device_sdc2.dev, OFF),
	CLK_PCOM("sdc_clk",	SDC3_CLK,	&msm_device_sdc3.dev, OFF),
	CLK_PCOM("sdc_pclk",	SDC3_PCLK,	&msm_device_sdc3.dev, OFF),
	CLK_PCOM("sdc_clk",	SDC4_CLK,	&msm_device_sdc4.dev, OFF),
	CLK_PCOM("sdc_pclk",	SDC4_PCLK,	&msm_device_sdc4.dev, OFF),
	CLK_PCOM("uart_clk",	UART1_CLK,	&msm_device_uart1.dev, OFF),
	CLK_PCOM("uart_clk",	UART2_CLK,	&msm_device_uart2.dev, 0),
	CLK_PCOM("uart_clk",	UART3_CLK,	&msm_device_uart3.dev, OFF),
	CLK_PCOM("uartdm_clk",	UART1DM_CLK,	&msm_device_uart_dm1.dev, OFF),
	CLK_PCOM("uartdm_clk",	UART2DM_CLK,	&msm_device_uart_dm2.dev, 0),
	CLK_PCOM("usb_hs_clk",	USB_HS_CLK,	NULL, OFF),
	CLK_PCOM("usb_hs_pclk",	USB_HS_PCLK,	NULL, OFF),
	CLK_PCOM("usb_otg_clk",	USB_OTG_CLK,	NULL, 0),
	CLK_PCOM("vdc_clk",	VDC_CLK,	NULL, OFF | CLK_MIN),
	CLK_PCOM("vfe_clk",	VFE_CLK,	NULL, OFF),
	CLK_PCOM("vfe_mdc_clk",	VFE_MDC_CLK,	NULL, OFF),
	CLK_PCOM("grp_pclk",	GRP_PCLK,	NULL, 0),
};

unsigned msm_num_clocks_7x25 = ARRAY_SIZE(msm_clocks_7x25);

struct clk msm_clocks_7x27[] = {
	CLK_PCOM("adm_clk",	ADM_CLK,	NULL, 0),
	CLK_PCOM("adsp_clk",	ADSP_CLK,	NULL, 0),
	CLK_PCOM("ebi1_clk",	EBI1_CLK,	NULL, CLK_MIN),
	CLK_PCOM("ebi2_clk",	EBI2_CLK,	NULL, 0),
	CLK_PCOM("ecodec_clk",	ECODEC_CLK,	NULL, 0),
	CLK_PCOM("gp_clk",	GP_CLK,		NULL, 0),
	CLK_PCOM("grp_clk",	GRP_CLK,	NULL, 0),
	CLK_PCOM("i2c_clk",	I2C_CLK,	&msm_device_i2c.dev, 0),
	CLK_PCOM("icodec_rx_clk",	ICODEC_RX_CLK,	NULL, 0),
	CLK_PCOM("icodec_tx_clk",	ICODEC_TX_CLK,	NULL, 0),
	CLK_PCOM("imem_clk",	IMEM_CLK,	NULL, OFF),
	CLK_PCOM("mdc_clk",	MDC_CLK,	NULL, 0),
	CLK_PCOM("mdp_clk",	MDP_CLK,	NULL, OFF),
	CLK_PCOM("mdp_lcdc_pclk_clk", MDP_LCDC_PCLK_CLK, NULL, 0),
	CLK_PCOM("mdp_lcdc_pad_pclk_clk", MDP_LCDC_PAD_PCLK_CLK, NULL, 0),
	CLK_PCOM("mdp_vsync_clk",	MDP_VSYNC_CLK,  NULL, 0),
	CLK_PCOM("pbus_clk",	PBUS_CLK,	NULL, CLK_MIN),
	CLK_PCOM("pcm_clk",	PCM_CLK,	NULL, 0),
	CLK_PCOM("mddi_clk",	PMDH_CLK,	NULL, OFF | CLK_MINMAX),
	CLK_PCOM("sdac_clk",	SDAC_CLK,	NULL, OFF),
	CLK_PCOM("sdc_clk",	SDC1_CLK,	&msm_device_sdc1.dev, OFF),
	CLK_PCOM("sdc_pclk",	SDC1_PCLK,	&msm_device_sdc1.dev, OFF),
	CLK_PCOM("sdc_clk",	SDC2_CLK,	&msm_device_sdc2.dev, OFF),
	CLK_PCOM("sdc_pclk",	SDC2_PCLK,	&msm_device_sdc2.dev, OFF),
	CLK_PCOM("sdc_clk",	SDC3_CLK,	&msm_device_sdc3.dev, OFF),
	CLK_PCOM("sdc_pclk",	SDC3_PCLK,	&msm_device_sdc3.dev, OFF),
	CLK_PCOM("sdc_clk",	SDC4_CLK,	&msm_device_sdc4.dev, OFF),
	CLK_PCOM("sdc_pclk",	SDC4_PCLK,	&msm_device_sdc4.dev, OFF),
	CLK_PCOM("uart_clk",	UART1_CLK,	&msm_device_uart1.dev, OFF),
	CLK_PCOM("uart_clk",	UART2_CLK,	&msm_device_uart2.dev, 0),
	CLK_PCOM("uart_clk",	UART3_CLK,	&msm_device_uart3.dev, OFF),
	CLK_PCOM("uartdm_clk",	UART1DM_CLK,	&msm_device_uart_dm1.dev, OFF),
	CLK_PCOM("uartdm_clk",	UART2DM_CLK,	&msm_device_uart_dm2.dev, 0),
	CLK_PCOM("usb_hs_clk",	USB_HS_CLK,	NULL, OFF),
	CLK_PCOM("usb_hs_pclk",	USB_HS_PCLK,	NULL, OFF),
	CLK_PCOM("usb_otg_clk",	USB_OTG_CLK,	NULL, 0),
	CLK_PCOM("vdc_clk",	VDC_CLK,	NULL, OFF | CLK_MIN),
	CLK_PCOM("vfe_clk",	VFE_CLK,	NULL, OFF),
	CLK_PCOM("vfe_mdc_clk",	VFE_MDC_CLK,	NULL, OFF),
};

unsigned msm_num_clocks_7x27 = ARRAY_SIZE(msm_clocks_7x27);

struct clk msm_clocks_7x30[] = {
	CLK_PCOM("adm_clk",	ADM_CLK,	NULL, 0),
	CLK_PCOM("adsp_clk",	ADSP_CLK,	NULL, 0),
	CLK_PCOM("ebi1_clk",	EBI1_CLK,	NULL, CLK_MIN),
	CLK_PCOM("ebi2_clk",	EBI2_CLK,	NULL, 0),
	CLK_PCOM("ecodec_clk",	ECODEC_CLK,	NULL, 0),
	CLK_PCOM("gp_clk",	GP_CLK,		NULL, 0),
	CLK_PCOM("grp_clk",	GRP_CLK,	NULL, 0),
	CLK_PCOM("i2c_clk",	I2C_CLK,	&msm_device_i2c.dev, 0),
	CLK_PCOM("imem_clk",	IMEM_CLK,	NULL, OFF),
	CLK_PCOM("mdc_clk",	MDC_CLK,	NULL, 0),
	CLK_PCOM("mdp_clk",	MDP_CLK,	NULL, OFF),
	CLK_PCOM("mdp_pclk",	MDP_PCLK,	NULL, 0),
	CLK_PCOM("mdp_lcdc_pclk_clk", MDP_LCDC_PCLK_CLK, NULL, 0),
	CLK_PCOM("mdp_lcdc_pad_pclk_clk", MDP_LCDC_PAD_PCLK_CLK, NULL, 0),
	CLK_PCOM("mdp_vsync_clk",	MDP_VSYNC_CLK,  NULL, 0),
	CLK_PCOM("pbus_clk",	PBUS_CLK,	NULL, CLK_MIN),
	CLK_PCOM("pcm_clk",	PCM_CLK,	NULL, 0),
	CLK_PCOM("mddi_clk",	PMDH_CLK,	NULL, OFF | CLK_MINMAX),
	CLK_PCOM("mddi_pclk",	PMDH_PCLK,	NULL, 0),
	CLK_PCOM("sdac_clk",	SDAC_CLK,	NULL, OFF),
	CLK_PCOM("sdc_clk",	SDC1_CLK,	&msm_device_sdc1.dev, OFF),
	CLK_PCOM("sdc_pclk",	SDC1_PCLK,	&msm_device_sdc1.dev, OFF),
	CLK_PCOM("sdc_clk",	SDC2_CLK,	&msm_device_sdc2.dev, OFF),
	CLK_PCOM("sdc_pclk",	SDC2_PCLK,	&msm_device_sdc2.dev, OFF),
	CLK_PCOM("sdc_clk",	SDC3_CLK,	&msm_device_sdc3.dev, OFF),
	CLK_PCOM("sdc_pclk",	SDC3_PCLK,	&msm_device_sdc3.dev, OFF),
	CLK_PCOM("sdc_clk",	SDC4_CLK,	&msm_device_sdc4.dev, OFF),
	CLK_PCOM("sdc_pclk",	SDC4_PCLK,	&msm_device_sdc4.dev, OFF),
	CLK_PCOM("uart_clk",	UART1_CLK,	&msm_device_uart1.dev, OFF),
	CLK_PCOM("uart_clk",	UART2_CLK,	&msm_device_uart2.dev, 0),
	CLK_PCOM("uart_clk",	UART3_CLK,	&msm_device_uart3.dev, OFF),
	CLK_PCOM("uartdm_clk",	UART1DM_CLK,	&msm_device_uart_dm1.dev, OFF),
	CLK_PCOM("uartdm_clk",	UART2DM_CLK,	&msm_device_uart_dm2.dev, 0),
	CLK_PCOM("usb_hs_clk",		USB_HS_CLK,		NULL, OFF),
	CLK_PCOM("usb_hs_pclk",		USB_HS_PCLK,		NULL, OFF),
	CLK_PCOM("usb_hs_core_clk",	USB_HS_CORE_CLK,	NULL, OFF),
	CLK_PCOM("usb_hs2_clk",		USB_HS2_CLK,		NULL, OFF),
	CLK_PCOM("usb_hs2_pclk",	USB_HS2_PCLK,		NULL, OFF),
	CLK_PCOM("usb_hs2_core_clk",	USB_HS2_CORE_CLK,	NULL, OFF),
	CLK_PCOM("usb_hs3_clk",		USB_HS3_CLK,		NULL, OFF),
	CLK_PCOM("usb_hs3_pclk",	USB_HS3_PCLK,		NULL, OFF),
	CLK_PCOM("usb_hs3_core_clk",	USB_HS3_CORE_CLK,	NULL, OFF),
	CLK_PCOM("usb_otg_clk",	USB_OTG_CLK,	NULL, 0),
	CLK_PCOM("vdc_clk",	VDC_CLK,	NULL, OFF | CLK_MIN),
	CLK_PCOM("vfe_clk",	VFE_CLK,	NULL, OFF),
	CLK_PCOM("vfe_mdc_clk",	VFE_MDC_CLK,	NULL, OFF),
	CLK_PCOM("grp_pclk",	GRP_PCLK,	NULL, 0),
};

unsigned msm_num_clocks_7x30 = ARRAY_SIZE(msm_clocks_7x30);

struct clk msm_clocks_8x50[] = {
	CLK_PCOM("adm_clk",	ADM_CLK,	NULL, 0),
	CLK_PCOM("ebi1_clk",	EBI1_CLK,	NULL, CLK_MIN),
	CLK_PCOM("ebi2_clk",	EBI2_CLK,	NULL, 0),
	CLK_PCOM("ecodec_clk",	ECODEC_CLK,	NULL, 0),
	CLK_PCOM("emdh_clk",	EMDH_CLK,	NULL, OFF | CLK_MINMAX),
	CLK_PCOM("gp_clk",	GP_CLK,		NULL, 0),
	CLK_PCOM("grp_clk",	GRP_CLK,	NULL, 0),
	CLK_PCOM("i2c_clk",	I2C_CLK,	&msm_device_i2c.dev, 0),
	CLK_PCOM("icodec_rx_clk",	ICODEC_RX_CLK,	NULL, 0),
	CLK_PCOM("icodec_tx_clk",	ICODEC_TX_CLK,	NULL, 0),
	CLK_PCOM("imem_clk",	IMEM_CLK,	NULL, OFF),
	CLK_PCOM("mdc_clk",	MDC_CLK,	NULL, 0),
	CLK_PCOM("mddi_clk",	PMDH_CLK,	NULL, OFF | CLK_MINMAX),
	CLK_PCOM("mdp_clk",	MDP_CLK,	NULL, OFF),
	CLK_PCOM("mdp_lcdc_pclk_clk", MDP_LCDC_PCLK_CLK, NULL, 0),
	CLK_PCOM("mdp_lcdc_pad_pclk_clk", MDP_LCDC_PAD_PCLK_CLK, NULL, 0),
	CLK_PCOM("mdp_vsync_clk",	MDP_VSYNC_CLK,	NULL, 0),
	CLK_PCOM("pbus_clk",	PBUS_CLK,	NULL, CLK_MIN),
	CLK_PCOM("pcm_clk",	PCM_CLK,	NULL, 0),
	CLK_PCOM("sdac_clk",	SDAC_CLK,	NULL, OFF),
	CLK_PCOM("sdc_clk",	SDC1_CLK,	&msm_device_sdc1.dev, OFF),
	CLK_PCOM("sdc_pclk",	SDC1_PCLK,	&msm_device_sdc1.dev, OFF),
	CLK_PCOM("sdc_clk",	SDC2_CLK,	&msm_device_sdc2.dev, OFF),
	CLK_PCOM("sdc_pclk",	SDC2_PCLK,	&msm_device_sdc2.dev, OFF),
	CLK_PCOM("sdc_clk",	SDC3_CLK,	&msm_device_sdc3.dev, OFF),
	CLK_PCOM("sdc_pclk",	SDC3_PCLK,	&msm_device_sdc3.dev, OFF),
	CLK_PCOM("sdc_clk",	SDC4_CLK,	&msm_device_sdc4.dev, OFF),
	CLK_PCOM("sdc_pclk",	SDC4_PCLK,	&msm_device_sdc4.dev, OFF),
	CLK_PCOM("spi_clk",	SPI_CLK,	NULL, 0),
	CLK_PCOM("tsif_clk",	TSIF_CLK,	NULL, 0),
	CLK_PCOM("tsif_ref_clk",	TSIF_REF_CLK,	NULL, 0),
	CLK_PCOM("tv_dac_clk",	TV_DAC_CLK,	NULL, 0),
	CLK_PCOM("tv_enc_clk",	TV_ENC_CLK,	NULL, 0),
	CLK_PCOM("uart_clk",	UART1_CLK,	&msm_device_uart1.dev, OFF),
	CLK_PCOM("uart_clk",	UART2_CLK,	&msm_device_uart2.dev, 0),
	CLK_PCOM("uart_clk",	UART3_CLK,	&msm_device_uart3.dev, OFF),
	CLK_PCOM("uartdm_clk",	UART1DM_CLK,	&msm_device_uart_dm1.dev, OFF),
	CLK_PCOM("uartdm_clk",	UART2DM_CLK,	&msm_device_uart_dm2.dev, 0),
	CLK_PCOM("usb_hs_clk",	USB_HS_CLK,	NULL, OFF),
	CLK_PCOM("usb_hs_pclk",	USB_HS_PCLK,	NULL, OFF),
	CLK_PCOM("usb_otg_clk",	USB_OTG_CLK,	NULL, 0),
	CLK_PCOM("vdc_clk",	VDC_CLK,	NULL, OFF | CLK_MIN),
	CLK_PCOM("vfe_clk",	VFE_CLK,	NULL, OFF),
	CLK_PCOM("vfe_mdc_clk",	VFE_MDC_CLK,	NULL, OFF),
	CLK_PCOM("vfe_axi_clk",	VFE_AXI_CLK,	NULL, OFF),
	CLK_PCOM("usb_hs2_clk",	USB_HS2_CLK,	NULL, OFF),
	CLK_PCOM("usb_hs2_pclk",	USB_HS2_PCLK,	NULL, OFF),
	CLK_PCOM("usb_hs3_clk",	USB_HS3_CLK,	NULL, OFF),
	CLK_PCOM("usb_hs3_pclk",	USB_HS3_PCLK,	NULL, OFF),
	CLK_PCOM("usb_phy_clk",	USB_PHY_CLK,	NULL, 0),
};

unsigned msm_num_clocks_8x50 = ARRAY_SIZE(msm_clocks_8x50);
