#
# Makefile for the Marvell Audio ALSA Device Driver
#

ifdef CONFIG_MV_HAL_RULES_PATH
include $(TOPDIR)/include/config/auto.conf
include $(TOPDIR)/$(subst ",,$(CONFIG_MV_HAL_RULES_PATH))
endif

obj-$(CONFIG_SND_DOVE_SOC_MRVL_AUDIO) += snd-mv88fx-soc.o
snd-mv88fx-soc-objs		     += mv88fx-pcm.o  mv88fx-i2s.o mv88fx.o

