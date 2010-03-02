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
# It defines the rules necessary to build Linux kernel HAL driver.
#


################################################################################
# Include common definitions.

include $(AQROOT)/GNUmakefile.common


################################################################################
# Define a shortcut for the main target.

TARGET_OUTPUT = $(OBJ_DIR)/GALCore.ko


################################################################################
# Specify targets.

all:	$(OBJ_DIR)
	@make ARCH=$(ARCH_TYPE) -C $(KERNEL_DIR) SUBDIRS=`pwd`

install:
	@-mkdir -p $(DRIVERS_COPY_DIR)
	@cp $(TARGET_OUTPUT) $(DRIVERS_COPY_DIR)

clean:
	@make V=$(V) -C $(KERNEL_DIR) SUBDIRS=`pwd` clean
	@rm -f  build.log
	@rm -f  PreComp.h.gch
	@rm -f  Module.symvers
	@rm -rf $(OBJ_DIR)


################################################################################
# Describe dependencies.

$(OBJ_DIR):
	@-mkdir -p $(OBJ_DIR)
