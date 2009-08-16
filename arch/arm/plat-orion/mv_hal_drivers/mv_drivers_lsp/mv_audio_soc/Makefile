#
# Makefile for the Marvell Audio ALSA Device Driver
#

ifdef CONFIG_MV_HAL_RULES_PATH
include $(srctree)/$(subst ",,$(CONFIG_MV_HAL_RULES_PATH))
endif

obj-$(CONFIG_SND_DOVE_SOC_MRVL_AUDIO) += snd-mv88fx-soc.o
obj-$(CONFIG_SND_DOVE_SOC_MRVL_AUDIO) += snd-mv88fx-i2s-soc.o
obj-$(CONFIG_SND_DOVE_SOC_MRVL_AUDIO) += snd-mv88fx-pcm-soc.o

snd-mv88fx-i2s-soc-objs               += mv88fx-i2s.o
snd-mv88fx-pcm-soc-objs		      += mv88fx-pcm.o

snd-mv88fx-soc-objs                  += mv88fx-rt5623.o
snd-mv88fx-soc-objs                  += mv88fx.o


