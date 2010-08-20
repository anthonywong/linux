/*
 * Marvell bindings for Secure Digital Host Controller Interface.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */
//#define DEBUG
#include <linux/module.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/mmc/host.h>
#include <linux/platform_device.h>
#include <linux/dove_sdhci.h>
#include <mach/gpio.h>
#include "sdhci.h"

#define DRIVER_NAME "sdhci-mv"

struct sdhci_mv_host {
        /* for dove card interrupt workaround */
        int                     dove_card_int_wa;
        struct sdhci_dove_int_wa dove_int_wa_info;
};
#ifdef CONFIG_PM

static int sdhci_mv_suspend(struct platform_device *pdev, pm_message_t state)
{
//	struct sdhci_host *host = dev_get_drvdata(&pdev->dev);

//	return mmc_suspend_host(host->mmc, state);
	return 0;
}

static int sdhci_mv_resume(struct platform_device *pdev)
{
//	struct sdhci_host *host = dev_get_drvdata(&pdev->dev);

//	return mmc_resume_host(host->mmc);
	return 0;
}

#else

#define sdhci_mv_suspend NULL
#define sdhci_mv_resume NULL

#endif
static void sdhci_sdio_gpio_irq_enable(struct sdhci_host *host);
static void sdhci_sdio_gpio_irq_disable(struct sdhci_host *host);

static struct sdhci_ops sdhci_mv_ops = {
	.gpio_irq_enable = sdhci_sdio_gpio_irq_enable,
	.gpio_irq_disable = sdhci_sdio_gpio_irq_disable,
};

static irqreturn_t sdhci_dove_gpio_irq(int irq, void *dev_id)
{
	struct sdhci_host* host = dev_id;
#ifdef VERBOSE
	DBG("*** %s got gpio interrupt\n",
		mmc_hostname(host->mmc));
#endif

#ifdef VERBOSE
	sdhci_dumpregs(host);
#endif
	mmc_signal_sdio_irq(host->mmc);

	return IRQ_HANDLED;
}


static void sdhci_enable_sdio_gpio_irq(struct mmc_host *mmc, int enable)
{
	struct sdhci_host *host = mmc_priv(mmc);
	struct sdhci_mv_host *mv_host = sdhci_priv(host);
	unsigned long flags;
	struct sdhci_dove_int_wa *wa_info;
	
	wa_info = &mv_host->dove_int_wa_info;
	spin_lock_irqsave(&host->lock, flags);

	if (enable) {
		if (wa_info->status == 0) {
			enable_irq(wa_info->irq);
			wa_info->status = 1;
		}
	} else {
		if (wa_info->status == 1) {
			disable_irq(wa_info->irq);
			wa_info->status = 0;
		}
	}
	
	mmiowb();

	spin_unlock_irqrestore(&host->lock, flags);
}

static void sdhci_sdio_gpio_irq_enable(struct sdhci_host *host)
{
	struct sdhci_mv_host *mv_host = sdhci_priv(host);
	u32 func_gpio_select;
	if (!mv_host->dove_card_int_wa)
		return;

	func_gpio_select = readl(DOVE_FUNC_GPIO_SELECT_VIRT_BASE);
	func_gpio_select |= mv_host->dove_int_wa_info.func_select_bit << 1;
	writel(func_gpio_select, DOVE_FUNC_GPIO_SELECT_VIRT_BASE);

	mmiowb();
}

static void sdhci_sdio_gpio_irq_disable(struct sdhci_host *host)
{
	struct sdhci_mv_host *mv_host = sdhci_priv(host);
	u32 func_gpio_select;

	if (!mv_host->dove_card_int_wa)
		return;

	func_gpio_select = readl(DOVE_FUNC_GPIO_SELECT_VIRT_BASE);
	func_gpio_select &= ~(mv_host->dove_int_wa_info.func_select_bit << 1);
	writel(func_gpio_select, DOVE_FUNC_GPIO_SELECT_VIRT_BASE);

	mmiowb();
}

static void sdhci_sdio_gpio_irq_dump(struct sdhci_host *host)
{
	struct sdhci_mv_host *mv_host = sdhci_priv(host);
	int gpio;

	if (!mv_host->dove_card_int_wa)
		return;
	
	gpio = mv_host->dove_int_wa_info.gpio;

	printk(" dump gpio irq regs:\n");
	printk(" select [0x%x] 0x%08x\n", DOVE_FUNC_GPIO_SELECT_VIRT_BASE,
	       readl(DOVE_FUNC_GPIO_SELECT_VIRT_BASE));
	printk(" gpio level [0x%x] 0x%08x\n", GPIO_LEVEL_MASK(gpio),
	       readl(GPIO_LEVEL_MASK(gpio)));
	printk(" gpio pol [0x%x] 0x%08x\n", GPIO_IN_POL(gpio),
	       readl(GPIO_IN_POL(gpio)));
	printk(" gpio data in [0x%x] 0x%08x\n", GPIO_DATA_IN(gpio),
	       readl(GPIO_DATA_IN(gpio)));
}

