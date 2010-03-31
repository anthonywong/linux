/****************************************************************************
*  
*    Copyright (C) 2002 - 2008 by Vivante Corp.
*  
*    This program is free software; you can redistribute it and/or modify
*    it under the terms of the GNU General Public Lisence as published by
*    the Free Software Foundation; either version 2 of the license, or
*    (at your option) any later version.
*  
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*    GNU General Public Lisence for more details.
*  
*    You should have received a copy of the GNU General Public License
*    along with this program; if not write to the Free Software
*    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*  
*****************************************************************************/


#ifdef ENABLE_GPU_CLOCK_BY_DRIVER
#undef ENABLE_GPU_CLOCK_BY_DRIVER
#endif

#if defined(CONFIG_DOVE_GPU)
#define ENABLE_GPU_CLOCK_BY_DRIVER	0
#else
#define ENABLE_GPU_CLOCK_BY_DRIVER	1
#endif 

/* You can comment below line to use legacy driver model */
#define USE_PLATFORM_DRIVER 1


#include <linux/device.h>

#include "PreComp.h"
#include "aqHalDriver.h"


#ifdef USE_PLATFORM_DRIVER
#include <linux/platform_device.h>
#endif

MODULE_DESCRIPTION("Vivante Graphics Driver");
MODULE_LICENSE("GPL");

struct class *gpuClass;

static gcoGALDEVICE galDevice;

static int major = 199;
module_param(major, int, 0644);

int irqLine = 0;
module_param(irqLine, int, 0644);

long registerMemBase = 0x80000000;
module_param(registerMemBase, long, 0644);

ulong registerMemSize = 256 << 10;
module_param(registerMemSize, ulong, 0644);

long contiguousSize = 4 << 20;
module_param(contiguousSize, long, 0644);

ulong contiguousBase = 0;
module_param(contiguousBase, ulong, 0644);

long bankSize = 32 << 20;
module_param(bankSize, long, 0644);

int fastClear = -1;
module_param(fastClear, int, 0644);

ulong baseAddress = 0;
module_param(baseAddress, ulong, 0644);

ulong gpu_frequency = 312;
module_param(gpu_frequency, ulong, 0644);

static int drv_open(struct inode *inode, struct file *filp);
static int drv_release(struct inode *inode, struct file *filp);
static int drv_ioctl(struct inode *inode, struct file *filp, 
                     unsigned int ioctlCode, unsigned long arg);
static int drv_mmap(struct file * filp, struct vm_area_struct * vma);

struct file_operations driver_fops =
{
    .open   	= drv_open,
    .release	= drv_release,
    .ioctl  	= drv_ioctl,
    .mmap   	= drv_mmap,
};

int drv_open(struct inode *inode, struct file* filp)
{
    gcsHAL_PRIVATE_DATA_PTR	private;

    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_DRIVER,
    	    	  "Entering drv_open\n");

    private = kmalloc(sizeof(gcsHAL_PRIVATE_DATA), GFP_KERNEL);

    if (private == gcvNULL)
    {
    	return -ENOTTY;
    }
    
    private->device				= galDevice;
    private->mappedMemory		= gcvNULL;
	private->contiguousLogical	= gcvNULL;

    if (!galDevice->contiguousMapped)
    {
    	gcmVERIFY_OK(gcoOS_MapMemory(galDevice->os,
									galDevice->contiguousPhysical,
									galDevice->contiguousSize,
									&private->contiguousLogical));
    }
    
    filp->private_data = private;

    return 0;
}

extern void
OnProcessExit(
	IN gcoOS Os,
	IN gcoKERNEL Kernel
	);

