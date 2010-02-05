/*
 * driver/uio/dove_vmeta_uio_driver.c
 */

#include <linux/uio_driver.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/notifier.h>
#include <linux/suspend.h>
#include <linux/io.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>

#include "dove_vmeta_uio_driver.h"

/* local control  */
struct vmeta_xv_video_queue {
	struct list_head list;
	struct vmeta_xv_frame frame_info;
};

struct vmeta_xv_data {
	struct mutex lock;			// for vmeta xv enhancement
	struct vmeta_xv_video_queue iqueue;	// for vmeta xv enhancement
	struct vmeta_xv_video_queue oqueue;	// for vmeta xv enhancement
};

struct vmeta_uio_data {
	struct uio_info		uio_info;
	struct clk		*vmeta_clk;
};
static struct vmeta_uio_data *vmeta_uio_priv_data;

#ifndef CONFIG_MEM_FOR_MULTIPROCESS
static atomic_t vmeta_available = ATOMIC_INIT(1);
#else /* CONFIG_MEM_FOR_MULTIPROCESS */
static atomic_t vmeta_available = ATOMIC_INIT(0);
#endif /* CONFIG_MEM_FOR_MULTIPROCESS */

static atomic_t vmeta_pm_suspend_available = ATOMIC_INIT(0);
static atomic_t vmeta_pm_suspend_successful = ATOMIC_INIT(0);
static atomic_t vmeta_pm_suspend_watch_dog_trig = ATOMIC_INIT(0);
struct mutex vmeta_pm_suspend_check;
struct timer_list vmeta_pm_watch_dog;

static void vmeta_pm_watch_dog_timer(unsigned long data)
{
	if(vmeta_pm_suspend_successful.counter == 0) {
		if(mutex_is_locked(&vmeta_pm_suspend_check)) {
			mutex_unlock(&vmeta_pm_suspend_check);
			atomic_set(&vmeta_pm_suspend_watch_dog_trig, 1);
		}
	}
}

