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




#include "PreComp.h"
#include <linux/pagemap.h>
#include <linux/seq_file.h>
#include <linux/mm.h>
#include <linux/mman.h>

#ifdef FLAREON
static struct dove_gpio_irq_handler gc500_handle;
#endif

/******************************************************************************\
******************************** gcoGALDEVICE Code *******************************
\******************************************************************************/

gceSTATUS
gcoGALDEVICE_AllocateMemory(
	IN gcoGALDEVICE Device,
	IN gctSIZE_T Bytes,
    OUT gctPOINTER *Logical,
    OUT gctPHYS_ADDR *Physical,
    OUT gctUINT32 *PhysAddr
	)
{
	gceSTATUS status;

	gcmVERIFY_ARGUMENT(Device != NULL);
	gcmVERIFY_ARGUMENT(Logical != NULL);
	gcmVERIFY_ARGUMENT(Physical != NULL);
	gcmVERIFY_ARGUMENT(PhysAddr != NULL);

	status = gcoOS_AllocateContiguous(Device->os,
					  gcvFALSE,
					  &Bytes,
					  Physical,
					  Logical);

	if (gcmIS_ERROR(status))
	{
		gcmTRACE_ZONE(gcvLEVEL_ERROR,
				gcvZONE_DRIVER,
    			"[galcore] gcoGALDEVICE_AllocateMemory: error status->0x%x\n",
			   status);

		return status;
	}

	*PhysAddr = ((PLINUX_MDL)*Physical)->dmaHandle - Device->baseAddress;
	gcmTRACE_ZONE(gcvLEVEL_INFO,
				gcvZONE_DRIVER,
    			"[galcore] gcoGALDEVICE_AllocateMemory: phys_addr->0x%x phsical->0x%x Logical->0x%x\n",
               (gctUINT32)*Physical,
			   (gctUINT32)*PhysAddr,
			   (gctUINT32)*Logical);

    /* Success. */
    return gcvSTATUS_OK;
}

gceSTATUS
gcoGALDEVICE_FreeMemory(
	IN gcoGALDEVICE Device,
	IN gctPOINTER Logical,
	IN gctPHYS_ADDR Physical)
{
	gcmVERIFY_ARGUMENT(Device != NULL);

    return gcoOS_FreeContiguous(Device->os,
					Physical,
					Logical,
					((PLINUX_MDL)Physical)->numPages*PAGE_SIZE);
}

irqreturn_t isrRoutine(int irq, void *ctxt)
{
    gcoGALDEVICE device = (gcoGALDEVICE)ctxt;
    int handled = 0;

    /* Call kernel interrupt notification. */
    if (gcoKERNEL_Notify(device->kernel,
						gcvNOTIFY_INTERRUPT,
						gcvTRUE) == gcvSTATUS_OK)
    {
		device->dataReady = gcvTRUE;

		up(&device->sema);

		handled = 1;
    }

    return IRQ_RETVAL(handled);
}

int threadRoutine(void *ctxt)
{
    gcoGALDEVICE device = (gcoGALDEVICE)ctxt;

	gcmTRACE_ZONE(gcvLEVEL_INFO,
				gcvZONE_DRIVER,
				"Starting isr Thread with extension->0x%x\n",
				(gctUINT32)device);

	while (1)
	{

		if(down_interruptible(&device->sema) == 0)
		{
		device->dataReady = gcvFALSE;

		if (device->killThread == gcvTRUE)
		{
			return 0;
		}

        /* Wait for the interrupt. */
		if (kthread_should_stop())
		{
			return 0;
		}

		gcoKERNEL_Notify(device->kernel, gcvNOTIFY_INTERRUPT, gcvFALSE);
    }
    }

    return 0;
}

