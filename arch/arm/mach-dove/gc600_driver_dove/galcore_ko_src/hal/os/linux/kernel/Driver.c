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
#include "Context.h"

#if USE_PLATFORM_DRIVER
#include <linux/platform_device.h>
#endif

MODULE_DESCRIPTION("Vivante Graphics Driver");
MODULE_LICENSE("GPL");

struct class *gpuClass;

static gcoGALDEVICE galDevice;

static int major = 199;
module_param(major, int, 0644);

int irqLine = 8;
module_param(irqLine, int, 0644);

long registerMemBase = 0xc0400000;
module_param(registerMemBase, long, 0644);

ulong registerMemSize = 256 << 10;
module_param(registerMemSize, ulong, 0644);

long contiguousSize = 32 << 20;
module_param(contiguousSize, long, 0644);

ulong contiguousBase = 0;
module_param(contiguousBase, ulong, 0644);

long bankSize = 32 << 20;
module_param(bankSize, long, 0644);

int fastClear = -1;
module_param(fastClear, int, 0644);

int compression = -1;
module_param(compression, int, 0644);

int signal = 48;
module_param(signal, int, 0644);

ulong baseAddress = 0;
module_param(baseAddress, ulong, 0644);

int showArgs = 0;
module_param(showArgs, int, 0644);
ulong gpu_frequency = 312;
module_param(gpu_frequency, ulong, 0644);


#ifdef CONFIG_PXA_DVFM
#include <mach/dvfm.h>
#include <mach/pxa3xx_dvfm.h>
#include <linux/delay.h>

#define MRVL_CONFIG_PROC

static int galcore_dvfm_notifier(struct notifier_block *nb,
				unsigned long val, void *data);

static struct notifier_block galcore_notifier_block = {
	.notifier_call = galcore_dvfm_notifier,
};
#endif

#ifdef MRVL_CONFIG_PROC
#include <linux/proc_fs.h>
#define GC_PROC_FILE    "driver/gc"
static struct proc_dir_entry * gc_proc_file;

/* cat /proc/driver/gc will print gc related msg */
static ssize_t gc_proc_read(struct file *file,
    char __user *buffer, size_t count, loff_t *offset)
{
	ssize_t len = 0;
	char buf[1000];
    gctUINT32 idle;
    gctUINT32 clockControl;
    
    gcmVERIFY_OK(gcoHARDWARE_GetIdle(galDevice->kernel->hardware, gcvFALSE, &idle));
	len += sprintf(buf+len, "idle register: 0x%02x\n", idle);
    
    gcoOS_ReadRegister(galDevice->os, 0x00000, &clockControl);
    len += sprintf(buf+len, "clockControl register: 0x%02x\n", clockControl);

	return simple_read_from_buffer(buffer, count, offset, buf, len);

    return 0;
}

