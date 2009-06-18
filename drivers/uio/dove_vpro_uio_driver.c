/*
 * driver/uio/dove_vpro_uio_driver.c
 */

#include <linux/uio_driver.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/io.h>

#include "dove_vpro_uio_driver.h"

/* local control  */
struct vpro_uio_data {
	long			freq;
	long			dove_uio_count;
	struct timer_list 	vpro_timer;
	struct uio_info		uio_info;
};

static int vpro_ioctl(struct uio_info *info, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch(cmd) {
		default:
			printk("vpro_ioctl\n");
			break;
	}

	return ret;
}


static ssize_t show_count(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct vpro_uio_data *vd  = (struct vpro_uio_data *)dev->driver_data;
	return sprintf(buf, "%ld\n", vd->dove_uio_count);
}

static ssize_t store_count(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct vpro_uio_data *vd  = (struct vpro_uio_data *)dev->driver_data;
	vd->dove_uio_count = simple_strtol(buf, NULL, 10);
	return count;
}
static DEVICE_ATTR(count, S_IRUGO|S_IWUSR|S_IWGRP, show_count, store_count);

static ssize_t show_freq(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct vpro_uio_data *vd  = (struct vpro_uio_data *)dev->driver_data;
	return sprintf(buf, "%ld\n", vd->freq);
}

static ssize_t store_freq(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct vpro_uio_data *vd  = (struct vpro_uio_data *)dev->driver_data;

	long tmp = simple_strtol(buf, NULL, 10);
	if(tmp < 1)
		tmp = 1;
	vd->freq = tmp;

	return count;
}
static DEVICE_ATTR(freq, S_IRUGO|S_IWUSR|S_IWGRP, show_freq, store_freq);

static void dove_vpro_timer(unsigned long data)
{
	struct vpro_uio_data *vd = (struct vpro_uio_data *)data;

	/*
	struct uio_info *info = (struct uio_info*)data;
	unsigned long *addr = (unsigned long*)info->mem[0].internal_addr;
	unsigned long *addr1 = (unsigned long*)info->mem[1].internal_addr;
	*/
	vd->dove_uio_count++;
	/*
	*addr = dove_uio_count;
	iowrite32(dove_uio_count, addr);
	iowrite32(dove_uio_count, addr1);
	*/
	uio_event_notify(&vd->uio_info);
	mod_timer(&vd->vpro_timer, jiffies + vd->freq);
}

