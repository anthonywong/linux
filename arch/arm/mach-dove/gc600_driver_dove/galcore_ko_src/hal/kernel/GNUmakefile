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



#
# This file is a part of Vivante software stack build environment for Linux.
# It defines the rules necessary to build the kernel HAL driver.
#


################################################################################
# Include common definitions.

include $(AQROOT)/GNUmakefile.common


################################################################################
# Define a shortcut for the main target.

TARGET_OUTPUT = $(OBJ_DIR)/libHALKernel.a


################################################################################
# Include common targets.

include $(AQROOT)/GNUmakefile.source


################################################################################
# Supply additional include directories.

INCLUDE += -I$(AQROOT)/hal/inc
INCLUDE += -I$(AQROOT)/hal/user
INCLUDE += -I$(AQARCH)/hal/kernel

CFLAGS += $(INCLUDE)


################################################################################
# Describe object files.

OBJECTS = $(OBJ_DIR)/Command.o \
          $(OBJ_DIR)/Event.o \
          $(OBJ_DIR)/Heap.o \
          $(OBJ_DIR)/Kernel.o \
          $(OBJ_DIR)/Mmu.o \
          $(OBJ_DIR)/VidMem.o


################################################################################
# Describe dependencies.

$(TARGET_OUTPUT): $(OBJ_DIR) $(PRE_COMPILED) $(OBJECTS)
	$(AR) -r $(TARGET_OUTPUT) $(OBJECTS)
	$(RANLIB) $(TARGET_OUTPUT)
