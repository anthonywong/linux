# Display drivers

display-objs				:= display-sysfs.o

obj-$(CONFIG_DISPLAY_SUPPORT)		+= display.o

obj-$(CONFIG_CH7025_COMPOSITE)  += ch7025_composite.o
obj-$(CONFIG_KG2_ANX7150) += ./kg2/kg2.o
obj-$(CONFIG_KG2_ANX7150) += ./ANX7150/ANX7150_i2c_intf.o
obj-$(CONFIG_KG2_ANX7150) += ./ANX7150/ANX7150_Sys7150.o
obj-$(CONFIG_KG2_ANX7150) += ./ANX7150/ANX7150_System_Config.o
obj-$(CONFIG_KG2_ANX7150) += ./ANX7150/ANX7150.o