/*******************************************************************************
**
**	gcoGALDEVICE_Setup_ISR
**
**	Start the ISR routine.
**
**	INPUT:
**
**		gcoGALDEVICE Device
**			Pointer to an gcoGALDEVICE object.
**
**	OUTPUT:
**
**		Nothing.
**
**	RETURNS:
**
**		gcvSTATUS_OK
**			Setup successfully.
**		gcvSTATUS_GENERIC_IO
**			Setup failed.
*/
gceSTATUS
gcoGALDEVICE_Setup_ISR(
	IN gcoGALDEVICE Device
	)
{
	gctINT ret;

	gcmVERIFY_ARGUMENT(Device != NULL);

	if (Device->irqLine == 0)
	{
		return gcvSTATUS_GENERIC_IO;
	}

	/* Hook up the isr based on the irq line. */
#ifdef FLAREON
	gc500_handle.dev_name = "galcore interrupt service";
	gc500_handle.dev_id = Device;
	gc500_handle.handler = isrRoutine;
	gc500_handle.intr_gen = GPIO_INTR_LEVEL_TRIGGER;
	gc500_handle.intr_trig = GPIO_TRIG_HIGH_LEVEL;
	ret = dove_gpio_request (DOVE_GPIO0_7, &gc500_handle);
#else
	ret = request_irq(Device->irqLine,
				isrRoutine,
				IRQF_DISABLED,
				"galcore interrupt service",
				Device);
#endif


	if (ret != 0) {
		gcmTRACE_ZONE(gcvLEVEL_INFO,
				gcvZONE_DRIVER,
            	"[galcore] gcoGALDEVICE_Setup_ISR: "
				"Could not register irq line->%d\n",
				Device->irqLine);

		Device->isrInitialized = gcvFALSE;

		return gcvSTATUS_GENERIC_IO;
	}

	Device->isrInitialized = gcvTRUE;

	gcmTRACE_ZONE(gcvLEVEL_INFO,
				gcvZONE_DRIVER,
            	"[galcore] gcoGALDEVICE_Setup_ISR: "
				"Setup the irq line->%d\n",
				Device->irqLine);

	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoGALDEVICE_Release_ISR
**
**	Release the irq line.
**
**	INPUT:
**
**		gcoGALDEVICE Device
**			Pointer to an gcoGALDEVICE object.
**
**	OUTPUT:
**
**		Nothing.
**
**	RETURNS:
**
**		Nothing.
*/
gceSTATUS
gcoGALDEVICE_Release_ISR(
	IN gcoGALDEVICE Device
	)
{
	gcmVERIFY_ARGUMENT(Device != NULL);

	/* release the irq */
	if (Device->isrInitialized)
	{
#ifdef FLAREON
		dove_gpio_free (DOVE_GPIO0_7, "galcore interrupt service");
#else
		free_irq(Device->irqLine, Device);
#endif
	}

	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoGALDEVICE_Start_Thread
**
**	Start the daemon thread.
**
**	INPUT:
**
**		gcoGALDEVICE Device
**			Pointer to an gcoGALDEVICE object.
**
**	OUTPUT:
**
**		Nothing.
**
**	RETURNS:
**
**		gcvSTATUS_OK
**			Start successfully.
**		gcvSTATUS_GENERIC_IO
**			Start failed.
*/
gceSTATUS
gcoGALDEVICE_Start_Thread(
	IN gcoGALDEVICE Device
	)
{
	gcmVERIFY_ARGUMENT(Device != NULL);

	/* start the kernel thread */
    Device->threadCtxt = kthread_run(threadRoutine,
					Device,
					"galcore daemon thread");

	Device->threadInitialized = gcvTRUE;
	gcmTRACE_ZONE(gcvLEVEL_INFO,
					gcvZONE_DRIVER,
					"[galcore] gcoGALDEVICE_Start_Thread: "
					"Stat the daemon thread.\n");

	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoGALDEVICE_Stop_Thread
**
**	Stop the gal device, including the following actions: stop the daemon
**	thread, release the irq.
**
**	INPUT:
**
**		gcoGALDEVICE Device
**			Pointer to an gcoGALDEVICE object.
**
**	OUTPUT:
**
**		Nothing.
**
**	RETURNS:
**
**		Nothing.
*/
gceSTATUS
gcoGALDEVICE_Stop_Thread(
	gcoGALDEVICE Device
	)
{
	gcmVERIFY_ARGUMENT(Device != NULL);

	/* stop the thread */
	if (Device->threadInitialized)
	{
		Device->killThread = gcvTRUE;
		up(&Device->sema);

		kthread_stop(Device->threadCtxt);
	}

	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoGALDEVICE_Start
**
**	Start the gal device, including the following actions: setup the isr routine
**  and start the daemoni thread.
**
**	INPUT:
**
**		gcoGALDEVICE Device
**			Pointer to an gcoGALDEVICE object.
**
**	OUTPUT:
**
**		Nothing.
**
**	RETURNS:
**
**		gcvSTATUS_OK
**			Start successfully.
*/
gceSTATUS
gcoGALDEVICE_Start(
	IN gcoGALDEVICE Device
	)
{
	/* start the daemon thread */
	gcmVERIFY_OK(gcoGALDEVICE_Start_Thread(Device));

	/* setup the isr routine */
	gcmVERIFY_OK(gcoGALDEVICE_Setup_ISR(Device));

    /*
       gcvPOWER_SUSPEND will stop GC and disable 2D/3D clk.
       Since frequency scaling is enabled, disable 2D/3D clk will gain little
       but lead to potential issue. I prefer not change power state here.
    */
#if 0
	/* Switch to SUSPEND power state. */
	gcmVERIFY_OK(
		gcoHARDWARE_SetPowerManagementState(Device->kernel->hardware,
											gcvPOWER_SUSPEND));
#endif
	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoGALDEVICE_Stop
**
**	Stop the gal device, including the following actions: stop the daemon
**	thread, release the irq.
**
**	INPUT:
**
**		gcoGALDEVICE Device
**			Pointer to an gcoGALDEVICE object.
**
**	OUTPUT:
**
**		Nothing.
**
**	RETURNS:
**
**		Nothing.
*/
gceSTATUS
gcoGALDEVICE_Stop(
    gcoGALDEVICE Device
    )
{
    gcmVERIFY_ARGUMENT(Device != NULL);

    /*
       There is not need to change power state here
    */
#if 0
	/* Switch to ON power state. */
	gcmVERIFY_OK(
		gcoHARDWARE_SetPowerManagementState(Device->kernel->hardware,
											gcvPOWER_ON));
#endif
    if (Device->isrInitialized)
    {
    	gcoGALDEVICE_Release_ISR(Device);
    }

    if (Device->threadInitialized)
    {
    	gcoGALDEVICE_Stop_Thread(Device);
    }

    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoGALDEVICE_Construct
**
**  Constructor.
**
**  INPUT:
**
**  OUTPUT:
**
**  	gcoGALDEVICE * Device
**  	    Pointer to a variable receiving the gcoGALDEVICE object pointer on
**  	    success.
*/
gceSTATUS
gcoGALDEVICE_Construct(
    IN gctINT IrqLine,
    IN gctUINT32 RegisterMemBase,
    IN gctSIZE_T RegisterMemSize,
    IN gctUINT32 ContiguousBase,
    IN gctSIZE_T ContiguousSize,
    IN gctSIZE_T BankSize,
    IN gctINT FastClear,
	IN gctINT Compression,
	IN gctUINT32 BaseAddress,
	IN gctINT Signal,
    OUT gcoGALDEVICE *Device
    )
{
    gctUINT32 internalBaseAddress, internalAlignment;
    gctUINT32 externalBaseAddress, externalAlignment;
    gctUINT32 horizontalTileSize, verticalTileSize;
    gctUINT32 physAddr;
    gctUINT32 physical;
    gcoGALDEVICE device;
    gceSTATUS status;

    gcmTRACE(gcvLEVEL_VERBOSE, "[galcore] Enter gcoGALDEVICE_Construct\n");

    printk("\n[galcore] registerBase =0x%08x, registerMemSize = 0x%08x, contiguousBase= 0x%08x, contiguousSize = 0x%08x\n", 
              (gctUINT32)RegisterMemBase, (gctUINT32)RegisterMemSize, (gctUINT32)ContiguousBase, (gctUINT32)ContiguousSize);

    /* Allocate device structure. */
    device = kmalloc(sizeof(struct _gcoGALDEVICE), GFP_KERNEL);
    if (!device)
    {
    	gcmTRACE_ZONE(gcvLEVEL_ERROR, gcvZONE_DRIVER,
    	    	      "[galcore] gcoGALDEVICE_Construct: Can't allocate memory.\n");

    	return gcvSTATUS_OUT_OF_MEMORY;
    }
    memset(device, 0, sizeof(struct _gcoGALDEVICE));

    physical = RegisterMemBase;

    /* Set up register memory region */
    if (physical != 0)
    {
    	/* Request a region. */
    	request_mem_region(RegisterMemBase, RegisterMemSize, "galcore register region");
        device->registerBase = (gctPOINTER) ioremap_nocache(RegisterMemBase,
	    	    	    	    	    	    	    RegisterMemSize);
        if (!device->registerBase)
	{
    	    gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_DRIVER,
	    	    	  "[galcore] gcoGALDEVICE_Construct: Unable to map location->0x%lX for size->%ld\n",
			  RegisterMemBase,
			  RegisterMemSize);

    	    return gcvSTATUS_OUT_OF_RESOURCES;
        }

    	physical += RegisterMemSize;

		gcmTRACE_ZONE(gcvLEVEL_INFO,
					gcvZONE_DRIVER,
        			"[galcore] gcoGALDEVICE_Construct: "
					"RegisterBase after mapping Address->0x%x is 0x%x\n",
               		(gctUINT32)RegisterMemBase,
					(gctUINT32)device->registerBase);
    }

	/* construct the gcoOS object */
	device->baseAddress = BaseAddress;
    gcmVERIFY_OK(gcoOS_Construct(device, &device->os));

    /* construct the gcoKERNEL object. */
    gcmVERIFY_OK(gcoKERNEL_Construct(device->os, device, &device->kernel));

    gcmVERIFY_OK(gcoHARDWARE_SetFastClear(device->kernel->hardware,
    					  				  FastClear,
										  Compression));

    /* query the ceiling of the system memory */
    gcmVERIFY_OK(gcoHARDWARE_QuerySystemMemory(device->kernel->hardware,
					&device->systemMemorySize,
                    &device->systemMemoryBaseAddress));

	gcmTRACE_ZONE(gcvLEVEL_INFO,
					gcvZONE_DRIVER,
					"[galcore] gcoGALDEVICE_Construct: "
    				"Will be trying to allocate contiguous memory of 0x%x bytes\n",
           			(gctUINT32)device->systemMemoryBaseAddress);

#if COMMAND_PROCESSOR_VERSION == 1
    /* start the command queue */
    gcmVERIFY_OK(gcoCOMMAND_Start(device->kernel->command));
#endif

    /* initialize the thread daemon */
	sema_init(&device->sema, 0);
	device->threadInitialized = gcvFALSE;
	device->killThread = gcvFALSE;

	/* initialize the isr */
	device->isrInitialized = gcvFALSE;
	device->dataReady = gcvFALSE;
	device->irqLine = IrqLine;

	device->signal = Signal;

	/* query the amount of video memory */
    gcmVERIFY_OK(gcoHARDWARE_QueryMemory(device->kernel->hardware,
                    &device->internalSize,
                    &internalBaseAddress,
                    &internalAlignment,
                    &device->externalSize,
                    &externalBaseAddress,
                    &externalAlignment,
                    &horizontalTileSize,
                    &verticalTileSize));

	/* set up the internal memory region */
    if (device->internalSize > 0)
    {
        gceSTATUS status = gcoVIDMEM_Construct(device->os,
                    internalBaseAddress,
                    device->internalSize,
                    internalAlignment,
                    0,
                    &device->internalVidMem);

        if (gcmIS_ERROR(status))
        {
            /* error, remove internal heap */
            device->internalSize = 0;
        }
        else
        {
            /* map internal memory */
            device->internalPhysical  = (gctPHYS_ADDR)physical;
            device->internalLogical   = (gctPOINTER)ioremap_nocache(
					physical, device->internalSize);

            gcmASSERT(device->internalLogical != NULL);

			physical += device->internalSize;
        }
    }

    if (device->externalSize > 0)
    {
        /* create the external memory heap */
        gceSTATUS status = gcoVIDMEM_Construct(device->os,
					externalBaseAddress,
					device->externalSize,
					externalAlignment,
					0,
					&device->externalVidMem);

        if (gcmIS_ERROR(status))
        {
            /* error, remove internal heap */
            device->externalSize = 0;
        }
        else
        {
            /* map internal memory */
            device->externalPhysical = (gctPHYS_ADDR)physical;
            device->externalLogical = (gctPOINTER)ioremap_nocache(
					physical, device->externalSize);

			gcmASSERT(device->externalLogical != NULL);

			physical += device->externalSize;
        }
    }

	/* set up the contiguous memory */
    device->contiguousSize = ContiguousSize;

	if (ContiguousBase == 0)
	{
		status = gcvSTATUS_OUT_OF_MEMORY;

		while (device->contiguousSize > 0)
		{
			gcmTRACE_ZONE(
				gcvLEVEL_INFO, gcvZONE_DRIVER,
				"[galcore] gcoGALDEVICE_Construct: Will be trying to allocate contiguous memory of %ld bytes\n",
				device->contiguousSize
				);

			/* allocate contiguous memory */
			status = gcoGALDEVICE_AllocateMemory(
				device,
				device->contiguousSize,
				&device->contiguousBase,
				&device->contiguousPhysical,
				&physAddr
				);

			if (gcmIS_SUCCESS(status))
			{
	    		gcmTRACE_ZONE(
					gcvLEVEL_INFO, gcvZONE_DRIVER,
					"[galcore] gcoGALDEVICE_Construct: Contiguous allocated size->0x%08X Virt->0x%08lX physAddr->0x%08X\n",
					device->contiguousSize,
					device->contiguousBase,
					physAddr
					);

				status = gcoVIDMEM_Construct(
					device->os,
					physAddr | device->systemMemoryBaseAddress,
					device->contiguousSize,
					64,
					BankSize,
					&device->contiguousVidMem
					);

				if (gcmIS_SUCCESS(status))
				{
					device->contiguousMapped = gcvFALSE;

					/* success, abort loop */
					gcmTRACE_ZONE(
						gcvLEVEL_INFO, gcvZONE_DRIVER,
						"Using %u bytes of contiguous memory.\n",
						device->contiguousSize
						);

					break;
				}

				gcmVERIFY_OK(gcoGALDEVICE_FreeMemory(
					device,
					device->contiguousBase,
					device->contiguousPhysical
					));

				device->contiguousBase = NULL;
			}

			device->contiguousSize -= (4 << 20);
		}
	}
	else
	{
		/* Create the contiguous memory heap. */
		status = gcoVIDMEM_Construct(
			device->os,
			(ContiguousBase - device->baseAddress) | device->systemMemoryBaseAddress,
			ContiguousSize,
			64,
			BankSize,
			&device->contiguousVidMem
			);

		if (gcmIS_ERROR(status))
		{
			/* Error, roll back. */
			device->contiguousVidMem = gcvNULL;
			device->contiguousSize   = 0;
		}
		else
		{
			/* Map the contiguous memory. */
			request_mem_region(
				ContiguousBase,
				ContiguousSize,
				"galcore managed memory"
				);

			device->contiguousPhysical = (gctPHYS_ADDR) ContiguousBase;
			device->contiguousSize     = ContiguousSize;
			device->contiguousBase     = (gctPOINTER) ioremap_nocache(ContiguousBase, ContiguousSize);
			device->contiguousMapped   = gcvTRUE;

			if (device->contiguousBase == gcvNULL)
			{
    			/* Error, roll back. */
				gcmVERIFY_OK(gcoVIDMEM_Destroy(device->contiguousVidMem));
				device->contiguousVidMem = gcvNULL;
				device->contiguousSize   = 0;

				status = gcvSTATUS_OUT_OF_RESOURCES;
			}
		}
	}

    *Device = device;
    
    printk("\n[galcore] real contiguouSize = 0x%08x \n",  (gctUINT32)(device->contiguousSize));
    
    gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_DRIVER,
    	    	  "[galcore] gcoGALDEVICE_Construct: Initialized device->%p contiguous->%lu @ %p (0x%08X)\n",
		  device,
		  device->contiguousSize,
		  device->contiguousBase,
		  device->contiguousPhysical);

    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoGALDEVICE_Destroy
**
**	Class destructor.
**
**	INPUT:
**
**		Nothing.
**
**	OUTPUT:
**
**		Nothing.
**
**	RETURNS:
**
**		Nothing.
*/
gceSTATUS
gcoGALDEVICE_Destroy(
	gcoGALDEVICE Device)
{
	gcmVERIFY_ARGUMENT(Device != NULL);

    gcmTRACE(gcvLEVEL_VERBOSE, "[ENTER] gcoGALDEVICE_Destroy\n");

    /* Destroy the gcoKERNEL object. */
    gcmVERIFY_OK(gcoKERNEL_Destroy(Device->kernel));

    if (Device->internalVidMem != gcvNULL)
    {
        /* destroy the internal heap */
        gcmVERIFY_OK(gcoVIDMEM_Destroy(Device->internalVidMem));

		/* unmap the internal memory */
		iounmap(Device->internalLogical);
    }

    if (Device->externalVidMem != gcvNULL)
    {
        /* destroy the internal heap */
        gcmVERIFY_OK(gcoVIDMEM_Destroy(Device->externalVidMem));

        /* unmap the external memory */
		iounmap(Device->externalLogical);
    }

    if (Device->contiguousVidMem != gcvNULL)
    {
        /* Destroy the contiguous heap */
        gcmVERIFY_OK(gcoVIDMEM_Destroy(Device->contiguousVidMem));

	if (Device->contiguousMapped)
	{
    	    gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_DRIVER,
	        	  "[galcore] gcoGALDEVICE_Destroy: "
			  "Unmapping contiguous memory->0x%08lX\n",
			  Device->contiguousBase);

	    iounmap(Device->contiguousBase);
	}
	else
	{
    	    gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_DRIVER,
	        	  "[galcore] gcoGALDEVICE_Destroy: "
			  "Freeing contiguous memory->0x%08lX\n",
			  Device->contiguousBase);

    	    gcmVERIFY_OK(gcoGALDEVICE_FreeMemory(Device,
						 Device->contiguousBase,
						 Device->contiguousPhysical));
    	}
    }

    if (Device->registerBase)
    {
        iounmap(Device->registerBase);
    }

    /* Destroy the gcoOS object. */
    gcmVERIFY_OK(gcoOS_Destroy(Device->os));

    kfree(Device);

    gcmTRACE(gcvLEVEL_VERBOSE, "[galcore] Leave gcoGALDEVICE_Destroy\n");

    return gcvSTATUS_OK;
}
