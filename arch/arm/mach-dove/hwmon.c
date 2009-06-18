/*
 * hwmon-dove.c - temperature monitoring driver for Dove SoC
 *
 * Inspired from other hwmon drivers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/hwmon.h>
#include <linux/sysfs.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/platform_device.h>
#include <linux/cpu.h>
#include <asm/io.h>
#include "pmu/mvPmuRegs.h"

#define DOVE_OVERHEAT_TEMP	90		/* degrees */
#define DOVE_OVERHEAT_DELAY	0x700
#define DOVE_OVERCOOL_TEMP	10		/* degrees */
#define	DOVE_OVERCOOL_DELAY	0x700

#define DOVE_TSEN_TEMP2RAW(x)	((3043800 - (17294 * x)) / 10000)
#define DOVE_TSEN_RAW2TEMP(x)	((3043800 - (10000 * x)) / 17294)

static struct device *hwmon_dev;

typedef enum { 
	SHOW_TEMP, 
	SHOW_MAX,
	SHOW_MIN,
	SHOW_NAME} SHOW;


static int dovetemp_initSensor(void) {
	u32 reg;
	u32 i, temp;

	/* Configure the Diode Control Register #0 */
        reg = readl(INTER_REGS_BASE | PMU_TEMP_DIOD_CTRL0_REG);
	/* Use average of 2 */
	reg &= ~PMU_TDC0_AVG_NUM_MASK;
	reg |= (0x1 << PMU_TDC0_AVG_NUM_OFFS);
	/* Reference calibration value */
	reg &= ~PMU_TDC0_REF_CAL_CNT_MASK;
	reg |= (0x0F1 << PMU_TDC0_REF_CAL_CNT_OFFS);
	/* Set the high level reference for calibration */
	reg &= ~PMU_TDC0_SEL_VCAL_MASK;
	reg |= (0x2 << PMU_TDC0_SEL_VCAL_OFFS);
	writel(reg, (INTER_REGS_BASE | PMU_TEMP_DIOD_CTRL0_REG));

	/* Reset the sensor */
	reg = readl(INTER_REGS_BASE | PMU_TEMP_DIOD_CTRL0_REG);
	writel((reg | PMU_TDC0_SW_RST_MASK), (INTER_REGS_BASE | PMU_TEMP_DIOD_CTRL0_REG));
	writel(reg, (INTER_REGS_BASE | PMU_TEMP_DIOD_CTRL0_REG));	

	/* Enable the sensor */
	reg = readl(INTER_REGS_BASE | PMU_THERMAL_MNGR_REG);
	reg &= ~PMU_TM_DISABLE_MASK;
	writel(reg, (INTER_REGS_BASE | PMU_THERMAL_MNGR_REG));

	/* Poll the sensor for the first reading */
	for (i=0; i< 1000000; i++) {
		reg = readl(INTER_REGS_BASE | PMU_THERMAL_MNGR_REG);
		if (reg & PMU_TM_CURR_TEMP_MASK)
			break;;
	}

	if (i== 1000000)
		return -EIO;

	/* Set the overheat threashold & delay */
	temp = DOVE_TSEN_TEMP2RAW(DOVE_OVERHEAT_TEMP);
	reg = readl(INTER_REGS_BASE | PMU_THERMAL_MNGR_REG);
	reg &= ~PMU_TM_OVRHEAT_THRSH_MASK;
	reg |= (temp << PMU_TM_OVRHEAT_THRSH_OFFS);
	writel(reg, (INTER_REGS_BASE | PMU_THERMAL_MNGR_REG));
	writel(DOVE_OVERHEAT_DELAY, (INTER_REGS_BASE | PMU_TM_OVRHEAT_DLY_REG));

	/* Set the overcool threshole & delay */
	temp = DOVE_TSEN_TEMP2RAW(DOVE_OVERCOOL_TEMP);
	reg = readl(INTER_REGS_BASE | PMU_THERMAL_MNGR_REG);
	reg &= ~PMU_TM_COOL_THRSH_MASK;
	reg |= (temp << PMU_TM_COOL_THRSH_OFFS);
	writel(reg, (INTER_REGS_BASE | PMU_THERMAL_MNGR_REG));
	writel(DOVE_OVERCOOL_DELAY, (INTER_REGS_BASE | PMU_TM_COOLING_DLY_REG));

	return 0;
}

static int dovetemp_readTemp(void) {
	u32 reg;

	/* Verify that the temperature is valid */
	reg = readl(INTER_REGS_BASE | PMU_TEMP_DIOD_CTRL1_REG);
	if ((reg & PMU_TDC1_TEMP_VLID_MASK) == 0x0)
		return -EIO;

	/* Read the raw temperature */
	reg = readl(INTER_REGS_BASE | PMU_THERMAL_MNGR_REG);
	reg &= PMU_TM_CURR_TEMP_MASK;
	reg >>= PMU_TM_CURR_TEMP_OFFS;
	
	/* Convert the temperature to Celsuis */
	return DOVE_TSEN_RAW2TEMP(reg);
}


