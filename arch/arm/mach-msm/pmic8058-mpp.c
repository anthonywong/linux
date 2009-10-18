/* Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
/*
 * Qualcomm PMIC8058 MPP driver
 *
 */

#include <linux/gpio.h>
#include <linux/mfd/pmic8058.h>
#include "gpio_chip.h"

#define PM8058_MPP_TO_INT(n) (PMIC8058_IRQ_BASE + NR_PMIC8058_GPIO_IRQS + (n))

static int pm8058_mpp_get_irq_num(struct gpio_chip *chip,
				   unsigned int gpio,
				   unsigned int *irqp,
				   unsigned long *irqnumflagsp)
{
	gpio -= chip->start;
	*irqp = PM8058_MPP_TO_INT(gpio);
	if (irqnumflagsp)
		*irqnumflagsp = 0;
	return 0;
}

static int pm8058_mpp_read(struct gpio_chip *chip, unsigned n)
{
	n -= chip->start;
	return pm8058_mpp_get(n);
}

struct msm_gpio_chip pm8058_mpp_chip = {
	.chip = {
		.start = NR_GPIO_IRQS + NR_PMIC8058_GPIO_IRQS,
		.end = NR_GPIO_IRQS + NR_PMIC8058_GPIO_IRQS +
			NR_PMIC8058_MPP_IRQS - 1,
		.get_irq_num = pm8058_mpp_get_irq_num,
		.read = pm8058_mpp_read,
	}
};

static int __init pm8058_mpp_init(void)
{
	int	rc;

	rc = register_gpio_chip(&pm8058_mpp_chip.chip);
	pr_info("%s: register_gpio_chip(): rc=%d\n", __func__, rc);

	return rc;
}
device_initcall(pm8058_mpp_init);
