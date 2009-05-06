/*
 * arch/arm/mach-dove/common.c
 *
 * Core functions for Marvell Dove MV88F6781 System On Chip
 *
 * Author: Tzachi Perelstein <tzachi@marvell.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/pci.h>
#include <linux/serial_8250.h>
#include <linux/mbus.h>
#include <linux/mv643xx_eth.h>
#include <linux/mv643xx_i2c.h>
#include <linux/ata_platform.h>
#include <linux/spi/orion_spi.h>
#include <linux/spi/mv_spi.h>
#include <linux/gpio_mouse.h>
#include <linux/dove_sdhci.h>
#include <asm/page.h>
#include <asm/setup.h>
#include <asm/timex.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>
#include <asm/mach/pci.h>
#include <mach/dove.h>
#include <asm/mach/arch.h>
#include <mach/gpio.h>
#include <linux/irq.h>
#include <plat/cache-tauros2.h>
#include <plat/ehci-orion.h>
#include <plat/i2s-orion.h>
#include <plat/orion_nand.h>
#include <plat/time.h>
#include <plat/mv_xor.h>
#include <ctrlEnv/mvCtrlEnvRegs.h>
#include <audio/mvAudioRegs.h>

/* used for memory allocation for the VPRO video engine */
#ifdef CONFIG_UIO_DOVE_VPRO
#define UIO_DOVE_VPRO_MEM_SIZE (CONFIG_UIO_DOVE_VPRO_MEM_SIZE << 20)
#else
#define UIO_DOVE_VPRO_MEM_SIZE 0
#endif

static unsigned int dove_vpro_memory_start;
static unsigned int vpro_size = UIO_DOVE_VPRO_MEM_SIZE;

/* used for memory allocation for the GPU graphics engine */
#ifdef CONFIG_DOVE_GPU
#define DOVE_GPU_MEM_SIZE (CONFIG_DOVE_GPU_MEM_SIZE << 20)
#else
#define DOVE_GPU_MEM_SIZE 0
#endif

static unsigned int dove_gpu_memory_start;
static unsigned int gpu_size = DOVE_GPU_MEM_SIZE;


#ifdef CONFIG_MV_INCLUDE_USB
extern u32   mvUsbHalInit(int dev, int isHost);
/* Required to get the configuration string from the Kernel Command Line */
static char *usb0Mode = "host";
static char *usb1Mode = "host";
static char *usb_dev_name  = "mv_udc";
int mv_usb0_cmdline_config(char *s);
int mv_usb1_cmdline_config(char *s);
__setup("usb0Mode=", mv_usb0_cmdline_config);
__setup("usb1Mode=", mv_usb1_cmdline_config);

static int noL2 = 0;
static int __init noL2_setup(char *__unused)
{
     noL2 = 1;
     return 1;
}

__setup("noL2", noL2_setup);
int mv_usb0_cmdline_config(char *s)
{
    usb0Mode = s;
    return 1;
}

int mv_usb1_cmdline_config(char *s)
{
    usb1Mode = s;
    return 1;
}


#endif
#include "common.h"

#ifdef CONFIG_PM
enum orion_cpu_conf_save_state {
	/* CPU Configuration Registers */
	DOVE_DWNSTRM_BRDG_CPU_CONFIG = 0,
	DOVE_DWNSTRM_BRDG_CPU_CTRL_STATUS,
	DOVE_DWNSTRM_BRDG_CPU_RESET_MASK,
	DOVE_DWNSTRM_BRDG_BRIDGE_MASK,
	DOVE_DWNSTRM_BRDG_POWER_MANAGEMENT,
	/* CPU Timers Registers */
	DOVE_DWNSTRM_BRDG_TIMER_CTRL,
	DOVE_DWNSTRM_BRDG_TIMER0_RELOAD,
	DOVE_DWNSTRM_BRDG_TIMER1_RELOAD,
	DOVE_DWNSTRM_BRDG_TIMER_WD_RELOAD,
	/* Main Interrupt Controller Registers */
	DOVE_DWNSTRM_BRDG_MAIN_IRQ_MASK,
	DOVE_DWNSTRM_BRDG_MAIN_FIQ_MASK,
	DOVE_DWNSTRM_BRDG_MAIN2_IRQ_MASK,
	DOVE_DWNSTRM_BRDG_MAIN2_FIQ_MASK,
	DOVE_DWNSTRM_BRDG_ENDPOINT2_MASK,
	DOVE_DWNSTRM_BRDG_PEX_0_1_MASK,	

	DOVE_DWNSTRM_BRDG_SIZE
};

