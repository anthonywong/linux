#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/param.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include "AD9889_setup.h"
#include "AD9889_interrupt_handler.h"
struct i2c_client *adi9889_g_client, *adi9889_edid_g_client;

static const struct i2c_device_id adi9889_register_id[] = {
	{ "adi9889_i2c", 0 },
	{ "adi9889_edid_i2c", 0},
     { }
};
MODULE_DEVICE_TABLE(i2c, adi9889_i2c_id);
static struct i2c_client *adi9889_i2c_client;

static int adi9889_i2c_remove(struct i2c_client *client)
{
        adi9889_i2c_client = NULL;
        return 0;
}

static DEPLINT adi9889_read_byte(DEPLINT devaddr, DEPLINT regaddr, BYTE *buffer, DEPLINT count)
{
	int i;
#if 0 
	return adi9889_reg_get (regaddr, buffer, count);
#else
	for (i = 0 ; i < count ; i ++) {
			if (devaddr == 0x72) buffer[i]=i2c_smbus_read_byte_data(adi9889_g_client, regaddr + i);
			else  if (devaddr == 0x7E) buffer[i]=i2c_smbus_read_byte_data(adi9889_edid_g_client, regaddr + i);
			else printk (KERN_ERR "Got WRONG addr in %s (0x%x)\n",__FUNCTION__,devaddr);
//			printk ("READ devaddr 0x%x, 0x%x = 0x%x\n",devaddr, regaddr+i,buffer[i]);
	}
#endif
	return 0;
}
static DEPLINT adi9889_write_function(DEPLINT devaddr, DEPLINT regaddr, BYTE *buffer, DEPLINT count)
{
	int i;
	s32 rc;
	for (i = 0 ; i < count ; i ++) {
		if (devaddr == 0x72) rc = i2c_smbus_write_byte_data(adi9889_g_client, regaddr + i, buffer[i]);
		else if (devaddr == 0x7E) rc = i2c_smbus_write_byte_data(adi9889_edid_g_client, regaddr + i, buffer[i]);
		else printk (KERN_ERR "Got WRONG addr in %s (0x%x)\n",__FUNCTION__,devaddr);
//		printk ("WRITE devaddr 0x%x, 0x%x = 0x%x\n",devaddr, regaddr+i,buffer[i]);
	}
	return 0; 

}
void adi9889_error_callback(int errcode)
{
	printk ("ERROR at %s - error code %d\n",__FUNCTION__, errcode);
}


static void setup_audio_video(void)
{
	printk ("Got into %s\n",__FUNCTION__);
	/*
	 * 0 - input select - RGB 4:4:4
	 * 0 - input style - RGB
	 * 0 - ?
	 * 0 - ?
	 */
/*
	Red --> Blue
	Blue --> red
	Gree --> Green ?
*/
//	set_input_format(0, 0, 0, 0);
//	msleep(1000);
	/*
	 * 0 - video_timing - Not used in the driver !
	 * RGB - Input color space
	 * AD9889_0_255 - Color space range
	 * _16x9 - Aspect ratio
	 */
	set_input_format(0, 0, 0, 0);
	set_video_mode(AD9889_720p, RGB, AD9889_0_255, _16x9);
	set_audio_format(SPDIF);
	set_spdif_audio();
	av_mute_off();

}
struct task_struct *adi9889_thread_struct;

int adi9889_thread (void *data)
{
	int counter = 0;
	while (1) {
		int ret;
		counter ++;
		msleep(2000);
		ret = ad9889_interrupt_handler(setup_audio_video);
	}
	return 0;
}
static int adi9889_i2c_probe(struct i2c_client *client,
                        const struct i2c_device_id *id)
{
        int rc;
	printk ("Probing in %s, name %s, addr 0x%x\n",__FUNCTION__,client->name,client->addr);
        if (client->addr == 0x39) memcpy(&adi9889_g_client, &client, sizeof(client));
	else if (client->addr == 0x3f) {
		memcpy(&adi9889_edid_g_client, &client, sizeof(client));
		return 0;
	}
	else return -ENODEV;
        if (!i2c_check_functionality(client->adapter,
                                     I2C_FUNC_SMBUS_I2C_BLOCK)) {
                dev_err(&client->dev, "i2c bus does not support the adi9889\n");
                rc = -ENODEV;
                goto exit;
        }

        //HW reset

	initialize_ad9889_i2c(adi9889_read_byte, adi9889_write_function);
	initialize_error_callback (adi9889_error_callback);
	AD9889_reset();
	adi9889_early_setup();
	disable_hdcp();
	adi9889_thread_struct = kthread_run (adi9889_thread, client, "adi9889_int_thread");
	if (!adi9889_thread_struct) printk (KERN_ERR "Error creating kernel thread in %s\n",__FUNCTION__);
        return 0;
        
 exit:
        return rc;
}

static struct i2c_driver adi9889_driver = {
     .driver = {
          .name   = "adi9889_i2c",
          .owner  = THIS_MODULE,
     },
     .probe          = adi9889_i2c_probe,
     .remove         = adi9889_i2c_remove,
     .id_table       = adi9889_register_id,
};

static int __init dove_adi9889_init(void)
{
        int ret;
	printk ("Initializing %s\n",__FUNCTION__);
        if ((ret = i2c_add_driver(&adi9889_driver)) < 0)
        {
                return ret;             

        }
        return ret;
}

static void __exit dove_adi9889_exit(void)
{
        i2c_del_driver(&adi9889_driver);
}

module_init(dove_adi9889_init);
module_exit(dove_adi9889_exit);

MODULE_AUTHOR("rabeeh@marvell.com");
MODULE_DESCRIPTION("ADI9889 of Video plug - based on ANX7150 driver and code from ADI");
MODULE_LICENSE("GPL");