int drv_release(struct inode* inode, struct file* filp)
{
    gcsHAL_PRIVATE_DATA_PTR	private;
    gcoGALDEVICE			device;

    gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_DRIVER,
    	    	  "Entering drv_close\n");

    private = filp->private_data;
    gcmASSERT(private != gcvNULL);

    device = private->device;

	OnProcessExit(galDevice->os, galDevice->kernel);
     
    gcmVERIFY_OK(gcoOS_CleanProcessSignal(device->os,
    	    	    	    	    	  (gctHANDLE) current->tgid));
	
    if (!device->contiguousMapped)
    {
		if (private->contiguousLogical != gcvNULL)
		{
			gcmVERIFY_OK(gcoOS_UnmapMemory(galDevice->os,
											galDevice->contiguousPhysical,
											galDevice->contiguousSize,
											private->contiguousLogical));
		}
    }

    kfree(private);
    filp->private_data = NULL;

    return 0;
}

int drv_ioctl(struct inode *inode,
    	      struct file *filp, 
    	      unsigned int ioctlCode,
	      unsigned long arg)
{
    gcsHAL_INTERFACE interface;
    gctUINT32 copyLen;
    DRIVER_ARGS drvArgs;
    gcoGALDEVICE device;
    gceSTATUS status;
    gcsHAL_PRIVATE_DATA_PTR private;
    
    private = filp->private_data;

    if (private == gcvNULL)
    {
    	gcmTRACE_ZONE(gcvLEVEL_ERROR, gcvZONE_DRIVER,
    	    	      "[galcore] drv_ioctl: private_data is NULL\n");

    	return -ENOTTY;
    }
    
    device = private->device;
    
    if (device == gcvNULL)
    {
    	gcmTRACE_ZONE(gcvLEVEL_ERROR, gcvZONE_DRIVER,
    	    	      "[galcore] drv_ioctl: device is NULL\n");

    	return -ENOTTY;
    }
	
    if (ioctlCode != IOCTL_GCHAL_INTERFACE
		&& ioctlCode != IOCTL_GCHAL_KERNEL_INTERFACE)
    {
        /* Unknown command. Fail the I/O. */
        return -ENOTTY;
    }

    /* Get the drvArgs to begin with. */
    copyLen = copy_from_user(&drvArgs,
    	    	    	     (void *) arg,
			     sizeof(DRIVER_ARGS));
			     
    if (copyLen != 0)
    {
    	/* The input buffer is not big enough. So fail the I/O. */
        return -ENOTTY;
    }

    /* Now bring in the AQHAL_INTERFACE structure. */
    if ((drvArgs.InputBufferSize  != sizeof(gcsHAL_INTERFACE))
    ||  (drvArgs.OutputBufferSize != sizeof(gcsHAL_INTERFACE))
    ) 
    {
        printk("\n [galcore] data structure size in kernel and user do not match !\n");
    	return -ENOTTY;
    }

    copyLen = copy_from_user(&interface,
    	    	    	     drvArgs.InputBuffer,
			     sizeof(gcsHAL_INTERFACE));
    
    if (copyLen != 0)
    {
        /* The input buffer is not big enough. So fail the I/O. */
        return -ENOTTY;
    }

    status = gcoKERNEL_Dispatch(device->kernel,
		(ioctlCode == IOCTL_GCHAL_INTERFACE) , &interface);
    
    if (gcmIS_ERROR(status))
    {
    	gcmTRACE_ZONE(gcvLEVEL_WARNING, gcvZONE_DRIVER,
	    	      "[galcore] gcoKERNEL_Dispatch returned %d.\n",
		      status);
    }

    else if (gcmIS_ERROR(interface.status))
    {
    	gcmTRACE_ZONE(gcvLEVEL_WARNING, gcvZONE_DRIVER,
	    	      "[galcore] IOCTL %d returned %d.\n",
		      interface.command,
		      interface.status);
    }
    
    /* See if this was a LOCK_VIDEO_MEMORY command. */
    else if (interface.command == gcvHAL_LOCK_VIDEO_MEMORY)
    {
    	/* Special case for mapped memory. */
    	if (private->mappedMemory != gcvNULL
		&& interface.u.LockVideoMemory.node->VidMem.memory->object.type
			== gcvOBJ_VIDMEM)
	{
	    /* Compute offset into mapped memory. */
	    gctUINT32 offset = (gctUINT8 *) interface.u.LockVideoMemory.memory
	    	    	     - (gctUINT8 *) device->contiguousBase;
			  
    	    /* Compute offset into user-mapped region. */
    	    interface.u.LockVideoMemory.memory =
	    	(gctUINT8 *)  private->mappedMemory + offset;
	}
    }

    /* Copy data back to the user. */
    copyLen = copy_to_user(drvArgs.OutputBuffer,
    	    	    	   &interface,
			   sizeof(gcsHAL_INTERFACE));

    if (copyLen != 0)
    {
    	/* The output buffer is not big enough. So fail the I/O. */
        return -ENOTTY;
    }

    return 0;
}

