#ifndef __ASM_ARCH_DOVE_BL_H
#define __ASM_ARCH_DOVE_BL_H

struct dovebl_platform_data {
	int	default_intensity;
	int	max_brightness;
	int	gpio_pm_control; /* enable LCD/panel power management via gpio*/
};
#endif /* __ASM_ARCH_DOVE_BL_H */