/* echo xx > /proc/driver/gc set ... */
static ssize_t gc_proc_write(struct file *file,
		const char *buff, size_t len, loff_t *off)
{
    char messages[256];

	if(len > 256)
		len = 256;

	if(copy_from_user(messages, buff, len))
		return -EFAULT;

    printk("\n");
    if(strncmp(messages, "d2debug", 7) == 0)
    {
        galDevice->needD2DebugInfo = galDevice->needD2DebugInfo ? gcvFALSE : gcvTRUE;
    }
    else if(strncmp(messages, "16", 2) == 0)
    {
		printk("frequency change to 1/16\n");
        /* frequency change to 1/16 */
        gcmVERIFY_OK(gcoOS_WriteRegister(galDevice->os,0x00000,0x210));
        /* Loading the frequency scaler. */
    	gcmVERIFY_OK(gcoOS_WriteRegister(galDevice->os,0x00000,0x010));
        
    }
    else if(strncmp(messages, "32", 2) == 0)
    {
		printk("frequency change to 1/32\n");
        /* frequency change to 1/32*/
        gcmVERIFY_OK(gcoOS_WriteRegister(galDevice->os,0x00000,0x208));
        /* Loading the frequency scaler. */
    	gcmVERIFY_OK(gcoOS_WriteRegister(galDevice->os,0x00000,0x008));
        
    }
	else if(strncmp(messages, "64", 2) == 0)
    {
		printk("frequency change to 1/64\n");
        /* frequency change to 1/64 */
        gcmVERIFY_OK(gcoOS_WriteRegister(galDevice->os,0x00000,0x204));
        /* Loading the frequency scaler. */
    	gcmVERIFY_OK(gcoOS_WriteRegister(galDevice->os,0x00000,0x004));
        
    }
    else if('1' == messages[0])
    {
        printk("frequency change to full speed\n");
        /* frequency change to full speed */
        gcmVERIFY_OK(gcoOS_WriteRegister(galDevice->os,0x00000,0x300));
        /* Loading the frequency scaler. */
    	gcmVERIFY_OK(gcoOS_WriteRegister(galDevice->os,0x00000,0x100));
        
    }
    else if('2' == messages[0])
    {
        printk("frequency change to 1/2\n");
        /* frequency change to 1/2 */
        gcmVERIFY_OK(gcoOS_WriteRegister(galDevice->os,0x00000,0x280));
        /* Loading the frequency scaler. */
    	gcmVERIFY_OK(gcoOS_WriteRegister(galDevice->os,0x00000,0x080));
        
    }
    else if('4' == messages[0])
    {
        printk("frequency change to 1/4\n");
        /* frequency change to 1/4 */
        gcmVERIFY_OK(gcoOS_WriteRegister(galDevice->os,0x00000,0x240));
        /* Loading the frequency scaler. */
    	gcmVERIFY_OK(gcoOS_WriteRegister(galDevice->os,0x00000,0x040));
        
    }
    else if('8' == messages[0])
    {
        printk("frequency change to 1/8\n");
        /* frequency change to 1/8 */
        gcmVERIFY_OK(gcoOS_WriteRegister(galDevice->os,0x00000,0x220));
        /* Loading the frequency scaler. */
    	gcmVERIFY_OK(gcoOS_WriteRegister(galDevice->os,0x00000,0x020));
        
    }
    else
    {
        printk("unknown echo\n");
    }

    return len;
}

static struct file_operations gc_proc_ops = {
	.read = gc_proc_read,
	.write = gc_proc_write,
};

static void create_gc_proc_file(void)
{
	gc_proc_file = create_proc_entry(GC_PROC_FILE, 0644, NULL);
	if (gc_proc_file) {
		gc_proc_file->owner = THIS_MODULE;
		gc_proc_file->proc_fops = &gc_proc_ops;
	} else
		printk("[galcore] proc file create failed!\n");
}

static void remove_gc_proc_file(void)
{
	remove_proc_entry(GC_PROC_FILE, NULL);
}

#endif

static int drv_open(struct inode *inode, struct file *filp);
static int drv_release(struct inode *inode, struct file *filp);
static int drv_ioctl(struct inode *inode, struct file *filp,
                     unsigned int ioctlCode, unsigned long arg);
static int drv_mmap(struct file * filp, struct vm_area_struct * vma);

struct file_operations driver_fops =
{
    .open   	= drv_open,
    .release	= drv_release,
    .ioctl  		= drv_ioctl,
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
#ifdef ANDROID
	private->memoryRecordList.prev = &private->memoryRecordList;
	private->memoryRecordList.next = &private->memoryRecordList;
#endif

	/* A process gets attached. */
	gcmVERIFY_OK(
		gcoKERNEL_AttachProcess(galDevice->kernel, gcvTRUE));

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

#ifdef ANDROID
	gcmVERIFY_OK(gcoCOMMAND_Stall(device->kernel->command));

	FreeAllMemoryRecord(galDevice->os, &private->memoryRecordList);

	gcmVERIFY_OK(gcoCOMMAND_Stall(device->kernel->command));
#else
	OnProcessExit(galDevice->os, galDevice->kernel);
#endif

    if (private->contiguousLogical != gcvNULL)
    {
			gcmVERIFY_OK(gcoOS_UnmapMemory(galDevice->os,
											galDevice->contiguousPhysical,
											galDevice->contiguousSize,
											private->contiguousLogical));
     }


