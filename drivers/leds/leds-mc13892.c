/*
 * Copyright 2008-2009 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/pmic_light.h>
#include <linux/workqueue.h>

#define LED_NAME_LEN 16

struct mc13892_led {
	char			name[LED_NAME_LEN];
	enum lit_channel	channel;
	int			brightness;
	struct work_struct	work;
	struct led_classdev	cdev;
};

static void mc13892_led_work(struct work_struct *work)
{
	struct mc13892_led *led = container_of(work, struct mc13892_led, work);

	/* set current with medium value, in case current is too large */
	mc13892_bklit_set_current(led->channel, LIT_CURR_12);
	/* max duty cycle is 63, brightness needs to be divided by 4 */
	mc13892_bklit_set_dutycycle(led->channel, led->brightness / 4);
}


static void mc13892_led_set(struct led_classdev *led_cdev,
			    enum led_brightness value)
{
	struct mc13892_led *led = container_of(led_cdev,
					struct mc13892_led, cdev);
	led->brightness = value;
	schedule_work(&led->work);
}

static int mc13892_led_remove(struct platform_device *dev)
{
	struct mc13892_led *led = platform_get_drvdata(dev);

	led_classdev_unregister(&led->cdev);
	flush_work(&led->work);
	kfree(led);

	return 0;
}

static enum lit_channel mc13892_led_channel(int id)
{
	switch (id) {
	case 'r':
		return LIT_RED;
	case 'g':
		return LIT_GREEN;
	case 'b':
		return LIT_BLUE;
	default:
		return -1;
	}
}

static int mc13892_led_probe(struct platform_device *dev)
{
	struct mc13892_led *led;
	enum lit_channel chan;
	int ret;

	/* ensure we have space for the channel name and a NUL */
	if (strlen(dev->name) > LED_NAME_LEN - 2) {
		dev_err(&dev->dev, "led name is too long\n");
		return -EINVAL;
	}

	chan = mc13892_led_channel(dev->id);
	if (chan == -1) {
		dev_err(&dev->dev, "invalid LED id '%d'\n", dev->id);
		return -EINVAL;
	}

	led = kzalloc(sizeof(*led), GFP_KERNEL);
	if (!led) {
		dev_err(&dev->dev, "No memory for device\n");
		return -ENOMEM;
	}

	led->channel = chan;
	led->cdev.name = led->name;
	led->cdev.brightness_set = mc13892_led_set;
	INIT_WORK(&led->work, mc13892_led_work);
	snprintf(led->name, sizeof(led->name), "%s%c",
			dev->name, (char)dev->id);

	ret = led_classdev_register(&dev->dev, &led->cdev);
	if (ret < 0) {
		dev_err(&dev->dev, "led_classdev_register failed\n");
		goto err_free;
	}

	platform_set_drvdata(dev, led);

	return 0;
err_free:
	kfree(led);
	return ret;
}

#ifdef CONFIG_PM
static int mc13892_led_suspend(struct platform_device *dev, pm_message_t state)
{
	struct mc13892_led *led = platform_get_drvdata(dev);

	led_classdev_suspend(&led->cdev);
	return 0;
}

static int mc13892_led_resume(struct platform_device *dev)
{
	struct mc13892_led *led = platform_get_drvdata(dev);

	led_classdev_resume(&led->cdev);
	return 0;
}
#else
#define mc13892_led_suspend NULL
#define mc13892_led_resume NULL
#endif

static struct platform_driver mc13892_led_driver = {
	.probe = mc13892_led_probe,
	.remove = mc13892_led_remove,
	.suspend = mc13892_led_suspend,
	.resume = mc13892_led_resume,
	.driver = {
		   .name = "pmic_leds",
		   .owner = THIS_MODULE,
		   },
};

static int __init mc13892_led_init(void)
{
	return platform_driver_register(&mc13892_led_driver);
}

static void __exit mc13892_led_exit(void)
{
	platform_driver_unregister(&mc13892_led_driver);
}

module_init(mc13892_led_init);
module_exit(mc13892_led_exit);

MODULE_DESCRIPTION("Led driver for PMIC mc13892");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
