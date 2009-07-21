#*******************************************************************************
# Marvell GPL License Option
#
# If you received this File from Marvell, you may opt to use, redistribute and/or 
# modify this File in accordance with the terms and conditions of the General 
# Public License Version 2, June 1991 (the "GPL License"), a copy of which is 
# available along with the File in the license.txt file or by writing to the Free 
# Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or 
# on the worldwide web at http://www.gnu.org/licenses/gpl.txt. 
#
# THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED 
# WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY 
# DISCLAIMED.  The GPL License provides additional details about this warranty 
# disclaimer.
#*******************************************************************************/
include $(TOPDIR)/$(subst ",,$(CONFIG_MV_HAL_RULES_PATH))

# Objects list
COMMON_OBJS	= $(COMMON_DIR)/mvDebug.o $(COMMON_DIR)/mvCommon.o $(COMMON_DIR)/mvStack.o

OSSERVICES_OBJS	= $(OSSERV_DIR)/mvOs.o

QD_OBJS		= $(HAL_QD_DIR)/src/driver/gtDrvConfig.o $(HAL_QD_DIR)/src/driver/gtDrvEvents.o \
                  $(HAL_QD_DIR)/src/driver/gtHwCntl.o $(HAL_QD_DIR)/src/platform/gtMiiSmiIf.o	\
                  $(HAL_QD_DIR)/src/platform/platformDeps.o $(HAL_QD_DIR)/src/platform/gtSem.o	\
                  $(HAL_QD_DIR)/src/platform/gtDebug.o $(HAL_QD_DIR)/src/msapi/gtBrgFdb.o 	\
                  $(HAL_QD_DIR)/src/msapi/gtBrgStp.o $(HAL_QD_DIR)/src/msapi/gtBrgVlan.o 	\
                  $(HAL_QD_DIR)/src/msapi/gtEvents.o $(HAL_QD_DIR)/src/msapi/gtPortCtrl.o 	\
                  $(HAL_QD_DIR)/src/msapi/gtPortStat.o $(HAL_QD_DIR)/src/msapi/gtPortStatus.o 	\
                  $(HAL_QD_DIR)/src/msapi/gtQosMap.o $(HAL_QD_DIR)/src/msapi/gtPIRL.o  		\
                  $(HAL_QD_DIR)/src/msapi/gtPhyCtrl.o $(HAL_QD_DIR)/src/msapi/gtPhyInt.o 	\
                  $(HAL_QD_DIR)/src/msapi/gtSysConfig.o $(HAL_QD_DIR)/src/msapi/gtSysCtrl.o	\
                  $(HAL_QD_DIR)/src/msapi/gtVersion.o $(HAL_QD_DIR)/src/msapi/gtUtils.o 	\
                  $(HAL_QD_DIR)/src/msapi/gtBrgVtu.o $(HAL_QD_DIR)/src/msapi/gtPortRmon.o 	\
                  $(HAL_QD_DIR)/src/msapi/gtSysStatus.o $(HAL_QD_DIR)/src/msapi/gtPortRateCtrl.o\
                  $(HAL_QD_DIR)/src/msapi/gtPortPav.o $(HAL_QD_DIR)/src/msapi/gtVct.o		\
		  $(HAL_QD_DIR)/src/msapi/gtPIRL2.o

obj-y	+=$(LSP_OBJS) $(COMMON_OBJS) $(OSSERVICES_OBJS)  $(HAL_OBJS)

obj-$(CONFIG_MV_INCLUDE_XOR) 	+= $(HAL_XOR_DIR)/mvXor.o 
obj-$(CONFIG_MV_INCLUDE_USB) 	+= $(HAL_USB_DIR)/mvUsb.o
obj-$(CONFIG_MV_INCLUDE_GIG_ETH)   +=	$(HAL_ETHPHY_DIR)/mvEthPhy.o \
					$(HAL_ETH_DIR)/mvEth.o 	\
					$(HAL_ETH_DIR)/mvEthDebug.o

obj-$(CONFIG_MV_GATEWAY) 		+= $(QD_OBJS)

obj-$(CONFIG_MV_ETH_NFP)		+= $(HAL_ETH_NFP_DIR)/mvNfp.o 
obj-$(CONFIG_MV_ETH_NFP_NAT_SUPPORT)	+= $(HAL_ETH_NFP_DIR)/mvNfpNat.o
obj-$(CONFIG_MV_ETH_NFP_FDB_SUPPORT)	+= $(HAL_ETH_NFP_DIR)/mvNfpFdb.o  


obj-$(CONFIG_MV_INCLUDE_CESA) 	+= $(HAL_CESA_DIR)/mvCesa.o \
				$(HAL_CESA_DIR)/mvCesaDebug.o \
				$(HAL_CESA_DIR)/mvMD5.o \
				$(HAL_CESA_DIR)/mvSHA1.o \
				$(HAL_CESA_AES_DIR)/mvAesAlg.o \
				$(HAL_CESA_AES_DIR)/mvAesApi.o

feroceon-$(CONFIG_MV_CESA_TEST) 	+= $(HAL_CESA_DIR)/mvCesaTest.o
obj-$(CONFIG_MV_INCLUDE_AUDIO) 	+= $(HAL_AUDIO_DIR)/mvAudio.o \
#		$(SOC_SYS_DIR)/mvSysAudio.o\
#		 $(HAL_AUDIO_DIR)/dac/mvCLAudioCodec.o \
#		../../plat-orion/mv_hal_drivers/mv_drivers_lsp/mv_audio/i2c-dummy.o	

AFLAGS_mvPmu_asm.o := $(EXTRA_INCLUDE) $(MV_DEFINE)
obj-$(CONFIG_MV_INCLUDE_PMU) 	+= $(HAL_PMU_DIR)/mvPmu.o \
				   $(HAL_PMU_DIR)/mvPmu_asm.o \
				   $(HAL_PMU_DIR)/mvSram.o

obj-$(CONFIG_MV_INCLUDE_NFC)	+= $(HAL_NFC_DIR)/mvNfc.o \
				   $(HAL_PDMA_DIR)/mvPdma.o

obj-$(CONFIG_MV_DDRMMC)	+=$(HAL_DDRMC_DIR)/mvDramTmng.o

# core drivers part

obj-$(CONFIG_MV_USE_XOR_ENGINE) 	+= ../../plat-orion/mv_hal_drivers/mv_drivers_lsp/mv_xor/
obj-$(CONFIG_MV_CESA) 			+= ../../plat-orion/mv_hal_drivers/mv_drivers_lsp/mv_cesa/

# The rest of the drivers are compiled through the driver dir directly.


# LSP part
#obj-$(CONFIG_MV_INCLUDE_USB)    	+= $(LSP_DIR)/usb.o


obj-y    	+= ../../plat-orion/mv_hal_drivers/mv_drivers_lsp/mv_key/