	/* A process gets detached. */
	gcmVERIFY_OK(
		gcoKERNEL_AttachProcess(galDevice->kernel, gcvFALSE));

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

#ifdef ANDROID
#if USE_EVENT_QUEUE
	if (interface.command == gcvHAL_EVENT_COMMIT)
	{
		MEMORY_RECORD_PTR mr;
		gcsQUEUE_PTR queue = interface.u.Event.queue;

		while (queue != gcvNULL)
		{
			gcsQUEUE_PTR record, next;

			/* Map record into kernel memory. */
			gcmERR_BREAK(gcoOS_MapUserPointer(device->os,
											  queue,
											  gcmSIZEOF(gcsQUEUE),
											  (gctPOINTER *) &record));

			switch (record->interface.command)
			{
			case gcvHAL_FREE_VIDEO_MEMORY:
				mr = FindMemoryRecord(device->os,
									&private->memoryRecordList,
									record->interface.u.FreeVideoMemory.node);

				if (mr != gcvNULL)
				{
					DestoryMemoryRecord(device->os, mr);
				}
				else
				{
					printk("*ERROR* Invalid video memory (%p) for free\n",
						record->interface.u.FreeVideoMemory.node);
				}
                break;

			default:
				break;
			}

			/* Next record in the queue. */
			next = record->next;

			/* Unmap record from kernel memory. */
			gcmERR_BREAK(gcoOS_UnmapUserPointer(device->os,
												queue,
												gcmSIZEOF(gcsQUEUE),
												(gctPOINTER *) record));
			queue = next;
		}
	}
#endif
#endif

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
#ifdef ANDROID
	else if (interface.command == gcvHAL_ALLOCATE_VIDEO_MEMORY)
	{
		CreateMemoryRecord(device->os,
							&private->memoryRecordList,
							interface.u.AllocateVideoMemory.node);
	}
	else if (interface.command == gcvHAL_ALLOCATE_LINEAR_VIDEO_MEMORY)
	{
		CreateMemoryRecord(device->os,
							&private->memoryRecordList,
							interface.u.AllocateLinearVideoMemory.node);
	}
	else if (interface.command == gcvHAL_FREE_VIDEO_MEMORY)
	{
		MEMORY_RECORD_PTR mr;

		mr = FindMemoryRecord(device->os,
							&private->memoryRecordList,
							interface.u.FreeVideoMemory.node);

		if (mr != gcvNULL)
		{
			DestoryMemoryRecord(device->os, mr);
		}
		else
		{
			printk("*ERROR* Invalid video memory for free\n");
		}
	}
#endif

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


#if !USE_PLATFORM_DRIVER
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

	if (showArgs)
	{
		printk("galcore options:\n");
		printk("  irqLine         = %d\n",      irqLine);
		printk("  registerMemBase = 0x%08lX\n", registerMemBase);
		printk("  contiguousSize  = %ld\n",     contiguousSize);
		printk("  contiguousBase  = 0x%08lX\n", contiguousBase);
		printk("  bankSize        = 0x%08lX\n", bankSize);
		printk("  fastClear       = %d\n",      fastClear);
		printk("  compression     = %d\n",      compression);
		printk("  signal          = %d\n",      signal);
		printk("  baseAddress     = 0x%08lX\n", baseAddress);
	}

    /* Create the GAL device. */
    gcmVERIFY_OK(gcoGALDEVICE_Construct(irqLine,
    	    	    	    	    	registerMemBase,
					registerMemSize,
					contiguousBase,
					contiguousSize,
					bankSize,
					fastClear,
					compression,
					baseAddress,
					signal,
					&device));
    printk("\n[galcore] chipModel=0x%x,chipRevision=0x%x,chipFeatures=0x%x,chipMinorFeatures=0x%x\n",
        device->kernel->hardware->chipModel, device->kernel->hardware->chipRevision,
        device->kernel->hardware->chipFeatures, device->kernel->hardware->chipMinorFeatures0);

#ifdef CONFIG_PXA_DVFM
    /* register galcore as a dvfm device*/
    if(dvfm_register("Galcore", &device->dvfm_dev_index))
    {
        printk("\n[galcore] fail to do dvfm_register\n");
    }

    if(dvfm_register_notifier(&galcore_notifier_block,
				DVFM_FREQUENCY_NOTIFIER))
    {
        printk("\n[galcore] fail to do dvfm_register_notifier\n");
    }