static int vmeta_pm_event(struct notifier_block *notifier, unsigned long val, void *v)
{
	/* Hibernation and suspend events */
	// PM_HIBERNATION_PREPARE	0x0001 /* Going to hibernate */
	// PM_POST_HIBERNATION	0x0002 /* Hibernation finished */
	// PM_SUSPEND_PREPARE	0x0003 /* Going to suspend the system */
	// PM_POST_SUSPEND		0x0004 /* Suspend finished */
	// PM_RESTORE_PREPARE	0x0005 /* Going to restore a saved image */
	// PM_POST_RESTORE		0x0006 /* Restore failed */

	uio_event_notify(&vmeta_uio_priv_data->uio_info);

	switch(val) {
		case PM_SUSPEND_PREPARE:
			atomic_set(&vmeta_pm_suspend_available, 1);
#ifndef CONFIG_MEM_FOR_MULTIPROCESS
			if(vmeta_available.counter == 0) {
#else /* CONFIG_MEM_FOR_MULTIPROCESS */
			if(vmeta_available.counter >= 1) {
#endif /* CONFIG_MEM_FOR_MULTIPROCESS */
				init_timer(&vmeta_pm_watch_dog);
				vmeta_pm_watch_dog.function = vmeta_pm_watch_dog_timer;
				mod_timer(&vmeta_pm_watch_dog, jiffies + HZ);

				atomic_set(&vmeta_pm_suspend_successful, 0);
				mutex_lock(&vmeta_pm_suspend_check);
				if(vmeta_pm_suspend_watch_dog_trig.counter == 1) {
					atomic_set(&vmeta_pm_suspend_watch_dog_trig, 0);
					if(!mutex_is_locked(&vmeta_pm_suspend_check))
						mutex_lock(&vmeta_pm_suspend_check);	// lock as default

					return NOTIFY_BAD;
				} else
					atomic_set(&vmeta_pm_suspend_successful, 1);
			}
			break;
		case PM_POST_SUSPEND:
			atomic_set(&vmeta_pm_suspend_available, 0);
			if(!mutex_is_locked(&vmeta_pm_suspend_check))
				mutex_lock(&vmeta_pm_suspend_check);	// lock as default
			break;
	};

	return NOTIFY_OK;
}

static struct notifier_block vmeta_pm_notifier = {
	.notifier_call = vmeta_pm_event,
	.priority = -5,
};

static int vmeta_open(struct uio_info *info, struct inode *inode)
{
#ifndef CONFIG_MEM_FOR_MULTIPROCESS
	if (!atomic_dec_and_test(&vmeta_available)) {
		atomic_inc(&vmeta_available);
		return -EBUSY;	/* already open */
	}
	clk_enable(vmeta_uio_priv_data->vmeta_clk);
#else /* CONFIG_MEM_FOR_MULTIPROCESS */
	if (atomic_add_return(1, &vmeta_available) == 1)
		clk_enable(vmeta_uio_priv_data->vmeta_clk);
#endif /* CONFIG_MEM_FOR_MULTIPROCESS */
	return 0;
}

static int vmeta_release(struct uio_info *info, struct inode *inode)
{
#ifndef CONFIG_MEM_FOR_MULTIPROCESS
	atomic_inc(&vmeta_available); /* release the device */
	clk_disable(vmeta_uio_priv_data->vmeta_clk);
#else /* CONFIG_MEM_FOR_MULTIPROCESS */
	if (atomic_dec_and_test(&vmeta_available))
		clk_disable(vmeta_uio_priv_data->vmeta_clk);
#endif /* CONFIG_MEM_FOR_MULTIPROCESS */
	return 0;
}

static int vmeta_mmap(struct uio_info *info, struct vm_area_struct *vma)
{
	struct uio_device *idev = vma->vm_private_data;
	int mi;

	for (mi = 0; mi < MAX_UIO_MAPS; mi++) {
		if (info->mem[mi].size == 0) {
			mi = -1;
			break;
		}
		if (vma->vm_pgoff == mi)
			break;
	}

	if (mi < 0)
		return -EINVAL;

	vma->vm_flags |= VM_IO | VM_RESERVED;

#if defined(CONFIG_ARCH_DOVE)
	if(mi == VMETA_CONTROL_REGISTER_MAP)
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	else if(mi == VMETA_DMA_BUFFER_MAP)
		vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
#else
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
#endif
	return remap_pfn_range(vma,
			       vma->vm_start,
			       info->mem[mi].addr >> PAGE_SHIFT,
			       vma->vm_end - vma->vm_start,
			       vma->vm_page_prot);
}

static int vmeta_ioctl(struct uio_info *info, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	struct vmeta_xv_data *xv_data = (struct vmeta_xv_data*)info->priv;

	switch(cmd) {
		case UIO_VMETA_IRQ_ENABLE:
			enable_irq(info->irq);
			break;
		case UIO_VMETA_IRQ_DISABLE:
			disable_irq(info->irq);
			break;
		case UIO_VMETA_XV_IN_QUEUE: {
			struct vmeta_xv_frame video_frame;
			struct vmeta_xv_video_queue *video_queue_data;

			__copy_from_user(&(video_frame), (int __user*)arg, sizeof(struct vmeta_xv_frame));
			video_queue_data = kzalloc(sizeof(struct vmeta_xv_video_queue), GFP_KERNEL);
			video_queue_data->frame_info.phy_addr = video_frame.phy_addr;
			video_queue_data->frame_info.size =  video_frame.size;

			mutex_lock(&xv_data->lock);
			list_add(&video_queue_data->list, &xv_data->iqueue.list);
			mutex_unlock(&xv_data->lock);
			}
			break;
		case UIO_VMETA_XV_DQUEUE: {
			struct vmeta_xv_frame video_frame;

			if (!list_empty(&xv_data->oqueue.list)) {
				struct vmeta_xv_video_queue *video_queue_data;
				
				mutex_lock(&xv_data->lock);
				video_queue_data = list_first_entry(&xv_data->oqueue.list, struct vmeta_xv_video_queue, list);
				video_frame.phy_addr = video_queue_data->frame_info.phy_addr;
				video_frame.size = video_queue_data->frame_info.size;
				list_del(&video_queue_data->list);
				kfree(video_queue_data);
				mutex_unlock(&xv_data->lock);
			} else {
				video_frame.phy_addr = 0;
				video_frame.size = 0;
			}

			__copy_to_user((int __user*)arg, &video_frame, sizeof(struct vmeta_xv_frame));
			}
			break;
		case UIO_VMETA_XV_QUERY_VIDEO: {
			struct vmeta_xv_frame video_frame;

			if (!list_empty(&xv_data->iqueue.list)) {
				struct vmeta_xv_video_queue *video_queue_data;
				
				mutex_lock(&xv_data->lock);
				video_queue_data = list_first_entry(&xv_data->iqueue.list, struct vmeta_xv_video_queue, list);
				video_frame.phy_addr = video_queue_data->frame_info.phy_addr;
				video_frame.size = video_queue_data->frame_info.size;
				list_del(&video_queue_data->list);
				kfree(video_queue_data);
				mutex_unlock(&xv_data->lock);
			} else {
				video_frame.phy_addr = 0;
				video_frame.size = 0;
			}

			__copy_to_user((int __user*)arg, &video_frame, sizeof(struct vmeta_xv_frame));
			}
			break;
		case UIO_VMETA_XV_FREE_VIDEO: {
			struct vmeta_xv_frame video_frame;
			struct vmeta_xv_video_queue *video_queue_data;

			__copy_from_user(&(video_frame), (int __user*)arg, sizeof(struct vmeta_xv_frame));
			video_queue_data = kzalloc(sizeof(struct vmeta_xv_video_queue), GFP_KERNEL);
			video_queue_data->frame_info.phy_addr = video_frame.phy_addr;
			video_queue_data->frame_info.size = video_frame.size;

			mutex_lock(&xv_data->lock);
			list_add(&video_queue_data->list, &xv_data->oqueue.list);
			mutex_unlock(&xv_data->lock);
			}
			break;
		case UIO_VMETA_XV_INIT_QUEUE: {
			mutex_lock(&xv_data->lock);
			while (!list_empty(&xv_data->iqueue.list)) {
				struct vmeta_xv_video_queue *iqueue_data;
				
				iqueue_data = list_first_entry(&xv_data->iqueue.list, struct vmeta_xv_video_queue, list);
				list_del(&iqueue_data->list);
				kfree(iqueue_data);
				printk(KERN_INFO "warning: [vmeta uio driver] Here is a xv frame buffer not be free in iqueue.\n");
			}
			while (!list_empty(&xv_data->oqueue.list)) {
				struct vmeta_xv_video_queue *oqueue_data;
				
				oqueue_data = list_first_entry(&xv_data->oqueue.list, struct vmeta_xv_video_queue, list);
				list_del(&oqueue_data->list);
				kfree(oqueue_data);
				printk(KERN_INFO "warning: [vmeta uio driver] Here is a xv     frame buffer not be free in oqueue.\n");
			}
			INIT_LIST_HEAD(&xv_data->iqueue.list);
			INIT_LIST_HEAD(&xv_data->oqueue.list);
			mutex_unlock(&xv_data->lock);
			}
			break;
		case UIO_VMETA_POWER_OFF:
			clk_disable(vmeta_uio_priv_data->vmeta_clk);
			break;
		case UIO_VMETA_POWER_ON:
			clk_enable(vmeta_uio_priv_data->vmeta_clk);
			break;
		case UIO_VMETA_SUSPEND_CHECK: {
			int vmeta_suspend_check = 0;

			if(vmeta_pm_suspend_available.counter)
				vmeta_suspend_check = 1;
				__copy_to_user((int __user*)arg, &vmeta_suspend_check, sizeof(int));
			}
			break;
		case UIO_VMETA_SUSPEND_READY:
			if(mutex_is_locked(&vmeta_pm_suspend_check))
				mutex_unlock(&vmeta_pm_suspend_check);
			break;
		default:
			break;
	}

	return ret;
}

static irqreturn_t vmeta_irqhandler(int irq, void *dev_id)
{
	disable_irq_nosync(irq);
	return IRQ_HANDLED;
}

static int dove_vmeta_probe(struct platform_device *pdev)
{
	int id, ret = -ENODEV;
	unsigned long start, size;
	struct resource *res;
	struct vmeta_uio_data *vd;
	struct vmeta_xv_data *xvd;

#ifdef CONFIG_MEM_FOR_MULTIPROCESS
	dma_addr_t mem_dma_addr;
	void *mem_vir_addr;
#endif
	printk(KERN_INFO "Registering VMETA UIO driver:.\n");

	vd = kzalloc(sizeof(struct vmeta_uio_data), GFP_KERNEL);
	if (vd == NULL) {
		printk(KERN_ERR "vdec_prvdec_probe: "
				"Failed to allocate memory.\n");
		return -ENOMEM;
	} else
		vmeta_uio_priv_data = vd;

	/* Get internal registers memory. */
	id = VMETA_CONTROL_REGISTER_MAP;
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		printk(KERN_ERR "dove_vmeta_probe: "
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
	/* Get VMETA reserved memory area. */
#ifndef CONFIG_VMETA_NEW
	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (res == NULL) {
		printk(KERN_ERR "dove_vmeta_probe: "
				"No VMETA memory supplied.\n");
		goto uio_register_fail;
	}

	id = VMETA_DMA_BUFFER_MAP_1;
	size = VMETA_DMA_BUFFER_1_SIZE;
	start = res->start;
	vd->uio_info.mem[id].internal_addr =
		(void __iomem *)ioremap_nocache(start, size);
	vd->uio_info.mem[id].addr = start;
	vd->uio_info.mem[id].size = VMETA_DMA_BUFFER_1_SIZE;
	vd->uio_info.mem[id].memtype = UIO_MEM_PHYS;

	printk(KERN_INFO "  o Mapping buffer #1 at %ld MB Size %ld MB.\n",
			start >> 20, vd->uio_info.mem[id].size >> 20);

	id = VMETA_DMA_BUFFER_MAP_2;
	start += VMETA_DMA_BUFFER_1_SIZE;
	size = res->end - res->start - VMETA_DMA_BUFFER_1_SIZE + 1;
	vd->uio_info.mem[id].internal_addr =
		(void __iomem *)ioremap_nocache(start, size);
	vd->uio_info.mem[id].addr = start;
	vd->uio_info.mem[id].size = size;
	vd->uio_info.mem[id].memtype = UIO_MEM_PHYS;
	printk(KERN_INFO "  o Mapping buffer #2 at %ld MB Size %ld MB.\n",
			start >> 20, vd->uio_info.mem[id].size >> 20);
#else /* CONFIG_VMETA_NEW */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (res == NULL) {
		printk(KERN_ERR "dove_vmeta_probe: "
				"No VMETA memory supplied.\n");
		goto uio_register_fail;
	}

	id = VMETA_DMA_BUFFER_MAP;
#ifndef CONFIG_MEM_FOR_MULTIPROCESS
	size = CONFIG_UIO_DOVE_VMETA_MEM_SIZE << 20;
#else
	size = (CONFIG_UIO_DOVE_VMETA_MEM_SIZE - 1) << 20;
#endif
	start = res->start;
	vd->uio_info.mem[id].internal_addr =
		(void __iomem *)ioremap_nocache(start, size);
	vd->uio_info.mem[id].addr = start;
	vd->uio_info.mem[id].size = size;
	vd->uio_info.mem[id].memtype = UIO_MEM_PHYS;

	printk(KERN_INFO "  o Mapping buffer at %ld MB Size %ld MB.\n",
			start >> 20, vd->uio_info.mem[id].size >> 20);
#ifdef CONFIG_MEM_FOR_MULTIPROCESS
	start += (( CONFIG_UIO_DOVE_VMETA_MEM_SIZE - 1) << 20);
	size = 1 << 20;
        vd->uio_info.mem[VMETA_VDEC_MEM].internal_addr = 
			(void __iomem *)ioremap_nocache(start, size);
        vd->uio_info.mem[VMETA_VDEC_MEM].addr = start;
        vd->uio_info.mem[VMETA_VDEC_MEM].memtype = UIO_MEM_PHYS;
        vd->uio_info.mem[VMETA_VDEC_MEM].size = size;

#endif

#endif /* CONFIG_VMETA_NEW */

	platform_set_drvdata(pdev, vd);

	vd->uio_info.name = "dove_vmeta_uio";
	vd->uio_info.version = "0.9.0";
	vd->uio_info.irq = platform_get_irq(pdev, 0);
	vd->uio_info.handler = vmeta_irqhandler;
	vd->uio_info.ioctl = vmeta_ioctl;
	vd->uio_info.mmap = vmeta_mmap;
	vd->uio_info.open = vmeta_open;
	vd->uio_info.release = vmeta_release;

	// init video buffer queue for vmeta xv interface
	xvd = kzalloc(sizeof(struct vmeta_xv_data), GFP_KERNEL);
	if (xvd == NULL) {
		printk(KERN_ERR "vdec_prvdec_probe: "
				"Failed to allocate memory.\n");
		ret = -ENOMEM;
		goto uio_register_fail;
	}
	mutex_init(&xvd->lock);
	INIT_LIST_HEAD(&xvd->iqueue.list);
	INIT_LIST_HEAD(&xvd->oqueue.list);
	vd->uio_info.priv = (void*)xvd;

	if (uio_register_device(&pdev->dev, &vd->uio_info)) {
		ret = -ENODEV;
		goto uio_register_fail;
	}

	// get vmeta clk control interface
	vd->vmeta_clk = clk_get(NULL, "vmeta");

	// disable interrupt at initial time
	disable_irq(vd->uio_info.irq);

	// power off the vmeta as default
	clk_enable(vd->vmeta_clk);
	clk_disable(vd->vmeta_clk);

	// register a pm notifier
	register_pm_notifier(&vmeta_pm_notifier);
	mutex_init(&vmeta_pm_suspend_check);
	if(!mutex_is_locked(&vmeta_pm_suspend_check))
		mutex_lock(&vmeta_pm_suspend_check);	// lock as default

	printk(KERN_INFO "VMETA UIO driver registered successfully.\n");
	return 0;

uio_register_fail:
#ifndef CONFIG_VMETA_NEW
	iounmap(vd->uio_info.mem[VMETA_DMA_BUFFER_MAP_1].internal_addr);
	iounmap(vd->uio_info.mem[VMETA_DMA_BUFFER_MAP_2].internal_addr);
#else /* CONFIG_VMETA_NEW */
	iounmap(vd->uio_info.mem[VMETA_DMA_BUFFER_MAP].internal_addr);
#endif /* CONFIG_VMETA_NEW */
	iounmap(vd->uio_info.mem[VMETA_CONTROL_REGISTER_MAP].internal_addr);
	kfree(vd);

	printk(KERN_INFO "Failed to register VMETA uio driver.\n");
	return ret;
}


static int dove_vmeta_remove(struct platform_device *pdev)
{
	struct vmeta_uio_data *vd = platform_get_drvdata(pdev);

	uio_unregister_device(&vd->uio_info);
#ifndef CONFIG_VMETA_NEW
	iounmap(vd->uio_info.mem[VMETA_DMA_BUFFER_MAP_1].internal_addr);
	iounmap(vd->uio_info.mem[VMETA_DMA_BUFFER_MAP_2].internal_addr);
#else /* CONFIG_VMETA_NEW */
	iounmap(vd->uio_info.mem[VMETA_DMA_BUFFER_MAP].internal_addr);
#endif /* CONFIG_VMETA_NEW */
	iounmap(vd->uio_info.mem[VMETA_CONTROL_REGISTER_MAP].internal_addr);
	memset(vd->uio_info.mem, 0, sizeof(vd->uio_info.mem));

	kfree(vd->uio_info.priv);
	kfree(vd);
	vmeta_uio_priv_data = NULL;

	// unregister pm notifier
	unregister_pm_notifier(&vmeta_pm_notifier);
	if(mutex_is_locked(&vmeta_pm_suspend_check))
		mutex_unlock(&vmeta_pm_suspend_check);

	return 0;
} 

static void dove_vmeta_shutdown(struct platform_device *pdev)
{
	return;
}

#ifdef CONFIG_PM
static int dove_vmeta_suspend(struct platform_device *dev, pm_message_t state)
{
	return 0;
}

static int dove_vmeta_resume(struct platform_device *dev)
{
	return 0;
}
#endif

static struct platform_driver vmeta_driver = {
	.probe		= dove_vmeta_probe,
	.remove		= dove_vmeta_remove,
	.shutdown	= dove_vmeta_shutdown,
#ifdef CONFIG_PM
	.suspend	= dove_vmeta_suspend,
	.resume		= dove_vmeta_resume,
#endif
	.driver = {
		.name	= "dove_vmeta_uio",
		.owner	= THIS_MODULE,
	},
};

static int __init dove_vmeta_init(void)
{
	return platform_driver_register(&vmeta_driver);
}

static void __exit dove_vmeta_exit(void)
{
	platform_driver_unregister(&vmeta_driver);
}

module_init(dove_vmeta_init);
module_exit(dove_vmeta_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Dove vMeta UIO driver");

