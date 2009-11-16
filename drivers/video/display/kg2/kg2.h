/*
 * kg2.h - Linux Driver for Marvell 88DE2750 Digital Video Format Converter
 */

#ifndef __KG2_H__
#define __KG2_H__

static int kg2_i2c_probe(struct i2c_client * client, const struct i2c_device_id * id);
static int kg2_i2c_remove(struct i2c_client * client);
int kg2_run_script(const unsigned char * array, int count);
int kg2_i2c_write(unsigned char baseaddr, unsigned char subaddr, const unsigned char * data, unsigned short dataLen);

#endif
