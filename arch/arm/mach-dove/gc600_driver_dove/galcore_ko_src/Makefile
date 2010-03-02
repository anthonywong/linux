##############################################################################
#  
#    Copyright (C) 2005 - 2009 by Vivante Corp.
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
PWD := $(srctree)/arch/arm/mach-dove/gc600_driver_dove/galcore_ko_src

ARCH=arm
CROSS_COMPILE = arm-marvell-linux-gnueabi-
GPU_TYPE = Unified
KERNEL_DIR = ${PWD}/linux

DEBUG = 0
ABI = aapcs-linux
ENABLE_ARM_L2_CACHE = 1
NO_DMA_COHERENT = 1
CONFIG_DOVE_GPU = 1
ANDROID = 0
ENABLE_GPU_CLOCK_BY_DRIVER = 0

BUILD_OPTIONS  = DEBUG=${DEBUG}
BUILD_OPTIONS += ABI=${ABI}
BUILD_OPTIONS += ENABLE_ARM_L2_CACHE=${ENABLE_ARM_L2_CACHE}
BUILD_OPTIONS += NO_DMA_COHERENT=${NO_DMA_COHERENT}
BUILD_OPTIONS += CONFIG_DOVE_GPU=${CONFIG_DOVE_GPU}
BUILD_OPTIONS += ANDROID=${ANDROID}
BUILD_OPTIONS += ENABLE_GPU_CLOCK_BY_DRIVER=${ENABLE_GPU_CLOCK_BY_DRIVER}
AQROOT := $(PWD)
AQARCH = ${PWD}/arch/${GPU_TYPE}


#
# Linux build file for kernel HAL driver.
#
include $(srctree)/arch/arm/mach-dove/gc600_driver_dove/galcore_ko_src/config
#include $(AQROOT)/config

DRIVER_OUT_DIR = hal/driver
KERNEL_DIR ?= $(TOOL_DIR)/kernel

OS_KERNEL_DIR   := hal/os/linux/kernel
ARCH_KERNEL_DIR := arch/$(notdir $(AQARCH))/hal/kernel
HAL_KERNEL_DIR  := hal/kernel

OBJS := $(OS_KERNEL_DIR)/Driver.o \
        $(OS_KERNEL_DIR)/Device.o \
        $(OS_KERNEL_DIR)/Debug.o \
        $(OS_KERNEL_DIR)/Kernel.o \
        $(OS_KERNEL_DIR)/Os.o

ifeq ($(USE_3D_VG), 1)

OBJS += $(HAL_KERNEL_DIR)/Command.o \
        $(HAL_KERNEL_DIR)/Event.o \
        $(HAL_KERNEL_DIR)/Heap.o \
        $(HAL_KERNEL_DIR)/Kernel.o \
        $(HAL_KERNEL_DIR)/Mmu.o \
        $(HAL_KERNEL_DIR)/VidMem.o

OBJS += $(ARCH_KERNEL_DIR)/Hardware.o

else

OBJS += $(HAL_KERNEL_DIR)/Command.o \
        $(HAL_KERNEL_DIR)/Heap.o \
        $(HAL_KERNEL_DIR)/Kernel.o \
        $(HAL_KERNEL_DIR)/Mmu.o \
        $(HAL_KERNEL_DIR)/VidMem.o \
        $(HAL_KERNEL_DIR)/Interrupt.o

OBJS += $(ARCH_KERNEL_DIR)/Command.o \
        $(ARCH_KERNEL_DIR)/Hardware.o

endif

ifeq ($(KERNELRELEASE), )

.PHONY: all clean install

# Define targets.
all:
	@mkdir -p $(DRIVER_OUT_DIR)
	@make V=$(V) ARCH=$(ARCH_TYPE) -C $(KERNEL_DIR) SUBDIRS=`pwd` modules

clean:
	@rm -rf $(OBJS)
	@rm -rf $(DRIVER_OUT_DIR)
	@rm -rf modules.order Module.symvers

install: all
	@mkdir -p $(SDK_DIR)/drivers
	@cp $(DRIVER_OUT_DIR)/galcore.ko $(SDK_DIR)/drivers

else


EXTRA_CFLAGS += -DLINUX -DDRIVER
EXTRA_CFLAGS += -Werror

ifeq ($(ENUM_WORKAROUND), 1)
EXTRA_CFLAGS += -DENUM_WORKAROUND=1
else
EXTRA_CFLAGS += -DENUM_WORKAROUND=0
endif

ifeq ($(FLAREON),1)
EXTRA_CFLAGS += -DFLAREON
endif

ifeq ($(DEBUG), 1)
EXTRA_CFLAGS += -DDBG=1 -DDEBUG -D_DEBUG
else
EXTRA_CFLAGS += -DDBG=0
endif

ifeq ($(NO_DMA_COHERENT), 1)
EXTRA_CFLAGS += -DNO_DMA_COHERENT
endif

ifeq ($(ENABLE_ARM_L2_CACHE), 1)
EXTRA_CFLAGS += -DENABLE_ARM_L2_CACHE=1
endif

ifeq ($(CONFIG_DOVE_GPU), 1)
EXTRA_CFLAGS += -DCONFIG_DOVE_GPU=1
endif

ifeq ($(USE_VIVANTE), 1)
EXTRA_CFLAGS += -DVIVANTE_PROFILER=1
else
EXTRA_CFLAGS += -DVIVANTE_PROFILER=0
endif

ifeq ($(ANDROID), 1)
EXTRA_CFLAGS += -DANDROID=1
endif

ifeq ($(ENABLE_GPU_CLOCK_BY_DRIVER), 1)
EXTRA_CFLAGS += -DENABLE_GPU_CLOCK_BY_DRIVER=1
else
EXTRA_CFLAGS += -DENABLE_GPU_CLOCK_BY_DRIVER=0
endif

EXTRA_CFLAGS += -I$(AQROOT)/hal/inc
EXTRA_CFLAGS += -I$(AQROOT)/hal/kernel
EXTRA_CFLAGS += -I$(AQARCH)/hal/kernel
EXTRA_CFLAGS += -I$(AQARCH)/cmodel/inc
EXTRA_CFLAGS += -I$(AQROOT)/hal/user

obj-m = galcore.o
galcore-objs  = $(OBJS)

endif