static int drv_mmap(struct file * filp, struct vm_area_struct * vma)
{
    gcsHAL_PRIVATE_DATA_PTR private = filp->private_data;
    gcoGALDEVICE device;
    int ret;
    unsigned long size = vma->vm_end - vma->vm_start;
    
    if (private == gcvNULL)
    {
    	return -ENOTTY;
    }
    
    device = private->device;
    
    if (device == gcvNULL)
    {
        return -ENOTTY;
    }
    
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    vma->vm_flags    |= VM_IO | VM_DONTCOPY | VM_DONTEXPAND;
    vma->vm_pgoff     = 0;

    if (device->contiguousMapped)
    {
    	ret = io_remap_pfn_range(vma,
	    	    	    	 vma->vm_start,
    	    	    	    	 (gctUINT32) device->contiguousPhysical >> PAGE_SHIFT,
				 size,
				 vma->vm_page_prot);
						   
    	private->mappedMemory = (ret == 0) ? (gctPOINTER) vma->vm_start : gcvNULL;
						   
    	return ret;
    }
    else
    {
    	return -ENOTTY;
    }
}


#ifndef USE_PLATFORM_DRIVER
static int __init drv_init(void)
#else
static int drv_init(void)
#endif
{
    int ret;
    gcoGALDEVICE device;
    
#if ENABLE_GPU_CLOCK_BY_DRIVER && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)    
    struct clk * clk = NULL;    
#endif

    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_DRIVER,
    	    	  "Entering drv_init\n");

#if ENABLE_GPU_CLOCK_BY_DRIVER && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
    clk = clk_get(NULL, "GCCLK");
    if (IS_ERR(clk)) 
    {
        int retval = PTR_ERR(clk);
        printk("clk get error: %d\n", retval);
        return retval;
    }    	    	  
	printk("\n[galcore] clk input = %dM Hz; running on %dM Hz\n",(int)gpu_frequency, (int)gpu_frequency/2);
    if (clk_set_rate(clk, gpu_frequency*1000*1000)) 
    { 
       	gcmTRACE_ZONE(gcvLEVEL_ERROR, gcvZONE_DRIVER,
    	    	      "[galcore] Can't set core clock to %d.",(int)gpu_frequency); 
        return -EAGAIN;
    }
    clk_enable(clk);
