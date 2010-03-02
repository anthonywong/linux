##############################################################################
#  
#    Copyright (C) 2002 - 2008 by Vivante Corp.
#  
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public Lisence as published by
#    the Free Software Foundation; either version 2 of the license, or
#    (at your option) any later version.
#  
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#    GNU General Public Lisence for more details.
#  
#    You should have received a copy of the GNU General Public License
#    along with this program; if not write to the Free Software
#    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#  
##############################################################################



# Makefile for building the kernel driver of HAL

STRIP=$(CROSS_COMPILE)-strip

ifeq ($(FLAREON),)
else
  CFLAGS += -DFLAREON
endif

ifneq ($(KERNELRELEASE), )

# build hal_kernel
#export CC=arm-none-linux-gnueabi-gcc
#export LD=arm-none-linux-gnueabi-ld
#export AS=arm-none-linux-gnueabi-as

HAL_OS_KERNEL_DIR   := .
HAL_KERNEL_DIR      := ../../../../hal/kernel
HAL_ARCH_KERNEL_DIR := ../../../../arch/$(notdir $(AQARCH))/hal/kernel

HAL_OS_KERNEL_OBJS := $(HAL_OS_KERNEL_DIR)/Driver.o \
		      $(HAL_OS_KERNEL_DIR)/Device.o \
		      $(HAL_OS_KERNEL_DIR)/Debug.o \
		      $(HAL_OS_KERNEL_DIR)/Kernel.o \
		      $(HAL_OS_KERNEL_DIR)/Os.o

HAL_KERNEL_OBJS := $(HAL_KERNEL_DIR)/Command.o \
		   $(HAL_KERNEL_DIR)/Event.o \
		   $(HAL_KERNEL_DIR)/Heap.o \
		   $(HAL_KERNEL_DIR)/Kernel.o \
		   $(HAL_KERNEL_DIR)/Mmu.o \
		   $(HAL_KERNEL_DIR)/VidMem.o

HAL_ARCH_KERNEL_OBJS := $(HAL_ARCH_KERNEL_DIR)/Hardware.o

HAL_KERNEL_INCLUDE_DIR = -I$(AQROOT)/hal/inc \
                         -I$(AQROOT)/hal/os/linux/kernel \
			 -I$(AQROOT)/hal/kernel \
			 -I$(AQARCH)/hal/kernel \
			 -I$(AQARCH)/cmodel/inc \
			 -I$(AQROOT)/hal/user
			 
ifeq ($(DEBUG), )
	CFLAGS+=-O2
else
	CFLAGS+=-g -D_DEBUG
endif

EXTRA_CFLAGS += -DLINUX -DDRIVER $(HAL_KERNEL_INCLUDE_DIR) -DDBG=0

obj-m := aqhal.o
aqhal-objs := $(HAL_OS_KERNEL_OBJS) $(HAL_KERNEL_OBJS) $(HAL_ARCH_KERNEL_OBJS)

else 
	KERNEL_BUILD_DIR ?= /lib/modules/$(shell uname -r)/build
	ARCH_TYPE ?= arm
	PWD := $(shell pwd)

default:
	$(MAKE) ARCH=$(ARCH_TYPE) -C $(KERNEL_BUILD_DIR) M=$(PWD) CROSS_COMPILE=$(CROSS_COMPILE)- modules || exit 1

clean:
	$(MAKE) V=$(V) -C $(KERNEL_BUILD_DIR) M=$(PWD) clean
	@rm -rf partial
	@rm -rf *.o *.obj

endif

