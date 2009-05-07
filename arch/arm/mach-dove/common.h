/*
 * arch/arm/mach-dove/common.h
 *
 * Core functions for Marvell Dove MV88F6781 System On Chip
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __ARCH_DOVE_COMMON_H
#define __ARCH_DOVE_COMMON_H

struct mv643xx_eth_platform_data;
struct mv_sata_platform_data;
struct dovefb_mach_info;
struct gpio_mouse_platform_data;
struct orion_i2s_platform_data;
struct cafe_cam_platform_data;

extern struct sys_timer dove_timer;
extern struct mbus_dram_target_info dove_mbus_dram_info;

int dove_tclk_get(void);

/*
 * Basic Dove init functions used early by machine-setup.
 */
int get_tclk(void);
void dove_map_io(void);
void dove_init(void);
void dove_init_irq(void);
void dove_setup_cpu_mbus(void);
void dove_ge00_init(struct mv643xx_eth_platform_data *eth_data);
void dove_mv_eth_init(void);
void dove_sata_init(struct mv_sata_platform_data *sata_data);
void dove_fb_init(struct dovefb_mach_info *fb_data);
void dove_pcie_init(int init_port0, int init_port1);
void dove_rtc_init(void);
void dove_ehci0_init(void);
void dove_ehci1_init(void);
void dove_sdio0_init(void);
void dove_sdio1_init(void);
void dove_uart0_init(void);
void dove_uart1_init(void);
void dove_uart2_init(void);
void dove_uart3_init(void);
void dove_spi0_init(int use_interrupt);
void dove_spi1_init(int use_interrupt);
void dove_i2c_init(void);
void dove_i2s_init(int port, struct orion_i2s_platform_data *i2s_data);
void dove_cam_init(struct cafe_cam_platform_data *cafe_cam_data);
void dove_sdhci_cam_mbus_init(void);
void dove_tact_init(struct gpio_mouse_platform_data *tact_data);
void dove_lcd_spi_init(void);
void dove_vpro_init(void);
void dove_gpu_init(void);
void dove_tag_fixup_mem32(struct machine_desc *mdesc, struct tag *t,
			  char **from, struct meminfo *meminfo);
void dove_sd_card_int_wa_setup(int port);
void dove_xor0_init(void);
void dove_xor1_init(void);
void dove_ac97_setup(void);
/*
 * Basic Dove PM functions
 */
#ifdef CONFIG_PM
void dove_save_cpu_wins(void);
void dove_restore_cpu_wins(void);
void dove_restore_pcie_regs(void);
void dove_save_cpu_conf_regs(void);
void dove_save_timer_regs(void);	
void dove_restore_timer_regs(void);	
void dove_restore_cpu_conf_regs(void);
void dove_save_int_regs(void);
void dove_restore_int_regs(void);
#endif

#endif