#endif

    /* Create the GAL device. */
    gcmVERIFY_OK(gcoGALDEVICE_Construct(irqLine,
    	    	    	    	    	registerMemBase,
					registerMemSize,
					contiguousBase,
					contiguousSize,
					bankSize,
					fastClear,
					baseAddress,
					&device));
    printk("\n[galcore] chipModel=0x%x,chipRevision=0x%x,chipFeatures=0x%x,chipMinorFeatures=0x%x\n",
        device->kernel->hardware->chipModel, device->kernel->hardware->chipRevision,
        device->kernel->hardware->chipFeatures, device->kernel->hardware->chipMinorFeatures);
    
    /* Start the GAL device. */
    if (gcmIS_ERROR(gcoGALDEVICE_Start(device)))
    {
    	gcmTRACE_ZONE(gcvLEVEL_ERROR, gcvZONE_DRIVER,
    	    	      "[galcore] Can't start the gal device.\n");

    	/* Roll back. */
    	gcoGALDEVICE_Stop(device);
    	gcoGALDEVICE_Destroy(device);

    	return -1;
    }
	
    /* Register the character device. */
    ret = register_chrdev(major, DRV_NAME, &driver_fops);
    if (ret < 0)
    {
    	gcmTRACE_ZONE(gcvLEVEL_ERROR, gcvZONE_DRIVER,
    	    	      "[galcore] Could not allocate major number for mmap.\n");

    	/* Roll back. */
    	gcoGALDEVICE_Stop(device);
    	gcoGALDEVICE_Destroy(device);

    	return -1;
    }
    else
    {
    	if (major == 0)
    	{
    	    major = ret;
    	}
    }

    galDevice = device;

	gpuClass = class_create(THIS_MODULE, "v_graphics_class");
	if (IS_ERR(gpuClass)) {
    	gcmTRACE_ZONE(gcvLEVEL_ERROR, gcvZONE_DRIVER,
					  "Failed to create the class.\n");
		return -1;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
	device_create(gpuClass, NULL, MKDEV(major, 0), NULL, "galcore");
#else
	device_create(gpuClass, NULL, MKDEV(major, 0), "galcore");
#endif
	
    gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_DRIVER,
    	    	  "[galcore] irqLine->%ld, contiguousSize->%lu, memBase->0x%lX\n",
		  irqLine,
		  contiguousSize,
		  registerMemBase);
	
    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_DRIVER,
    	    	  "[galcore] driver registered successfully.\n");

    return 0;
}

#ifndef USE_PLATFORM_DRIVER
static void __exit drv_exit(void)
#else
static void drv_exit(void)
#endif
{
#if ENABLE_GPU_CLOCK_BY_DRIVER && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
    struct clk * clk = NULL;   
#endif
    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_DRIVER,
    	    	  "[galcore] Entering drv_exit\n");

	device_destroy(gpuClass, MKDEV(major, 0));
	class_destroy(gpuClass);

    unregister_chrdev(major, DRV_NAME);

    gcoGALDEVICE_Stop(galDevice);
    gcoGALDEVICE_Destroy(galDevice);

#if ENABLE_GPU_CLOCK_BY_DRIVER && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
    clk = clk_get(NULL, "GCCLK");
    clk_disable(clk);
#endif
    
}

#ifndef USE_PLATFORM_DRIVER
module_init(drv_init);
module_exit(drv_exit);
#else

#define DEVICE_NAME "galcore"

static int __devinit gpu_probe(struct platform_device *pdev)
{
	int ret = -ENODEV;
	struct resource *res;
	res = platform_get_resource_byname(pdev, IORESOURCE_IRQ,"gpu_irq");
	if (!res) {
		printk(KERN_ERR "%s: No irq line supplied.\n",__FUNCTION__);
		goto gpu_probe_fail;
	}
	irqLine = res->start;
	
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM,"gpu_base");
	if (!res) {
		printk(KERN_ERR "%s: No register base supplied.\n",__FUNCTION__);
		goto gpu_probe_fail;
	}
	registerMemBase = res->start;
	registerMemSize = res->end - res->start;
	
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM,"gpu_mem");
	if (!res) {
		printk(KERN_ERR "%s: No memory base supplied.\n",__FUNCTION__);
		goto gpu_probe_fail;
	}
	contiguousBase  = res->start;
	contiguousSize  = res->end-res->start;
	
	ret = drv_init();
	if (!ret)
	{
		platform_set_drvdata(pdev, galDevice);
	}

	return ret;

gpu_probe_fail:	
	printk(KERN_INFO "Failed to register gpu driver.\n");
	return ret;
}

static int __devinit gpu_remove(struct platform_device *pdev)
{
	drv_exit();

	return 0;
}