    device->dvfm_notifier = &galcore_notifier_block;

    device->needResetAfterD2 = gcvTRUE;
    device->needD2DebugInfo = gcvFALSE;

    gcoOS_SetConstraint(device->os, gcvTRUE, gcvTRUE);
#endif

    device->enableLowPowerMode = gcvTRUE;
    device->enableDVFM = gcvTRUE;

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

    /* device should be idle because it is just initialized */
    gcoOS_NotifyIdle(device->os, gcvTRUE);
    return 0;
}

#if !USE_PLATFORM_DRIVER
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

#ifdef CONFIG_PXA_DVFM
    gcoOS_UnSetConstraint(galDevice->os, gcvTRUE, gcvTRUE);

    if(dvfm_unregister_notifier(&galcore_notifier_block,
				DVFM_FREQUENCY_NOTIFIER))
    {
        printk("\n[galcore] fail to do dvfm_unregister_notifier\n");
    }
		
    if(dvfm_unregister("Galcore", &galDevice->dvfm_dev_index))
    {
        printk("\n[galcore] fail to do dvfm_unregister\n");
    }
#endif

    gcoGALDEVICE_Destroy(galDevice);

#if ENABLE_GPU_CLOCK_BY_DRIVER && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
    clk = clk_get(NULL, "GCCLK");
    clk_disable(clk);
#endif
    
}

#if !USE_PLATFORM_DRIVER
module_init(drv_init);
module_exit(drv_exit);
#else

#ifdef CONFIG_DOVE_GPU
#ifndef ANDROID
#define DEVICE_NAME "galcore"
#else
#define DEVICE_NAME "galcore"
#endif
#else
#define DEVICE_NAME "galcore"
#endif

#ifdef ANDROID
static void gpu_early_suspend(struct early_suspend *h)
{
    printk("[galcore]: %s\n",__func__);
	/*pm_message_t t;
	if (gpu_suspend != NULL)
		gpu_suspend(NULL, t);*/
}
static void gpu_late_resume(struct early_suspend *h)
{
    printk("[galcore]: %s\n",__func__);
	/*if (gpu_resume != NULL)
		gpu_resume(NULL);*/
}
static struct early_suspend gpu_early_suspend_desc = {
	.suspend = gpu_early_suspend,
	.resume = gpu_late_resume,
};
#endif
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

    if (ret < 0)
    {
     	printk(KERN_ERR "%s: gpu_probe failed.\n",__FUNCTION__);
		goto gpu_probe_fail;
    }

    platform_set_drvdata(pdev, galDevice);
    
#ifdef MRVL_CONFIG_PROC
    create_gc_proc_file();
#endif

#ifdef ANDROID
    register_early_suspend(&gpu_early_suspend_desc);
#endif

    return ret;

gpu_probe_fail:
	printk(KERN_INFO "Failed to register gpu driver.\n");
	return ret;
}

static int __devinit gpu_remove(struct platform_device *pdev)
{
	drv_exit();

#ifdef MRVL_CONFIG_PROC
    remove_gc_proc_file();
#endif

#ifdef ANDROID
    unregister_early_suspend(&gpu_early_suspend_desc);
#endif

	return 0;
}

static int __devinit gpu_suspend(struct platform_device *dev, pm_message_t state)
{
	gceSTATUS status;
	gcoGALDEVICE device;
    struct clk * clk = NULL;    

    printk("[galcore]: %s\n",__func__);
    
	device = platform_get_drvdata(dev);

#if 1
    if(device->kernel->hardware->chipPowerState != gcvPOWER_OFF)
    {
        status = gcoHARDWARE_SetPowerManagementState(device->kernel->hardware, gcvPOWER_OFF);
    	if (gcmIS_ERROR(status))
    	{
    		return -1;
    	}
        
        disable_irq(galDevice->irqLine);

        clk = clk_get(NULL, "GCCLK");
        clk_disable(clk);
    }
#ifdef CONFIG_PXA_DVFM
    galDevice->needResetAfterD2 = gcvFALSE;
#endif
#else

	status = gcoHARDWARE_SetPowerManagementState(device->kernel->hardware, gcvPOWER_OFF);

	if (gcmIS_ERROR(status))
	{
		return -1;
	}
#endif

    gcoOS_UnSetConstraint(galDevice->os, gcvTRUE, gcvTRUE);

	return 0;
}