#define DOVE_DWNSTRM_BRDG_SAVE(x) \
	dove_downstream_regs[DOVE_DWNSTRM_BRDG_##x] = readl(x)
#define DOVE_DWNSTRM_BRDG_RESTORE(x) \
	writel(dove_downstream_regs[DOVE_DWNSTRM_BRDG_##x], x)

static u32 dove_downstream_regs[DOVE_DWNSTRM_BRDG_SIZE];
#endif

/*****************************************************************************
 * I/O Address Mapping
 ****************************************************************************/
static struct map_desc dove_io_desc[] __initdata = {
	{
		.virtual	= DOVE_PMUSP_VIRT_BASE,
		.pfn		= __phys_to_pfn(DOVE_PMUSP_PHYS_BASE),
		.length		= DOVE_PMUSP_SIZE,
		.type		= MT_EXEC_REGS,
	}, {
		.virtual	= DOVE_SCRATCHPAD_VIRT_BASE,
		.pfn		= __phys_to_pfn(DOVE_SCRATCHPAD_PHYS_BASE),
		.length		= DOVE_SCRATCHPAD_SIZE,
		.type		= MT_EXEC_REGS,
	}, {
		.virtual	= DOVE_SB_REGS_VIRT_BASE,
		.pfn		= __phys_to_pfn(DOVE_SB_REGS_PHYS_BASE),
		.length		= DOVE_SB_REGS_SIZE,
		.type		= MT_DEVICE,
	}, {
		.virtual	= DOVE_NB_REGS_VIRT_BASE,
		.pfn		= __phys_to_pfn(DOVE_NB_REGS_PHYS_BASE),
		.length		= DOVE_NB_REGS_SIZE,
		.type		= MT_DEVICE,
	}, {
		.virtual	= DOVE_PCIE0_IO_VIRT_BASE,
		.pfn		= __phys_to_pfn(DOVE_PCIE0_IO_PHYS_BASE),
		.length		= DOVE_PCIE0_IO_SIZE,
		.type		= MT_DEVICE,
	}, {
		.virtual	= DOVE_PCIE1_IO_VIRT_BASE,
		.pfn		= __phys_to_pfn(DOVE_PCIE1_IO_PHYS_BASE),
		.length		= DOVE_PCIE1_IO_SIZE,
		.type		= MT_DEVICE,
	},{
		.virtual	= DOVE_CESA_VIRT_BASE,
		.pfn		= __phys_to_pfn(DOVE_CESA_PHYS_BASE),
		.length		= DOVE_CESA_SIZE,
		.type		= MT_DEVICE,
	},


};

void __init dove_map_io(void)
{
	iotable_init(dove_io_desc, ARRAY_SIZE(dove_io_desc));
}
/*****************************************************************************
 * SDIO
 ****************************************************************************/
#define DOVE_SD0_DATA1_GPIO	(32 + 11)
#define DOVE_SD1_DATA1_GPIO	(32 + 17)


struct sdhci_dove_int_wa sdio0_data = {
	.gpio = DOVE_SD0_DATA1_GPIO,
	.func_select_bit = 0
};

static u64 sdio_dmamask = DMA_BIT_MASK(32);
/*****************************************************************************
 * SDIO0
 ****************************************************************************/

static struct resource dove_sdio0_resources[] = {
	{
		.start	= DOVE_SDIO0_PHYS_BASE,
		.end	= DOVE_SDIO0_PHYS_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= IRQ_DOVE_SDIO0,
		.end	= IRQ_DOVE_SDIO0,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device dove_sdio0 = {
	.name		= "sdhci-mv",
	.id		= 0,
	.dev		= {
		.dma_mask		= &sdio_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
		.platform_data		= &sdio0_data,
	},
	.resource	= dove_sdio0_resources,
	.num_resources	= ARRAY_SIZE(dove_sdio0_resources),
};

void __init dove_sdio0_init(void)
{
	platform_device_register(&dove_sdio0);
}

/*****************************************************************************
 * SDIO1
 ****************************************************************************/
struct sdhci_dove_int_wa sdio1_data = {
	.gpio = DOVE_SD1_DATA1_GPIO,
	.func_select_bit = 1
};

static struct resource dove_sdio1_resources[] = {
	{
		.start	= DOVE_SDIO1_PHYS_BASE,
		.end	= DOVE_SDIO1_PHYS_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= IRQ_DOVE_SDIO1,
		.end	= IRQ_DOVE_SDIO1,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device dove_sdio1 = {
	.name		= "sdhci-mv",
	.id		= 1,
	.dev		= {
		.dma_mask		= &sdio_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
		.platform_data		= &sdio1_data,
	},
	.resource	= dove_sdio1_resources,
	.num_resources	= ARRAY_SIZE(dove_sdio1_resources),
};

void __init dove_sdio1_init(void)
{
	platform_device_register(&dove_sdio1);
}

/* sdio card interrupt workaround, when no command is running, switch the
 * DATA[1] pin to be gpio-in with level interrupt.
 */

void __init dove_sd_card_int_wa_setup(int port)
{
	int	gpio = 0;
	char	*name;

	switch(port) {
	case 0:
		gpio = DOVE_SD0_DATA1_GPIO;
		name = "sd0_data1";
		sdio0_data.irq = gpio_to_irq(gpio);
		break;
	case 1:
		gpio = DOVE_SD1_DATA1_GPIO;
		name = "sd1_data1";
		sdio1_data.irq = gpio_to_irq(gpio);
		break;
	default:
		printk(KERN_ERR "dove_sd_card_int_wa_setup: bad port (%d)\n", 
		       port);
		return;
	}

	orion_gpio_set_valid(gpio, 1);
	
	if (gpio_request(gpio, name) != 0)
		printk(KERN_ERR "dove: failed to config gpio for sd_data1\n");

	gpio_direction_input(gpio);
	set_irq_type(gpio_to_irq(gpio), IRQ_TYPE_LEVEL_LOW);
}

/*****************************************************************************
 * EHCI
 ****************************************************************************/
static struct orion_ehci_data dove_ehci_data = {
	.dram		= &dove_mbus_dram_info,
};

static u64 ehci_dmamask = DMA_BIT_MASK(32);

/*****************************************************************************
 * EHCI0
 ****************************************************************************/
static struct resource dove_ehci0_resources[] = {
	{
		.start	= DOVE_USB0_PHYS_BASE,
		.end	= DOVE_USB0_PHYS_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= IRQ_DOVE_USB0,
		.end	= IRQ_DOVE_USB0,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device dove_ehci0 = {
	.name		= "orion-ehci",
	.id		= 0,
	.dev		= {
		.dma_mask		= &ehci_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
		.platform_data		= &dove_ehci_data,
	},
	.resource	= dove_ehci0_resources,
	.num_resources	= ARRAY_SIZE(dove_ehci0_resources),
};

void __init dove_ehci0_init(void)
{
#ifdef CONFIG_MV_INCLUDE_USB
	if (	(strcmp(usb0Mode, "device") == 0) && 
		(strcmp(usb1Mode, "device") == 0)) {
		printk("Warning: trying to set both USB0 and USB1 to device mode!\n");
	}

	if (strcmp(usb0Mode, "host") == 0) {
		printk("Initializing USB0 Host\n");
		mvUsbHalInit(0, 1);
	}
	else {
		printk("Initializing USB0 Device\n");
		dove_ehci0.name = usb_dev_name;
		mvUsbHalInit(0, 0);
	}
	platform_device_register(&dove_ehci0);
#endif
}

/*****************************************************************************
 * EHCI1
 ****************************************************************************/
static struct resource dove_ehci1_resources[] = {
	{
		.start	= DOVE_USB1_PHYS_BASE,
		.end	= DOVE_USB1_PHYS_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= IRQ_DOVE_USB1,
		.end	= IRQ_DOVE_USB1,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device dove_ehci1 = {
	.name		= "orion-ehci",
	.id		= 1,
	.dev		= {
		.dma_mask		= &ehci_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
		.platform_data		= &dove_ehci_data,
	},
	.resource	= dove_ehci1_resources,
	.num_resources	= ARRAY_SIZE(dove_ehci1_resources),
};

void __init dove_ehci1_init(void)
{
#ifdef CONFIG_MV_INCLUDE_USB
	if (strcmp(usb1Mode, "host") == 0) {
		printk("Initializing USB1 Host\n");
		mvUsbHalInit(1, 1);
	}
	else {
		printk("Initializing USB1 Device\n");
		dove_ehci1.name = usb_dev_name;
		mvUsbHalInit(1, 0);
	}

	platform_device_register(&dove_ehci1);
#endif
}
#if 1
/*****************************************************************************
 * GE00
 ****************************************************************************/
struct mv643xx_eth_shared_platform_data dove_ge00_shared_data = {
	.t_clk		= 0,
	.dram		= &dove_mbus_dram_info,
};

static struct resource dove_ge00_shared_resources[] = {
	{
		.name	= "ge00 base",
		.start	= DOVE_GE00_PHYS_BASE + 0x2000,
		.end	= DOVE_GE00_PHYS_BASE + SZ_16K - 1,
		.flags	= IORESOURCE_MEM,
	},
};

static struct platform_device dove_ge00_shared = {
	.name		= MV643XX_ETH_SHARED_NAME,
	.id		= 0,
	.dev		= {
		.platform_data	= &dove_ge00_shared_data,
	},
	.num_resources	= 1,
	.resource	= dove_ge00_shared_resources,
};

static struct resource dove_ge00_resources[] = {
	{
		.name	= "ge00 irq",
		.start	= IRQ_DOVE_GE00_SUM,
		.end	= IRQ_DOVE_GE00_SUM,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device dove_ge00 = {
	.name		= MV643XX_ETH_NAME,
	.id		= 0,
	.num_resources	= 1,
	.resource	= dove_ge00_resources,
};

void __init dove_ge00_init(struct mv643xx_eth_platform_data *eth_data)
{
	eth_data->shared = &dove_ge00_shared;
	dove_ge00.dev.platform_data = eth_data;

	platform_device_register(&dove_ge00_shared);
	platform_device_register(&dove_ge00);
}
#endif

/*****************************************************************************
 * SoC RTC
 ****************************************************************************/
static struct resource dove_rtc_resource[] = {
	{
		.start	= DOVE_RTC_PHYS_BASE,
		.end	= DOVE_RTC_PHYS_BASE + 32 - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= IRQ_DOVE_RTC,
		.flags	= IORESOURCE_IRQ,
	}
};

void __init dove_rtc_init(void)
{
	platform_device_register_simple("rtc-mv", -1, dove_rtc_resource, 2);
}

/*****************************************************************************
 * SATA
 ****************************************************************************/
static struct resource dove_sata_resources[] = {
	{
		.name	= "sata base",
		.start	= DOVE_SATA_PHYS_BASE,
		.end	= DOVE_SATA_PHYS_BASE + 0x5000 - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.name	= "sata irq",
		.start	= IRQ_DOVE_SATA,
		.end	= IRQ_DOVE_SATA,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device dove_sata = {
	.name		= "sata_mv",
	.id		= 0,
	.dev		= {
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
	.num_resources	= ARRAY_SIZE(dove_sata_resources),
	.resource	= dove_sata_resources,
};

void __init dove_sata_init(struct mv_sata_platform_data *sata_data)
{
	sata_data->dram = &dove_mbus_dram_info;
	dove_sata.dev.platform_data = sata_data;
	platform_device_register(&dove_sata);
}

/*****************************************************************************
 * UART0
 ****************************************************************************/
static struct plat_serial8250_port dove_uart0_data[] = {
	{
		.mapbase	= DOVE_UART0_PHYS_BASE,
		.membase	= (char *)DOVE_UART0_VIRT_BASE,
		.irq		= IRQ_DOVE_UART_0,
		.flags		= UPF_SKIP_TEST | UPF_BOOT_AUTOCONF,
		.iotype		= UPIO_MEM,
		.regshift	= 2,
		.uartclk	= 0,
	}, {
	},
};

static struct resource dove_uart0_resources[] = {
	{
		.start		= DOVE_UART0_PHYS_BASE,
		.end		= DOVE_UART0_PHYS_BASE + SZ_256 - 1,
		.flags		= IORESOURCE_MEM,
	}, {
		.start		= IRQ_DOVE_UART_0,
		.end		= IRQ_DOVE_UART_0,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct platform_device dove_uart0 = {
	.name			= "serial8250",
	.id			= 0,
	.dev			= {
		.platform_data	= dove_uart0_data,
	},
	.resource		= dove_uart0_resources,
	.num_resources		= ARRAY_SIZE(dove_uart0_resources),
};

void __init dove_uart0_init(void)
{
	platform_device_register(&dove_uart0);
}

/*****************************************************************************
 * UART1
 ****************************************************************************/
static struct plat_serial8250_port dove_uart1_data[] = {
	{
		.mapbase	= DOVE_UART1_PHYS_BASE,
		.membase	= (char *)DOVE_UART1_VIRT_BASE,
		.irq		= IRQ_DOVE_UART_1,
		.flags		= UPF_SKIP_TEST | UPF_BOOT_AUTOCONF,
		.iotype		= UPIO_MEM,
		.regshift	= 2,
		.uartclk	= 0,
	}, {
	},
};

static struct resource dove_uart1_resources[] = {
	{
		.start		= DOVE_UART1_PHYS_BASE,
		.end		= DOVE_UART1_PHYS_BASE + SZ_256 - 1,
		.flags		= IORESOURCE_MEM,
	}, {
		.start		= IRQ_DOVE_UART_1,
		.end		= IRQ_DOVE_UART_1,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct platform_device dove_uart1 = {
	.name			= "serial8250",
	.id			= 1,
	.dev			= {
		.platform_data	= dove_uart1_data,
	},
	.resource		= dove_uart1_resources,
	.num_resources		= ARRAY_SIZE(dove_uart1_resources),
};

void __init dove_uart1_init(void)
{
	platform_device_register(&dove_uart1);
}

/*****************************************************************************
 * UART2
 ****************************************************************************/
static struct plat_serial8250_port dove_uart2_data[] = {
	{
		.mapbase	= DOVE_UART2_PHYS_BASE,
		.membase	= (char *)DOVE_UART2_VIRT_BASE,
		.irq		= IRQ_DOVE_UART_2,
		.flags		= UPF_SKIP_TEST | UPF_BOOT_AUTOCONF,
		.iotype		= UPIO_MEM,
		.regshift	= 2,
		.uartclk	= 0,
	}, {
	},
};

static struct resource dove_uart2_resources[] = {
	{
		.start		= DOVE_UART2_PHYS_BASE,
		.end		= DOVE_UART2_PHYS_BASE + SZ_256 - 1,
		.flags		= IORESOURCE_MEM,
	}, {
		.start		= IRQ_DOVE_UART_2,
		.end		= IRQ_DOVE_UART_2,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct platform_device dove_uart2 = {
	.name			= "serial8250",
	.id			= 2,
	.dev			= {
		.platform_data	= dove_uart2_data,
	},
	.resource		= dove_uart2_resources,
	.num_resources		= ARRAY_SIZE(dove_uart2_resources),
};

void __init dove_uart2_init(void)
{
	platform_device_register(&dove_uart2);
}

/*****************************************************************************
 * UART3
 ****************************************************************************/
static struct plat_serial8250_port dove_uart3_data[] = {
	{
		.mapbase	= DOVE_UART3_PHYS_BASE,
		.membase	= (char *)DOVE_UART3_VIRT_BASE,
		.irq		= IRQ_DOVE_UART_3,
		.flags		= UPF_SKIP_TEST | UPF_BOOT_AUTOCONF,
		.iotype		= UPIO_MEM,
		.regshift	= 2,
		.uartclk	= 0,
	}, {
	},
};

static struct resource dove_uart3_resources[] = {
	{
		.start		= DOVE_UART3_PHYS_BASE,
		.end		= DOVE_UART3_PHYS_BASE + SZ_256 - 1,
		.flags		= IORESOURCE_MEM,
	}, {
		.start		= IRQ_DOVE_UART_3,
		.end		= IRQ_DOVE_UART_3,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct platform_device dove_uart3 = {
	.name			= "serial8250",
	.id			= 3,
	.dev			= {
		.platform_data	= dove_uart3_data,
	},
	.resource		= dove_uart3_resources,
	.num_resources		= ARRAY_SIZE(dove_uart3_resources),
};

void __init dove_uart3_init(void)
{
	platform_device_register(&dove_uart3);
}

/*****************************************************************************
 * SPI0
 ****************************************************************************/
static struct orion_spi_info dove_spi0_data = {
	.tclk		= 0,
};

static struct resource dove_spi0_resources[] = {
	{
		.start	= DOVE_SPI0_PHYS_BASE,
		.end	= DOVE_SPI0_PHYS_BASE + SZ_512 - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= IRQ_DOVE_SPI0,
		.end	= IRQ_DOVE_SPI0,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device dove_spi0 = {
	.name		= "orion_spi",
	.id		= 0,
	.resource	= dove_spi0_resources,
	.dev		= {
		.platform_data	= &dove_spi0_data,
	},
	.num_resources	= ARRAY_SIZE(dove_spi0_resources),
};

void __init dove_spi0_init(int use_interrupt)
{
	dove_spi0_data.use_interrupt = use_interrupt;
	platform_device_register(&dove_spi0);
}

/*****************************************************************************
 * SPI1
 ****************************************************************************/
static struct orion_spi_info dove_spi1_data = {
	.tclk		= 0,
};

static struct resource dove_spi1_resources[] = {
	{
		.start	= DOVE_SPI1_PHYS_BASE,
		.end	= DOVE_SPI1_PHYS_BASE + SZ_512 - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= IRQ_DOVE_SPI1,
		.end	= IRQ_DOVE_SPI1,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device dove_spi1 = {
	.name		= "orion_spi",
	.id		= 1,
	.resource	= dove_spi1_resources,
	.dev		= {
		.platform_data	= &dove_spi1_data,
	},
	.num_resources	= ARRAY_SIZE(dove_spi1_resources),
};

void __init dove_spi1_init(int use_interrupt)
{
	dove_spi1_data.use_interrupt = use_interrupt;
	platform_device_register(&dove_spi1);
}

/*****************************************************************************
 * LCD SPI
 ****************************************************************************/
static struct mv_spi_info dove_lcd_spi_data = {
	.clk		= 400000000,
};

static struct resource dove_lcd_spi_resources[] = {
	{
		.start	= DOVE_LCD_PHYS_BASE + 0x10000,
		.end	= DOVE_LCD_PHYS_BASE + 0x10000 + SZ_512 - 1,
		.flags	= IORESOURCE_MEM,
	},
};

static struct platform_device dove_lcd_spi = {
	.name		= "mv_spi",
	.id		= 2,
	.dev		= {
		.platform_data	= &dove_lcd_spi_data,
	},
};

void __init dove_lcd_spi_init(void)
{
	platform_device_register(&dove_lcd_spi);
}

/*****************************************************************************
 * I2C
 ****************************************************************************/
static struct mv64xxx_i2c_pdata dove_i2c_data = {
	.freq_m		= 10, /* assumes 166 MHz TCLK gets 94.3kHz */
	.freq_n		= 3,
	.timeout	= 1000, /* Default timeout of 1 second */
};

static struct resource dove_i2c_resources[] = {
	{
		.name	= "i2c base",
		.start	= DOVE_I2C_PHYS_BASE,
		.end	= DOVE_I2C_PHYS_BASE + 0x20 -1,
		.flags	= IORESOURCE_MEM,
	}, {
		.name	= "i2c irq",
		.start	= IRQ_DOVE_I2C,
		.end	= IRQ_DOVE_I2C,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device dove_i2c = {
	.name		= MV64XXX_I2C_CTLR_NAME,
	.id		= 0,
	.num_resources	= ARRAY_SIZE(dove_i2c_resources),
	.resource	= dove_i2c_resources,
	.dev		= {
		.platform_data = &dove_i2c_data,
	},
};

void __init dove_i2c_init(void)
{
	platform_device_register(&dove_i2c);
}

/*****************************************************************************
 * Camera
 ****************************************************************************/
static struct resource dove_cam_resources[] = {
	{
		.start	= DOVE_CAM_PHYS_BASE,
		.end	= DOVE_CAM_PHYS_BASE + SZ_16K - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= IRQ_DOVE_CAM,
		.end	= IRQ_DOVE_CAM,
		.flags	= IORESOURCE_IRQ,
	},
};

static u64 dove_cam_dmamask = DMA_BIT_MASK(32);

static struct platform_device dove_cam = {
	.name			= "cafe1000-ccic",
	.id			= 0,
	.dev			= {
		.dma_mask		= &dove_cam_dmamask,
		.coherent_dma_mask = 0xFFFFFFFF,
	},
	.num_resources		= ARRAY_SIZE(dove_cam_resources),
	.resource		= dove_cam_resources,
};

void __init dove_cam_init(struct cafe_cam_platform_data *cafe_cam_data)
{
	dove_cam.dev.platform_data = cafe_cam_data;
	platform_device_register(&dove_cam);
}

/*****************************************************************************
 * SDIO and Camera mbus driver
 ****************************************************************************/
#define DOVE_SDHCI_CAM_MBUS_PHYS_BASE DOVE_CAFE_WIN_PHYS_BASE
static struct resource dove_sdhci_cam_mbus_resources[] = {
	{
		.start	= DOVE_SDHCI_CAM_MBUS_PHYS_BASE,
		.end	= DOVE_SDHCI_CAM_MBUS_PHYS_BASE + 64 - 1,
		.flags	= IORESOURCE_MEM,
	},
};

static struct platform_device dove_sdhci_cam_mbus = {
	.name			= "sdhci_cam_mbus",
	.id			= 0,
	.dev			= {
		.platform_data	= &dove_mbus_dram_info,
	},
	.num_resources		= ARRAY_SIZE(dove_sdhci_cam_mbus_resources),
	.resource		= dove_sdhci_cam_mbus_resources,
};

void __init dove_sdhci_cam_mbus_init(void)
{
	platform_device_register(&dove_sdhci_cam_mbus);
}

/*****************************************************************************
 * I2S/SPDIF
 ****************************************************************************/
static struct resource dove_i2s0_resources[] = {
	[0] = {
		.start  = DOVE_AUD0_PHYS_BASE,
		.end    = DOVE_AUD0_PHYS_BASE + SZ_16K -1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_DOVE_I2S0,
		.end    = IRQ_DOVE_I2S0,
		.flags  = IORESOURCE_IRQ,
	}
};

static u64 dove_i2s0_dmamask = 0xFFFFFFFFUL;

 
static struct platform_device dove_i2s0 = {
	.name           = "mv88fx_snd",
	.id             = 0,
	.dev            = {
		.dma_mask = &dove_i2s0_dmamask,
		.coherent_dma_mask = 0xFFFFFFFF,
	},
	.num_resources  = ARRAY_SIZE(dove_i2s0_resources),
	.resource       = dove_i2s0_resources,
};

static struct resource dove_i2s1_resources[] = {
	[0] = {
		.start  = DOVE_AUD1_PHYS_BASE,
		.end    = DOVE_AUD1_PHYS_BASE + SZ_16K -1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_DOVE_I2S1,
		.end    = IRQ_DOVE_I2S1,
		.flags  = IORESOURCE_IRQ,
	}
};

static u64 dove_i2s1_dmamask = 0xFFFFFFFFUL;

static struct platform_device dove_i2s1 = {
	.name           = "mv88fx_snd",
	.id             = 1,
	.dev            = {
		.dma_mask = &dove_i2s1_dmamask,
		.coherent_dma_mask = 0xFFFFFFFF,
	},
	.num_resources  = ARRAY_SIZE(dove_i2s1_resources),
	.resource       = dove_i2s1_resources,
};

static struct platform_device dove_mv88fx_i2s = {
	.name           = "mv88fx-i2s",
	.id             = -1,
};



void __init dove_i2s_init(int port, struct orion_i2s_platform_data *i2s_data)
{
	platform_device_register(&dove_mv88fx_i2s);
	switch(port){
	case 0:
		i2s_data->dram = &dove_mbus_dram_info;
		dove_i2s0.dev.platform_data = i2s_data;
		platform_device_register(&dove_i2s0);
		return;
	case 1:
		i2s_data->dram = &dove_mbus_dram_info;
		dove_i2s1.dev.platform_data = i2s_data;
		platform_device_register(&dove_i2s1);
		return;
	default:
		BUG();
	}
	
}

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

static void dove_clocks_set_axi_clock(u32 divider)
{
	dove_clocks_set_bits(DOVE_SB_REGS_VIRT_BASE + 0x000D0068, 10, 10, 1);
	dove_clocks_set_bits(DOVE_SB_REGS_VIRT_BASE + 0x000D0064, 1, 6, 
			     divider);
	dove_clocks_set_bits(DOVE_SB_REGS_VIRT_BASE + 0x000D0064, 7, 7, 1);
	udelay(1);
	dove_clocks_set_bits(DOVE_SB_REGS_VIRT_BASE + 0x000D0064, 7, 7, 0);
}

static u32 dove_clocks_get_gpu_clock(void)
{
	return dove_clocks_get_bits(DOVE_SB_REGS_VIRT_BASE + 0x000D0064, 8, 13);
}

static u32 dove_clocks_get_axi_clock(void)
{
	return dove_clocks_get_bits(DOVE_SB_REGS_VIRT_BASE + 0x000D0064, 1, 6);
}

static struct platform_device dove_clocks = {
	.name		= "dove_clocks",
	.id		= 0,
	.num_resources  = 0,
	.resource       = NULL,
};

static ssize_t dove_clocks_axi_show(struct kobject *kobj,
				   struct kobj_attribute *attr, char *buf)
{
	u32 divider;
	u32 c;

	divider = dove_clocks_get_axi_clock();
	c = dove_clocks_divide(2000, divider);

	return sprintf(buf, "%u (%u Mhz)\n", divider, c);
}

static ssize_t dove_clocks_axi_store(struct kobject *kobj,
				    struct kobj_attribute *attr,
				    const char *buf, size_t n)
{
	u32 value;

	if (sscanf(buf, "%u", &value) != 1)
		return -EINVAL;
	dove_clocks_set_axi_clock(value);
	return n;
}

static ssize_t dove_clocks_gpu_show(struct kobject *kobj,
				   struct kobj_attribute *attr, char *buf)
{
	u32 divider;
	u32 c;

	divider = dove_clocks_get_gpu_clock();
	c = dove_clocks_divide(2000, divider);

	return sprintf(buf, "%u (%u Mhz)\n", divider, c);
}

static ssize_t dove_clocks_gpu_store(struct kobject *kobj,
				struct kobj_attribute *attr,
				const char *buf, size_t n)
{
	u32 value;

	if (sscanf(buf, "%u", &value) != 1)
		return -EINVAL;
	dove_clocks_set_gpu_clock(value);
	return n;
}

static struct kobj_attribute dove_clocks_axi_attr =
	__ATTR(axi, 0644, dove_clocks_axi_show, dove_clocks_axi_store);

static struct kobj_attribute dove_clocks_gpu_attr =
	__ATTR(gpu, 0644, dove_clocks_gpu_show, dove_clocks_gpu_store);

void __init dove_clocks_config_gpu_clock(u32 clock)
{
	u32 divider;

	divider = dove_clocks_divide(2000, clock);
	printk(KERN_INFO "Setting gpu clock to %u MHz (divider: %u)\n",
		 clock, divider);
	dove_clocks_set_gpu_clock(divider);
}

void __init dove_clocks_config_axi_clock(u32 clock)
{
	u32 divider;

	divider = dove_clocks_divide(2000, clock);
	printk(KERN_INFO "Setting axi clock to %u MHz (divider: %u)\n",
		 clock, divider);
	dove_clocks_set_axi_clock(divider);
}

static int __init dove_clocks_init(void)
{
	dove_clocks_config_gpu_clock(250);

	dove_clocks_config_axi_clock(400);

	platform_device_register(&dove_clocks);
	if (sysfs_create_file(&dove_clocks.dev.kobj,
			&dove_clocks_axi_attr.attr))
		printk(KERN_ERR "%s: sysfs_create_file failed!", __func__);
	if (sysfs_create_file(&dove_clocks.dev.kobj,
			&dove_clocks_gpu_attr.attr))
		printk(KERN_ERR "%s: sysfs_create_file failed!", __func__);

	return 0;
}

/*****************************************************************************
 * VPU
 ****************************************************************************/
static struct resource dove_vpro_resources[] = {
	[0] = {
		.start	= DOVE_VPU_PHYS_BASE,
		.end	= DOVE_VPU_PHYS_BASE + 0x280000 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {		/* Place holder for reserved memory */
		.start	= 0,
		.end	= 0,
		.flags	= IORESOURCE_MEM,
	},
	[2] = {
		.start  = IRQ_DOVE_VPRO_DMA1,
		.end    = IRQ_DOVE_VPRO_DMA1,
		.flags  = IORESOURCE_IRQ,
	}
};

void __init dove_vpro_init(void)
{
#ifdef CONFIG_UIO_DOVE_VPRO
	if (vpro_size == 0) {
		printk("memory allocation for VPRO failed\n");
		return;
	}

	dove_vpro_resources[1].start = dove_vpro_memory_start;
	dove_vpro_resources[1].end = dove_vpro_memory_start + vpro_size - 1;

	platform_device_register_simple("dove_vpro_uio", 0,
			dove_vpro_resources,
			ARRAY_SIZE(dove_vpro_resources));
#endif
}

/*****************************************************************************
 * GPU
 ****************************************************************************/
static struct resource dove_gpu_resources[] = {
	{
		.name   = "gpu_irq",
		.start	= IRQ_DOVE_GPU,
		.end	= IRQ_DOVE_GPU,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name   = "gpu_base",
		.start	= DOVE_GPU_PHYS_BASE,
		.end	= DOVE_GPU_PHYS_BASE + 0x40000 - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name   = "gpu_mem",
		.start	= 0,
		.end	= 0,
		.flags	= IORESOURCE_MEM,
	},
};

static struct platform_device dove_gpu = {
	.name		= "dove_gpu",
	.id		= 0,
	.num_resources  = ARRAY_SIZE(dove_gpu_resources),
	.resource       = dove_gpu_resources,
};

void __init dove_gpu_init(void)
{
#ifdef CONFIG_DOVE_GPU
	if (gpu_size == 0) {
		printk("memory allocation for GPU failed\n");
		return;
	}

	dove_gpu_resources[2].start = dove_gpu_memory_start;
	dove_gpu_resources[2].end = dove_gpu_memory_start + gpu_size - 1;
	
	platform_device_register(&dove_gpu);
#endif
}

/*****************************************************************************
 * TACT switch
 ****************************************************************************/
static struct platform_device dove_tact = {
	.name			= "gpio_mouse",
	.id			= 0,
};

void __init dove_tact_init(struct gpio_mouse_platform_data *tact_data)
{
	dove_tact.dev.platform_data = tact_data;
	platform_device_register(&dove_tact);
}

/*****************************************************************************
 * Time handling
 ****************************************************************************/
int dove_tclk_get(void)
{
	/* tzachi: use DOVE_RESET_SAMPLE_HI/LO to detect tclk
	 * wait for spec, currently use hard code */
	return 166666667;
}

static void dove_timer_init(void)
{
	orion_time_init(IRQ_DOVE_BRIDGE, dove_tclk_get());
}

struct sys_timer dove_timer = {
	.init = dove_timer_init,
};

/*****************************************************************************
 * XOR 
 ****************************************************************************/
static struct mv_xor_platform_shared_data dove_xor_shared_data = {
	.dram		= &dove_mbus_dram_info,
};

/*****************************************************************************
 * XOR 0
 ****************************************************************************/
static u64 dove_xor0_dmamask = DMA_32BIT_MASK;

static struct resource dove_xor0_shared_resources[] = {
	{
		.name	= "xor 0 low",
		.start	= DOVE_XOR0_PHYS_BASE,
		.end	= DOVE_XOR0_PHYS_BASE + 0xff,
		.flags	= IORESOURCE_MEM,
	}, {
		.name	= "xor 0 high",
		.start	= DOVE_XOR0_HIGH_PHYS_BASE,
		.end	= DOVE_XOR0_HIGH_PHYS_BASE + 0xff,
		.flags	= IORESOURCE_MEM,
	},
};

static struct platform_device dove_xor0_shared = {
	.name		= MV_XOR_SHARED_NAME,
	.id		= 0,
	.dev		= {
		.platform_data = &dove_xor_shared_data,
	},
	.num_resources	= ARRAY_SIZE(dove_xor0_shared_resources),
	.resource	= dove_xor0_shared_resources,
};

static struct resource dove_xor00_resources[] = {
	[0] = {
		.start	= IRQ_DOVE_XOR_00,
		.end	= IRQ_DOVE_XOR_00,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct mv_xor_platform_data dove_xor00_data = {
	.shared		= &dove_xor0_shared,
	.hw_id		= 0,
	.pool_size	= PAGE_SIZE,
};

static struct platform_device dove_xor00_channel = {
	.name		= MV_XOR_NAME,
	.id		= 0,
	.num_resources	= ARRAY_SIZE(dove_xor00_resources),
	.resource	= dove_xor00_resources,
	.dev		= {
		.dma_mask		= &dove_xor0_dmamask,
		.coherent_dma_mask	= DMA_64BIT_MASK,
		.platform_data		= (void *)&dove_xor00_data,
	},
};

static struct resource dove_xor01_resources[] = {
	[0] = {
		.start	= IRQ_DOVE_XOR_01,
		.end	= IRQ_DOVE_XOR_01,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct mv_xor_platform_data dove_xor01_data = {
	.shared		= &dove_xor0_shared,
	.hw_id		= 1,
	.pool_size	= PAGE_SIZE,
};

static struct platform_device dove_xor01_channel = {
	.name		= MV_XOR_NAME,
	.id		= 1,
	.num_resources	= ARRAY_SIZE(dove_xor01_resources),
	.resource	= dove_xor01_resources,
	.dev		= {
		.dma_mask		= &dove_xor0_dmamask,
		.coherent_dma_mask	= DMA_64BIT_MASK,
		.platform_data		= (void *)&dove_xor01_data,
	},
};

void __init dove_xor0_init(void)
{
	platform_device_register(&dove_xor0_shared);

	/*
	 * two engines can't do memset simultaneously, this limitation
	 * satisfied by removing memset support from one of the engines.
	 */
	dma_cap_set(DMA_MEMCPY, dove_xor00_data.cap_mask);
	dma_cap_set(DMA_XOR, dove_xor00_data.cap_mask);
	platform_device_register(&dove_xor00_channel);

	dma_cap_set(DMA_MEMCPY, dove_xor01_data.cap_mask);
	dma_cap_set(DMA_MEMSET, dove_xor01_data.cap_mask);
	dma_cap_set(DMA_XOR, dove_xor01_data.cap_mask);
	platform_device_register(&dove_xor01_channel);
}

/*****************************************************************************
 * XOR 1
 ****************************************************************************/
static u64 dove_xor1_dmamask = DMA_32BIT_MASK;

static struct resource dove_xor1_shared_resources[] = {
	{
		.name	= "xor 0 low",
		.start	= DOVE_XOR1_PHYS_BASE,
		.end	= DOVE_XOR1_PHYS_BASE + 0xff,
		.flags	= IORESOURCE_MEM,
	}, {
		.name	= "xor 0 high",
		.start	= DOVE_XOR1_HIGH_PHYS_BASE,
		.end	= DOVE_XOR1_HIGH_PHYS_BASE + 0xff,
		.flags	= IORESOURCE_MEM,
	},
};

static struct platform_device dove_xor1_shared = {
	.name		= MV_XOR_SHARED_NAME,
	.id		= 1,
	.dev		= {
		.platform_data = &dove_xor_shared_data,
	},
	.num_resources	= ARRAY_SIZE(dove_xor1_shared_resources),
	.resource	= dove_xor1_shared_resources,
};

static struct resource dove_xor10_resources[] = {
	[0] = {
		.start	= IRQ_DOVE_XOR_10,
		.end	= IRQ_DOVE_XOR_10,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct mv_xor_platform_data dove_xor10_data = {
	.shared		= &dove_xor1_shared,
	.hw_id		= 0,
	.pool_size	= PAGE_SIZE,
};

static struct platform_device dove_xor10_channel = {
	.name		= MV_XOR_NAME,
	.id		= 2,
	.num_resources	= ARRAY_SIZE(dove_xor10_resources),
	.resource	= dove_xor10_resources,
	.dev		= {
		.dma_mask		= &dove_xor1_dmamask,
		.coherent_dma_mask	= DMA_64BIT_MASK,
		.platform_data		= (void *)&dove_xor10_data,
	},
};

static struct resource dove_xor11_resources[] = {
	[0] = {
		.start	= IRQ_DOVE_XOR_11,
		.end	= IRQ_DOVE_XOR_11,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct mv_xor_platform_data dove_xor11_data = {
	.shared		= &dove_xor1_shared,
	.hw_id		= 1,
	.pool_size	= PAGE_SIZE,
};

static struct platform_device dove_xor11_channel = {
	.name		= MV_XOR_NAME,
	.id		= 3,
	.num_resources	= ARRAY_SIZE(dove_xor11_resources),
	.resource	= dove_xor11_resources,
	.dev		= {
		.dma_mask		= &dove_xor1_dmamask,
		.coherent_dma_mask	= DMA_64BIT_MASK,
		.platform_data		= (void *)&dove_xor11_data,
	},
};

void __init dove_xor1_init(void)
{
	platform_device_register(&dove_xor1_shared);

	/*
	 * two engines can't do memset simultaneously, this limitation
	 * satisfied by removing memset support from one of the engines.
	 */
	dma_cap_set(DMA_MEMCPY, dove_xor10_data.cap_mask);
	dma_cap_set(DMA_XOR, dove_xor10_data.cap_mask);
	platform_device_register(&dove_xor10_channel);

	dma_cap_set(DMA_MEMCPY, dove_xor11_data.cap_mask);
	dma_cap_set(DMA_MEMSET, dove_xor11_data.cap_mask);
	dma_cap_set(DMA_XOR, dove_xor11_data.cap_mask);
	platform_device_register(&dove_xor11_channel);
}

/*****************************************************************************
 * General
 ****************************************************************************/
/*
 * This fixup function is used to reserve memory for the GPU and VPU engines
 * as these drivers require large chunks of consecutive memory.
 */
void __init dove_tag_fixup_mem32(struct machine_desc *mdesc, struct tag *t,
		char **from, struct meminfo *meminfo)
{
	struct tag *last_tag = NULL;
	int total_size = vpro_size + gpu_size;

	for (; t->hdr.size; t = tag_next(t))
		if ((t->hdr.tag == ATAG_MEM) && (t->u.mem.size >= total_size)) {
			if ((last_tag == NULL) ||
			    (t->u.mem.start > last_tag->u.mem.start))
				last_tag = t;
		}

	if (last_tag == NULL) {
		early_printk(KERN_WARNING "No suitable memory tag was found, "
				"required memory %d MB.\n", total_size);
		vpro_size = 0;
		gpu_size = 0;
		return;
	}

	/* Resereve memory from last tag for VPU usage.	*/
	last_tag->u.mem.size -= vpro_size;
	dove_vpro_memory_start = last_tag->u.mem.start + last_tag->u.mem.size;

	/* Reserve memory for gpu usage */
	last_tag->u.mem.size -= gpu_size;
	dove_gpu_memory_start = last_tag->u.mem.start + last_tag->u.mem.size;
}

void __init dove_ac97_setup(void)
{
	uint32_t reg;

	/* Enable AC97 Control 			*/
	reg = readl(DOVE_SB_REGS_VIRT_BASE + MPP_GENERAL_CONTROL_REG);
	if ((reg & 0x10000) == 0)
		printk("\nError: AC97 is disabled according to sample at reset "
			"option (Reg 0x%x).\n", MPP_GENERAL_CONTROL_REG);

	/* Set DCO clock to 24.576		*/
	reg = readl(DOVE_SB_REGS_VIRT_BASE + MV_AUDIO_DCO_CTRL_REG(0));
	reg = (reg & ~0x3) | 0x2;
	writel(reg, DOVE_SB_REGS_VIRT_BASE + MV_AUDIO_DCO_CTRL_REG(0));
}

void __init dove_config_arbitration(void)
{
	u32 sc_dec;

	sc_dec = readl(DOVE_MC_VIRT_BASE + 0x280);
	printk("PLiao: DOVE_MC @ 0x280 is %08X\n", sc_dec);
	sc_dec &= 0xfffff0ff;
	sc_dec |= 0x00000e00;
	writel(sc_dec, DOVE_MC_VIRT_BASE + 0x280);
        /*
        * Master 0 - VPro
        * Master 1 - GC500
        * Master 2 - LCD
        * Master 3 - Upstream (SB)
        */
        sc_dec = readl(DOVE_MC_VIRT_BASE + 0x510);
        printk("PLiao: DOVE_MC @ 0x510 is %08X\n", sc_dec);
	
	sc_dec &= 0xf0f0f0f0;
        sc_dec |= 0x010e0101;
        writel(sc_dec, DOVE_MC_VIRT_BASE + 0x510);
        /* End of supersection testing */

}
void __init dove_init(void)
{
	int tclk;

	tclk = dove_tclk_get();

	printk(KERN_INFO "Dove MV88F6781 SoC, ");
	printk("TCLK = %dMHz\n", (tclk + 499999) / 1000000);

	dove_setup_cpu_mbus();
	dove_config_arbitration();
#if defined(CONFIG_CACHE_TAUROS2)
	if (!noL2)
		tauros2_init();
#endif
	dove_clocks_init();


#if 1
	dove_ge00_shared_data.t_clk = tclk;
#endif
	dove_uart0_data[0].uartclk = tclk;
	dove_uart1_data[0].uartclk = tclk;
	dove_uart2_data[0].uartclk = tclk;
	dove_uart3_data[0].uartclk = tclk;
	dove_spi0_data.tclk = tclk;
	dove_spi1_data.tclk = tclk;
}

#ifdef CONFIG_PM
void dove_save_cpu_conf_regs(void)
{
	DOVE_DWNSTRM_BRDG_SAVE(CPU_CONFIG);
	DOVE_DWNSTRM_BRDG_SAVE(CPU_CTRL_STATUS);
	DOVE_DWNSTRM_BRDG_SAVE(CPU_RESET_MASK);
	DOVE_DWNSTRM_BRDG_SAVE(BRIDGE_MASK);
	DOVE_DWNSTRM_BRDG_SAVE(POWER_MANAGEMENT);
}

void dove_restore_cpu_conf_regs(void)
{
	DOVE_DWNSTRM_BRDG_RESTORE(CPU_CONFIG);
	DOVE_DWNSTRM_BRDG_RESTORE(CPU_CTRL_STATUS);
	DOVE_DWNSTRM_BRDG_RESTORE(CPU_RESET_MASK);
	DOVE_DWNSTRM_BRDG_RESTORE(BRIDGE_MASK);
	DOVE_DWNSTRM_BRDG_RESTORE(POWER_MANAGEMENT);
}

void dove_save_timer_regs(void)
{
	DOVE_DWNSTRM_BRDG_SAVE(TIMER_CTRL);
	DOVE_DWNSTRM_BRDG_SAVE(TIMER0_RELOAD);
	DOVE_DWNSTRM_BRDG_SAVE(TIMER1_RELOAD);
	DOVE_DWNSTRM_BRDG_SAVE(TIMER_WD_RELOAD);
}

void dove_restore_timer_regs(void)
{
	DOVE_DWNSTRM_BRDG_RESTORE(TIMER_CTRL);
	DOVE_DWNSTRM_BRDG_RESTORE(TIMER0_RELOAD);
	DOVE_DWNSTRM_BRDG_RESTORE(TIMER1_RELOAD);
	DOVE_DWNSTRM_BRDG_RESTORE(TIMER_WD_RELOAD);
}

void dove_save_int_regs(void)
{
	DOVE_DWNSTRM_BRDG_SAVE(MAIN_IRQ_MASK);
	DOVE_DWNSTRM_BRDG_SAVE(MAIN_FIQ_MASK);
	DOVE_DWNSTRM_BRDG_SAVE(MAIN2_IRQ_MASK);
	DOVE_DWNSTRM_BRDG_SAVE(MAIN2_FIQ_MASK);
	DOVE_DWNSTRM_BRDG_SAVE(ENDPOINT2_MASK);
	DOVE_DWNSTRM_BRDG_SAVE(PEX_0_1_MASK);
}

void dove_restore_int_regs(void)
{
	DOVE_DWNSTRM_BRDG_RESTORE(MAIN_IRQ_MASK);
	DOVE_DWNSTRM_BRDG_RESTORE(MAIN_FIQ_MASK);
	DOVE_DWNSTRM_BRDG_RESTORE(MAIN2_IRQ_MASK);
	DOVE_DWNSTRM_BRDG_RESTORE(MAIN2_FIQ_MASK);
	DOVE_DWNSTRM_BRDG_RESTORE(ENDPOINT2_MASK);
	DOVE_DWNSTRM_BRDG_RESTORE(PEX_0_1_MASK);
}
#endif