/*
 * Sysfs stuff
 */

static ssize_t show_name(struct device *dev, struct device_attribute
			  *devattr, char *buf) {
	return sprintf(buf, "%s\n", "dove-hwmon");
}

static ssize_t show_alarm(struct device *dev, struct device_attribute
			  *devattr, char *buf)
{
	int alarm = 0;
	u32 reg;

	reg = readl(INTER_REGS_BASE | PMU_INT_CAUSE_REG);
	if (reg & PMU_INT_OVRHEAT_MASK)
	{
		alarm = 1;
		writel ((reg & ~PMU_INT_OVRHEAT_MASK), (INTER_REGS_BASE | PMU_INT_CAUSE_REG));
	}
	else if (reg & PMU_INT_COOLING_MASK)
	{
		alarm = 2;
		writel ((reg & ~PMU_INT_COOLING_MASK), (INTER_REGS_BASE | PMU_INT_CAUSE_REG));
	}

	return sprintf(buf, "%d\n", alarm);
}

static ssize_t show_temp(struct device *dev,
			 struct device_attribute *devattr, char *buf) {
	int ret;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);

	if (attr->index == SHOW_TEMP)
		ret = sprintf(buf, "%d\n", dovetemp_readTemp());
	else if (attr->index == SHOW_MAX)
		ret = sprintf(buf, "%d\n", DOVE_OVERHEAT_TEMP);
	else if (attr->index == SHOW_MIN)
		ret = sprintf(buf, "%d\n", DOVE_OVERCOOL_TEMP);
	else
		ret = sprintf(buf, "%d\n", -1);

	return ret;
}

static SENSOR_DEVICE_ATTR(temp1_input, S_IRUGO, show_temp, NULL,
			  SHOW_TEMP);
static SENSOR_DEVICE_ATTR(temp1_max, S_IRUGO, show_temp, NULL,
			  SHOW_MAX);
static SENSOR_DEVICE_ATTR(temp1_min, S_IRUGO, show_temp, NULL,
			  SHOW_MIN);
static DEVICE_ATTR(temp1_crit_alarm, S_IRUGO, show_alarm, NULL);
static SENSOR_DEVICE_ATTR(name, S_IRUGO, show_name, NULL, SHOW_NAME);

static struct attribute *dovetemp_attributes[] = {
	&sensor_dev_attr_name.dev_attr.attr,
	&dev_attr_temp1_crit_alarm.attr,
	&sensor_dev_attr_temp1_input.dev_attr.attr,
	&sensor_dev_attr_temp1_max.dev_attr.attr,
	&sensor_dev_attr_temp1_min.dev_attr.attr,
	NULL
};

static const struct attribute_group dovetemp_group = {
	.attrs = dovetemp_attributes,
};

static int __devinit dovetemp_probe(struct platform_device *pdev)
{
	int err;

	err = dovetemp_initSensor();
	if (err)
		goto exit;

	err = sysfs_create_group(&pdev->dev.kobj, &dovetemp_group);
	if (err)
		goto exit;

	hwmon_dev = hwmon_device_register(&pdev->dev);
	if (IS_ERR(hwmon_dev)) {
		dev_err(&pdev->dev, "Class registration failed (%d)\n",
			err);
		goto exit;
	}

	printk(KERN_INFO "Dove hwmon thermal sensor initialized.\n");

	return 0;

exit:
	sysfs_remove_group(&pdev->dev.kobj, &dovetemp_group);
	return err;
}

static int __devexit dovetemp_remove(struct platform_device *pdev)
{
	struct dovetemp_data *data = platform_get_drvdata(pdev);

	hwmon_device_unregister(hwmon_dev);
	sysfs_remove_group(&pdev->dev.kobj, &dovetemp_group);
	platform_set_drvdata(pdev, NULL);
	kfree(data);
	return 0;
}

static struct platform_driver dovetemp_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "dove-temp",
	},
	.probe = dovetemp_probe,
	.remove = __devexit_p(dovetemp_remove),
};

static int __init dovetemp_init(void)
{
	return platform_driver_register(&dovetemp_driver);
}

static void __exit dovetemp_exit(void)
{
	platform_driver_unregister(&dovetemp_driver);
}

MODULE_AUTHOR("Marvell Semiconductors");
MODULE_DESCRIPTION("Marvell Dove SoC hwmon driver");
MODULE_LICENSE("GPL");

module_init(dovetemp_init)
module_exit(dovetemp_exit)
