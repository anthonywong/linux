ifeq ($(CONFIG_MMC_DEBUG),y)
	EXTRA_CFLAGS		+= -DDEBUG
endif

obj-$(CONFIG_MSPRO) += memstick.o mspro_block.o xd_card.o flash_bd.o jmb38x_ms.o jmb38x_xd.o

xd_card-objs := xd_card_blk.o xd_card_ecc.o