static int __devinit gpu_suspend(struct platform_device *dev, pm_message_t state)
{
	gceSTATUS status;
	gcoGALDEVICE device;
	struct clk * clk = NULL;

	device = platform_get_drvdata(dev);

#if 1
	status = gcoHARDWARE_NotifyPower(device->kernel->hardware, gcvFALSE);
	if (gcmIS_ERROR(status))
	{
		return -1;
	}

	disable_irq(galDevice->irqLine);

	clk = clk_get(NULL, "GCCLK");
	clk_disable(clk);
#else
	status = gcoHARDWARE_SetPowerManagementState(device->kernel->hardware, gcvPOWER_OFF);

	if (gcmIS_ERROR(status))
	{
		return -1;
	}
#endif

	return 0;
}

static int __devinit gpu_resume(struct platform_device *dev)
{
	gceSTATUS status;
	gcoGALDEVICE device;
	struct clk * clk = NULL;

	device = platform_get_drvdata(dev);

#if 1
	clk = clk_get(NULL, "GCCLK");
	if (IS_ERR(clk))
	{
		int retval = PTR_ERR(clk);
		printk("clk get error\n");
		return retval;
	}

	if (clk_set_rate(clk, gpu_frequency*1000*1000))
	{
		gcmTRACE_ZONE(gcvLEVEL_ERROR, gcvZONE_DRIVER, 
                  "[galcore] Can't set core clock.");
		return -EAGAIN;
	}

	clk_enable(clk);
	enable_irq(device->irqLine);

	status = gcoHARDWARE_NotifyPower(device->kernel->hardware, gcvTRUE);
	if (gcmIS_ERROR(status))
	{
		return -1;
	}
#else
	status = gcoHARDWARE_SetPowerManagementState(device->kernel->hardware, gcvPOWER_ON);

	if (gcmIS_ERROR(status))
	{
		return -1;
	}
#endif

	return 0;
}

static struct platform_driver gpu_driver = {
	.probe		= gpu_probe,
	.remove		= gpu_remove,

	.suspend	= gpu_suspend,
	.resume		= gpu_resume,

	.driver		= {
		.name	= DEVICE_NAME,
	}
};

#ifndef CONFIG_DOVE_GPU

static struct resource gpu_resources[] = {
    {    
        .name   = "gpu_irq",
        .flags  = IORESOURCE_IRQ,
    },   
    {    
        .name   = "gpu_base",
        .flags  = IORESOURCE_MEM,
    },   
    {    
        .name   = "gpu_mem",
        .flags  = IORESOURCE_MEM,
    },   
};

static struct platform_device * gpu_device;

#endif 

static int __init gpu_init(void)
{
	int ret = 0;

#ifndef CONFIG_DOVE_GPU
	gpu_resources[0].start = gpu_resources[0].end = irqLine;

	gpu_resources[1].start = registerMemBase;
	gpu_resources[1].end   = registerMemBase + registerMemSize;

	gpu_resources[2].start = contiguousBase;
	gpu_resources[2].end   = contiguousBase + contiguousSize;

	/* Allocate device */
	gpu_device = platform_device_alloc(DEVICE_NAME, -1);
	if (!gpu_device)
	{
		printk(KERN_ERR "galcore: platform_device_alloc failed.\n");
		ret = -ENOMEM;
		goto out;
	}

	/* Insert resource */
	ret = platform_device_add_resources(gpu_device, gpu_resources, 3);
	if (ret)
	{
		printk(KERN_ERR "galcore: platform_device_add_resources failed.\n");
		goto put_dev;
	}

	/* Add device */
	ret = platform_device_add(gpu_device);
	if (ret)
	{
		printk(KERN_ERR "galcore: platform_device_add failed.\n");
		goto del_dev;
	}
#endif

	ret = platform_driver_register(&gpu_driver);
	if (!ret)
	{
		goto out;
	}

#ifndef CONFIG_DOVE_GPU
del_dev:
	platform_device_del(gpu_device);
put_dev:
	platform_device_put(gpu_device);
#endif

out:
	return ret;

}

static void __exit gpu_exit(void)
{
	platform_driver_unregister(&gpu_driver);
#ifndef CONFIG_DOVE_GPU
	platform_device_unregister(gpu_device);
#endif
}

module_init(gpu_init);
module_exit(gpu_exit);

#endif