static int dove_vpro_probe(struct platform_device *pdev)
{
	int id, ret = -ENODEV;
	unsigned long start, size;
	struct resource *res;
	struct vpro_uio_data *vd;

	printk(KERN_INFO "Registering VPro UIO driver:.\n");

	vd = kzalloc(sizeof(struct vpro_uio_data), GFP_KERNEL);
	if (vd == NULL) {
		printk(KERN_ERR "vdec_prvdec_probe: "
				"Failed to allocate memory.\n");
		return -ENOMEM;
	}

	/* Get internal registers memory. */
	id = VPRO_CONTROL_REGISTER_MAP;
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		printk(KERN_ERR "dove_vpro_probe: "
				"No registers memory supplied.\n");
		goto uio_register_fail;
	}
	vd->uio_info.mem[id].internal_addr =
		(void __iomem *)ioremap(res->start, res->end - res->start + 1);
	vd->uio_info.mem[id].addr = res->start;
	vd->uio_info.mem[id].size = res->end - res->start + 1;
	vd->uio_info.mem[id].memtype = UIO_MEM_PHYS;

	printk(KERN_INFO "  o Mapping registers at 0x%x Size %ld KB.\n",
			res->start, vd->uio_info.mem[id].size >> 10);
	/* Get VPRO reserved memory area. */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (res == NULL) {
		printk(KERN_ERR "dove_vpro_probe: "
				"No VPRO memory supplied.\n");
		goto uio_register_fail;
	}

	id = VPRO_DMA_BUFFER_MAP_1;
	size = VPRO_DMA_BUFFER_1_SIZE;
	start = res->start;
	vd->uio_info.mem[id].internal_addr =
		(void __iomem *)ioremap_nocache(start, size);
	vd->uio_info.mem[id].addr = start;
	vd->uio_info.mem[id].size = VPRO_DMA_BUFFER_1_SIZE;
	vd->uio_info.mem[id].memtype = UIO_MEM_PHYS;

	printk(KERN_INFO "  o Mapping buffer #1 at %ld MB Size %ld MB.\n",
			start >> 20, vd->uio_info.mem[id].size >> 20);

	id = VPRO_DMA_BUFFER_MAP_2;
	start += VPRO_DMA_BUFFER_1_SIZE;
	size = res->end - res->start - VPRO_DMA_BUFFER_1_SIZE;
	vd->uio_info.mem[id].internal_addr =
		(void __iomem *)ioremap_nocache(start, size);
	vd->uio_info.mem[id].addr = start;
	vd->uio_info.mem[id].size = size;
	vd->uio_info.mem[id].memtype = UIO_MEM_PHYS;
	printk(KERN_INFO "  o Mapping buffer #2 at %ld MB Size %ld MB.\n",
			start >> 20, vd->uio_info.mem[id].size >> 20);

	platform_set_drvdata(pdev, vd);

	vd->freq = VPRO_TIMER_FREQ;

	vd->uio_info.name = "dove_vpro_uio";
	vd->uio_info.version = "0.0.0";
	vd->uio_info.irq = UIO_IRQ_CUSTOM;
	vd->uio_info.ioctl = vpro_ioctl;

	if (uio_register_device(&pdev->dev, &vd->uio_info)) {
		ret = -ENODEV;
		goto uio_register_fail;
	}

	ret = device_create_file(&pdev->dev, &dev_attr_count);
	if(ret)
		goto fail_create_attr_count;
	
	ret = device_create_file(&pdev->dev, &dev_attr_freq);
	if(ret)
		goto fail_create_attr_freq;

	init_timer(&vd->vpro_timer);
	vd->vpro_timer.data = (unsigned long)vd;
	vd->vpro_timer.function = dove_vpro_timer;
	mod_timer(&vd->vpro_timer, jiffies + vd->freq);

	printk(KERN_INFO "VPRO UIO driver registered successfully.\n");
	return 0;

fail_create_attr_freq:
	device_remove_file(&pdev->dev, &dev_attr_count);
fail_create_attr_count:
	uio_unregister_device(&vd->uio_info);
uio_register_fail:
	if ((id == VPRO_DMA_BUFFER_MAP_1) || (id == VPRO_DMA_BUFFER_MAP_2))
		iounmap(
		vd->uio_info.mem[VPRO_CONTROL_REGISTER_MAP].internal_addr);
	kfree(vd);

	printk(KERN_INFO "Failed to register VPRO uio driver.\n");
	return ret;
}


static int dove_vpro_remove(struct platform_device *pdev)
{
	struct vpro_uio_data *vd = platform_get_drvdata(pdev);

	del_timer_sync(&vd->vpro_timer);
	device_remove_file(&pdev->dev, &dev_attr_freq);
	device_remove_file(&pdev->dev, &dev_attr_count);

	uio_unregister_device(&vd->uio_info);
	iounmap(vd->uio_info.mem[VPRO_CONTROL_REGISTER_MAP].internal_addr);
	memset(vd->uio_info.mem, 0, sizeof(vd->uio_info.mem));

	kfree(vd);

	return 0;
}

static void dove_vpro_shutdown(struct platform_device *pdev)
{
	return;
}

#ifdef CONFIG_PM
static int dove_vpro_suspend(struct platform_device *dev, pm_message_t state)
{
	return 0;
}

static int dove_vpro_resume(struct platform_device *dev)
{
	return 0;
}
#endif

static struct platform_driver vpro_driver = {
	.probe		= dove_vpro_probe,
	.remove		= dove_vpro_remove,
	.shutdown	= dove_vpro_shutdown,
#ifdef CONFIG_PM
	.suspend	= dove_vpro_suspend,
	.resume		= dove_vpro_resume,
#endif
	.driver = {
		.name	= "dove_vpro_uio",
		.owner	= THIS_MODULE,
	},
};

static int __init dove_vpro_init(void)
{
	return platform_driver_register(&vpro_driver);
}

static void __exit dove_vpro_exit(void)
{
	platform_driver_unregister(&vpro_driver);
}

module_init(dove_vpro_init);
module_exit(dove_vpro_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Dove vPro UIO driver");