static int __devinit gpu_resume(struct platform_device *dev)
{
	gceSTATUS status;
	gcoGALDEVICE device;
    struct clk * clk = NULL;

    printk("[galcore]: %s\n",__func__);

	device = platform_get_drvdata(dev);

    gcoOS_SetConstraint(galDevice->os, gcvTRUE, gcvTRUE);
	
#if 1
    if(device->kernel->hardware->chipPowerState != gcvPOWER_ON)
    {
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

        status = gcoHARDWARE_SetPowerManagementState(device->kernel->hardware, gcvPOWER_ON);
    	if (gcmIS_ERROR(status))
    	{
    		return -1;
    	}
    }
#ifdef CONFIG_PXA_DVFM
    galDevice->needResetAfterD2 = gcvTRUE;
#endif    
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

#ifdef CONFIG_PXA_DVFM
//#define TRACE printk("%s,%d\n",__func__,__LINE__);
static int galcore_dvfm_notifier(struct notifier_block *nb,
				unsigned long val, void *data)
{
    struct dvfm_freqs *freqs = (struct dvfm_freqs *)data;
	struct op_info *new = NULL;
	struct dvfm_md_opt *md;

    gctUINT32 idle;
    
#if MRVL_LOW_POWER_MODE_DEBUG 
    int strlen = 0;
#endif

	if (freqs)
		new = &freqs->new_info;
	else
		return 0;
	md = (struct dvfm_md_opt *)new->op;

#if MRVL_LOW_POWER_MODE_DEBUG 
/*    strlen = sprintf(galDevice->kernel->kernelMSG + galDevice->kernel->msgLen,"md->power_mode=%d\n",md->power_mode); \
     galDevice->kernel->msgLen += strlen;
*/
#endif

	switch (val) {
	case DVFM_FREQ_PRECHANGE:
		if ((md->power_mode == POWER_MODE_D1) ||
			(md->power_mode == POWER_MODE_D2) ||
			(md->power_mode == POWER_MODE_CG)) {
#if MRVL_LOW_POWER_MODE_DEBUG           			
            {
    			strlen = sprintf(galDevice->kernel->kernelMSG + galDevice->kernel->msgLen,"[galcore] enter D2\n"); \
                galDevice->kernel->msgLen += strlen;

    			gcmVERIFY_OK(gcoHARDWARE_GetIdle(galDevice->kernel->hardware, gcvFALSE, &idle));
                strlen = sprintf(galDevice->kernel->kernelMSG + galDevice->kernel->msgLen,"Idle register = 0x%08X\n",idle); \
                galDevice->kernel->msgLen += strlen;
            }
#endif
            if(galDevice->needResetAfterD2)
            {
                if(galDevice->kernel->hardware->chipPowerState != gcvPOWER_OFF)
                {
                    gceSTATUS  status;
                	gcoCOMMAND command;
                	gctPOINTER buffer;
                	gctSIZE_T  bytes, requested;

                    command = galDevice->kernel->command;
                    
                    // stall
                	{
                		/* Acquire the context switching mutex so nothing else can be
                		** committed. */
                		gcmONERROR(
                			gcoOS_AcquireMutex(galDevice->kernel->hardware->os,
                							   command->mutexContext,
                							   gcvINFINITE));
    
                		/* Get the size of the flush command. */
                		gcmONERROR(
                			gcoHARDWARE_Flush(galDevice->kernel->hardware,
                							  ~0,
                							  gcvNULL,
                							  &requested));
    
                		/* Reserve space in the command queue. */
                		gcmONERROR(
                			gcoCOMMAND_Reserve(command,
                							   requested,
                							   &buffer,
                							   &bytes));
    
                		/* Append a flush. */
                		gcmONERROR(
                			gcoHARDWARE_Flush(galDevice->kernel->hardware,
                							  ~0,
                							  buffer,
                							  &bytes));
    
                		/* Execute the command queue. */
                		gcmONERROR(
                			gcoCOMMAND_Execute(command,
                							   requested));
    
                		/* Wait to finish all commands. */
                		//gcmONERROR(gcoCOMMAND_Stall2(command));
                	}
    
                	// stop
                	{
                		/* Stop the command parser. */
                		gcmONERROR(
                			gcoCOMMAND_Stop(command));
    
                		/* Grab the command queue mutex so nothing can get access to the
                		** command queue. */
                		gcmONERROR(
                			gcoOS_AcquireMutex(galDevice->kernel->hardware->os,
                							   command->mutexQueue,
                							   gcvINFINITE));
                	}
    
            		
                    disable_irq(galDevice->irqLine);
                    clk_disable(clk_get(NULL, "GCCLK"));

                    galDevice->kernel->hardware->chipPowerState = gcvPOWER_OFF;
                }
            }
		}
		break;
	case DVFM_FREQ_POSTCHANGE:
		/* It's existing from D1/D2.
		 * And new op_info won't be changed.
		 */
		if ((md->power_mode == POWER_MODE_D1) ||
			(md->power_mode == POWER_MODE_D2) ||
			(md->power_mode == POWER_MODE_CG)) {
			
            static int count = 0;
            if(galDevice->needResetAfterD2)
            {
                if(galDevice->kernel->hardware->chipPowerState != gcvPOWER_ON)
                {
                    gceSTATUS  status;
                    
                    clk_enable(clk_get(NULL, "AXICLK"));
                    clk_enable(clk_get(NULL, "GCCLK"));
                    enable_irq(galDevice->irqLine);
    
                    // INITIALIZE
                	{
                		/* Initialize hardware. */
                		gcmONERROR(
                			gcoHARDWARE_InitializeHardware(galDevice->kernel->hardware));
    
                		gcmONERROR(
                			gcoHARDWARE_SetFastClear(galDevice->kernel->hardware,
                									 galDevice->kernel->hardware->allowFastClear,
                									 galDevice->kernel->hardware->allowCompression));
    
                		/* Force the command queue to reload the next context. */
                		galDevice->kernel->command->currentContext = 0;
                	}

                	/* Sleep for 1ms, to make sure everything is powered on. */
                	mdelay(1);//gcmVERIFY_OK(gcoOS_Delay(galDevice->os, 1));
    
                	// start
                	{
                		/* Release the command mutex queue. */
                		gcmONERROR(
                			gcoOS_ReleaseMutex(galDevice->kernel->hardware->os,
                							   galDevice->kernel->command->mutexQueue));
    
                		/* Start the command processor. */
                		gcmONERROR(
                			gcoCOMMAND_Start(galDevice->kernel->command));
                	}
    
                	// RELEASE_CONTEXT
                	{
                		/* Release the context switching mutex. */
                		gcmVERIFY_OK(
                			gcoOS_ReleaseMutex(galDevice->kernel->hardware->os,
                							   galDevice->kernel->command->mutexContext));
                	}
    
                    galDevice->kernel->hardware->chipPowerState = gcvPOWER_ON;
                }
                
                /* Read idle register. */
            	gcmVERIFY_OK(gcoHARDWARE_GetIdle(galDevice->kernel->hardware, gcvFALSE, &idle));

                if(galDevice->needD2DebugInfo)
                    printk("%d: reset after power mode %d: 0x%08X\n",count++,md->power_mode,idle);
            }
            else
            {
                if(galDevice->needD2DebugInfo)
                    printk("%d: no reset after power mode %d: 0x%08X\n",count++,md->power_mode,idle);
            }
#if MRVL_LOW_POWER_MODE_DEBUG
            {
                int i = 0;
                
                printk(">>>>>>>>>>>>galDevice->kernel->kernelMSG\n");
                printk("galDevice->kernel->msgLen=%d\n",galDevice->kernel->msgLen);
                
                for(i=0;i<galDevice->kernel->msgLen;i+=1024)
                {
                    galDevice->kernel->kernelMSG[i+1023] = '\0';
            	    printk("%s\n",(char*)galDevice->kernel->kernelMSG + i);
                }
            }
#endif

		}
		break;
	}
	return 0;

OnError:
	printk("ERROR: %s has error \n",__func__);
    
    return 0;
}

#endif