static int __devinit sdhci_mv_probe(struct platform_device *pdev)
{
	struct sdhci_host *host;
	struct sdhci_mv_host *mv_host;
	struct resource *r;
	struct sdhci_dove_int_wa *data = (struct sdhci_dove_int_wa *)
		pdev->dev.platform_data;
	int ret;

	host = sdhci_alloc_host(&pdev->dev, sizeof(*mv_host));
	if (IS_ERR(host)) {
		dev_err(&pdev->dev, "failed to alloc sdhci host\n");
                return -ENOMEM;
        }
	mv_host = sdhci_priv(host);
	dev_set_drvdata(&pdev->dev, host);
	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
        if (!r) {
		goto err_get_resource;
                ret = -ENXIO;
	}

        host->irq = platform_get_irq(pdev, 0);
        if (host->irq < 0) {
		goto err_get_resource;
                ret = -ENXIO;
	}
        r = devm_request_mem_region(&pdev->dev, r->start, SZ_4K, DRIVER_NAME);
	if (!r) {
		dev_err(&pdev->dev, "cannot request region\n");
		ret = -EINVAL;
                goto err_request_region;
	}

	dev_dbg(&pdev->dev, "r->start=0x%x\n", r->start);

        host->ioaddr = devm_ioremap(&pdev->dev, r->start, SZ_4K);

	dev_dbg(&pdev->dev, "slot 0 at 0x%p, irq %d\n", host->ioaddr, host->irq);

        if (!host->ioaddr) {
                ret = -EINVAL;
                goto err_ioremap;
        }

	host->hw_name = dev_name(&pdev->dev);
	host->ops = &sdhci_mv_ops;
	host->quirks = SDHCI_QUIRK_NO_SIMULT_VDD_AND_POWER |
		SDHCI_QUIRK_NO_BUSY_IRQ |
		SDHCI_QUIRK_BROKEN_TIMEOUT_VAL |
		SDHCI_QUIRK_PIO_USE_WORD_ACCESS |
		SDHCI_QUIRK_HIGH_SPEED_WA;
	
        if (data) {
		dev_dbg(&pdev->dev, " request wa irq\n");
                mv_host->dove_card_int_wa = 1;
                mv_host->dove_int_wa_info.irq = data->irq;
                mv_host->dove_int_wa_info.gpio = data->gpio;
                mv_host->dove_int_wa_info.func_select_bit = data->func_select_bit;
                mv_host->dove_int_wa_info.status = 0; //disabled
		ret = devm_request_irq(&pdev->dev, mv_host->dove_int_wa_info.irq,
				       sdhci_dove_gpio_irq, IRQF_SHARED,
				       mmc_hostname(host->mmc), host);
		if(ret) {
			dev_err(&pdev->dev, "cannot request wa irq\n");
                        goto err_request_irq;
		}

                /* to balance disable/enable_irq */
                disable_irq(mv_host->dove_int_wa_info.irq);

        } else {
		dev_dbg(&pdev->dev, " no request wa irq\n");
		mv_host->dove_card_int_wa = 0;
	}

	ret = sdhci_add_host(host);
	if (ret)
		goto err_add_host;
	if (mv_host->dove_card_int_wa)
		host->mmc->ops->enable_sdio_irq = sdhci_enable_sdio_gpio_irq;
	return 0;

err_add_host:
err_request_irq:
err_ioremap:
err_request_region:
err_get_resource:
	sdhci_free_host(host);
	return ret;
}

static int __devexit sdhci_mv_remove(struct platform_device *pdev)
{
	struct sdhci_host *host = dev_get_drvdata(&pdev->dev);

	sdhci_remove_host(host, 0);
	sdhci_free_host(host);
	return 0;
}

static struct platform_driver sdhci_mv_driver = {
	.probe		= sdhci_mv_probe,
	.remove		= __devexit_p(sdhci_mv_remove),
	.suspend	= sdhci_mv_suspend,
	.resume		= sdhci_mv_resume,
	.driver		= {
		.name	= "sdhci-mv",
		.owner	= THIS_MODULE,
	},
};

static int __init sdhci_mv_init(void)
{
	return platform_driver_register(&sdhci_mv_driver);
}
module_init(sdhci_mv_init);

static void __exit sdhci_mv_exit(void)
{
	platform_driver_unregister(&sdhci_mv_driver);
}
module_exit(sdhci_mv_exit);

MODULE_DESCRIPTION("Secure Digital Host Controller Interface MV driver");
MODULE_AUTHOR("Saeed Bishara <saeed@marvell.com>");
MODULE_LICENSE("GPL");
