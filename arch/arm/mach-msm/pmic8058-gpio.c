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
 * Qualcomm PMIC8058 GPIO driver
 *
 */

#include <linux/gpio.h>
#include <linux/mfd/pmic8058.h>
#include "gpio_chip.h"

#define PM8058_GPIO_TO_INT(n) (PMIC8058_IRQ_BASE + (n))

static int pm8058_gpio_configure(struct gpio_chip *chip,
				 unsigned int gpio,
				 unsigned long flags)
{
	int	rc = 0, direction;

	gpio -= chip->start;

	if (flags & (GPIOF_INPUT | GPIOF_DRIVE_OUTPUT)) {
		direction = 0;
		if (flags & GPIOF_INPUT)
			direction |= PM_GPIO_DIR_IN;
		if (flags & GPIOF_DRIVE_OUTPUT)
			direction |= PM_GPIO_DIR_OUT;

		if (flags & (GPIOF_OUTPUT_LOW | GPIOF_OUTPUT_HIGH)) {
			if (flags & GPIOF_OUTPUT_HIGH)
				rc = pm8058_gpio_set(gpio, 1);
			else
				rc = pm8058_gpio_set(gpio, 0);

			if (rc) {
				pr_err("%s: FAIL pm8058_gpio_set(): rc=%d.\n",
					__func__, rc);
				goto bail_out;
			}
		}

		rc = pm8058_gpio_set_direction(gpio, direction);
		if (rc)
			pr_err("%s: FAIL pm8058_gpio_config(): rc=%d.\n",
				__func__, rc);
	}

bail_out:
	return rc;
}

static int pm8058_gpio_get_irq_num(struct gpio_chip *chip,
				   unsigned int gpio,
				   unsigned int *irqp,
				   unsigned long *irqnumflagsp)
{
	gpio -= chip->start;
	*irqp = PM8058_GPIO_TO_INT(gpio);
	if (irqnumflagsp)
		*irqnumflagsp = 0;
	return 0;
}

static int pm8058_gpio_read(struct gpio_chip *chip, unsigned n)
{
	n -= chip->start;
	return pm8058_gpio_get(n);
}

static int pm8058_gpio_write(struct gpio_chip *chip, unsigned n, unsigned on)
{
	n -= chip->start;
	return pm8058_gpio_set(n, on);
}

struct msm_gpio_chip pm8058_gpio_chip = {
	.chip = {
		.start = NR_GPIO_IRQS,
		.end = NR_GPIO_IRQS + NR_PMIC8058_GPIO_IRQS - 1,
		.configure = pm8058_gpio_configure,
		.get_irq_num = pm8058_gpio_get_irq_num,
		.read = pm8058_gpio_read,
		.write = pm8058_gpio_write,
	}
};

static int __init pm8058_gpio_init(void)
{
	int	rc;

	rc = register_gpio_chip(&pm8058_gpio_chip.chip);
	pr_info("%s: register_gpio_chip(): rc=%d\n", __func__, rc);

	return rc;
}
device_initcall(pm8058_gpio_init);
