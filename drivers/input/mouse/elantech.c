/*
 * Elantech Touchpad driver
 *
 * Copyright (C) 2008 Tom Lin <tomlin690@msn.com>
 *	     (c) 2008 Chris Yang <chris.i611@gmail.com>
 *      add Elantech smart pad and touchpad
 *
 * Copyright (C) 2007 Arjan Opmeer <arjan@opmeer.net>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * Trademarks are the property of their respective owners.
 */

#include <linux/module.h>
#include <linux/input.h>
#include <linux/serio.h>
#include <linux/libps2.h>
#include <linux/delay.h>
#include <linux/i8042.h>
#include "psmouse.h"
//#include "synaptics.h"
#include "elantech.h"

#define ETP_RAW_MODE 		0x04
#define ETP_4_BYTE_MODE 	0x02


/* These values work with the touchpad on my laptop. Do they need adjustment? */
#define ETP_XMIN 		32
#define ETP_XMAX 		0x240
#define ETP_YMIN 		32
#define ETP_YMAX 		0x160

/*
 * Send a synaptics style special commands
 */
/*static int synaptics_send_cmd(struct psmouse *psmouse, unsigned char c, unsigned char *param)
{
	printk(KERN_DEBUG "+synaptics_send_cmd\n");
	if (psmouse_sliced_command(psmouse, c))
		return -1;
	if (ps2_command(&psmouse->ps2dev, param, PSMOUSE_CMD_GETINFO))
		return -1;
	printk(KERN_DEBUG "-synaptics_send_cmd\n");
	return 0;
}*/

/*
 * Send an Elantech style special command to write a register with a value
 */

static int elantech_ps2_command(struct psmouse *psmouse,unsigned char *para, unsigned char val)
{
	int err,i;
	err=0;

	for(i=0;i < 2 ;i++){
		if(ps2_command(&psmouse->ps2dev,para,val)==0){
			err=0;
			break;
		}
		err=-1;
	}

	return err;
}

static int elantech_write_reg(struct psmouse *psmouse, unsigned char reg, unsigned char val)
{


	//printk(KERN_DEBUG "+elantech_write_reg\n");
	if ((reg < 0x10) || (reg > 0x26))
		return -1;
	if ((reg > 0x11) && (reg < 0x20))
		return -1;
	//printk(KERN_DEBUG "reg=%x val=%x\n",reg,val);
	if (psmouse_sliced_command(psmouse, ELANTECH_COMMAND) ||
	    psmouse_sliced_command(psmouse, reg) ||
	    psmouse_sliced_command(psmouse, val) ||
	    ps2_command(&psmouse->ps2dev, NULL, PSMOUSE_CMD_SETSCALE11)) {
		return -1;
	}
	//printk(KERN_DEBUG "-elantech_write_reg\n");
	return 0;
}

static int elantech_write_reg_new(struct psmouse *psmouse, unsigned char reg, unsigned char val)
{


	//printk(KERN_DEBUG "+elantech_write_reg\n");
	if ((reg < 0x10) || (reg > 0x26))
		return -1;
	if ((reg > 0x11) && (reg < 0x20))
		return -1;

	//printk(KERN_DEBUG "reg=%x val=%x\n",reg,val);
	if(elantech_ps2_command(psmouse,NULL,0x00f8) != 0)
		return -1;

	if(elantech_ps2_command(psmouse,NULL,0x0011) != 0)
		return -1;

	if(elantech_ps2_command(psmouse,NULL,0x00f8) != 0)
		return -1;

	if(elantech_ps2_command(psmouse,NULL,reg) != 0)
		return -1;

	if(elantech_ps2_command(psmouse,NULL,0x00f8) != 0)
		return -1;

	if(elantech_ps2_command(psmouse,NULL,val) != 0)
		return -1;

	if(elantech_ps2_command(psmouse,NULL,0x00e6) != 0)
		return -1;
	//printk(KERN_DEBUG "-elantech_write_reg\n");

	return 0;
}


static int elantech_write_reg_debug(struct psmouse *psmouse, unsigned char reg, unsigned char val)
{


	//printk(KERN_DEBUG "+elantech_write_reg\n");


	//printk(KERN_DEBUG "reg=%x val=%x\n",reg,val);
	if(elantech_ps2_command(psmouse,NULL,0x00f8) != 0)
		return -1;

	if(reg==0x06){
		if(elantech_ps2_command(psmouse,NULL,0x0019) != 0)
		return -1;
	}else{
		if(elantech_ps2_command(psmouse,NULL,0x0011) != 0)
		return -1;
	}

	if(elantech_ps2_command(psmouse,NULL,0x00f8) != 0)
		return -1;

	if(elantech_ps2_command(psmouse,NULL,reg) != 0)
		return -1;

	if(elantech_ps2_command(psmouse,NULL,0x00f8) != 0)
		return -1;

	if(elantech_ps2_command(psmouse,NULL,val) != 0)
		return -1;

	if(elantech_ps2_command(psmouse,NULL,0x00e6) != 0)
		return -1;
	//printk(KERN_DEBUG "-elantech_write_reg\n");

	return 0;
}

static int elantech_read_reg_new(struct psmouse *psmouse, unsigned char reg, unsigned char val)
{
	int i;
	unsigned char	param[3];
	//printk(KERN_DEBUG "+elantech_read_reg_new\n");
	if ((reg < 0x10) || (reg > 0x26))
		return -1;
	if ((reg > 0x11) && (reg < 0x20))
		return -1;
	//printk(KERN_DEBUG "reg=%x val=%x\n",reg,val);

	if(elantech_ps2_command(psmouse,NULL,0x00f8) != 0)
		return -1;

	if(elantech_ps2_command(psmouse,NULL,0x0010) != 0)
		return -1;

	if(elantech_ps2_command(psmouse,NULL,0x00f8) != 0)
		return -1;

	if(elantech_ps2_command(psmouse,NULL,reg) != 0)
		return -1;



	for(i=0;i < 2 ;i++){
		if(ps2_command(&psmouse->ps2dev,param,PSMOUSE_CMD_GETINFO)==0){
		  break;
		}
	}
	printk(KERN_DEBUG "-elantech_read_reg_new param[0]=%x param[1]=%x param[2]=%x\n",param[0],param[1],param[2]);
	if(param[0]!=val)
		return -1;
	//printk(KERN_DEBUG "-elantech_read_reg_new\n");
	return 0;
}

static int elantech_read_reg(struct psmouse *psmouse, unsigned char reg, unsigned char val)
{
	unsigned char	param[3];
	//printk(KERN_DEBUG "+elantech_write_reg\n");
	if ((reg < 0x10) || (reg > 0x26))
		return -1;
	if ((reg > 0x11) && (reg < 0x20))
		return -1;
	//printk(KERN_DEBUG "reg=%x val=%x\n",reg,val);
	if (psmouse_sliced_command(psmouse, 0x10) ||
	    psmouse_sliced_command(psmouse, reg) ||
	    ps2_command(&psmouse->ps2dev, param, PSMOUSE_CMD_GETINFO)) {
		return -1;
	}
		printk(KERN_DEBUG "elantech_read_reg reg=%x val=%x param[0]=%x\n",reg,val,param[0]);
	if(param[0]!=val)
		return -1;
	//printk(KERN_DEBUG "-elantech_write_reg\n");
	return 0;
}

/*
 * Process byte stream from mouse and interpret complete data packages
 */
static psmouse_ret_t elantech_process_4byte_EF013(struct psmouse *psmouse)
{


	struct input_dev 		*dev = psmouse->dev;
	unsigned char 			*packet = psmouse->packet;
	unsigned int 			z,button,rock_switch;
	int				x1,y1,z1=60,x2,y2,x3,y3,x4,y4;
	static int			x1_old,y1_old,count;
	unsigned char			bit7,bit6,bit5,bit4,bit3,bit2,bit1,bit0;
	unsigned char			SA,B,C,D;
	int				C_B,C_C,C_D;




	x1=y1=x2=y2=x3=y3=x4=y4=0;
	//printk(KERN_DEBUG "+psmouse_ret_t elantech_process_byte\n");
	//printk(KERN_ERR "elantech.c: psmouse->fw_version %d\n",psmouse->fw_version);




		if (psmouse->pktcnt < 4)
			return PSMOUSE_GOOD_DATA;
//-----------------in check byte data--------------------------------------

		SA = packet[0];
		B  = packet[1];
		C  = packet[2];
		D  = packet[3];

		if(!(packet[0] & 0x08))			//chech SA byte
			goto shift_data;

		bit7 = (packet[1] & 0x80) >> 7;
		bit6 = (packet[1] & 0x40) >> 6;
		bit5 = (packet[1] & 0x20) >> 5;
		bit4 = (packet[1] & 0x10) >> 4;
		bit3 = (packet[1] & 0x08) >> 3;
		bit2 = (packet[1] & 0x04) >> 2;
		bit1 = (packet[1] & 0x02) >> 1;
		bit0 = (packet[1] & 0x01) ;
		C_B = (bit7 + bit6 + bit5 + bit4 + bit3 + bit2 + bit1 + bit0)%2;

		if((packet[0]&0x10) && (C_B == 1))	//chech B byte
			goto shift_data;
		else if(!(packet[0]&0x10) && (C_B == 0))
			goto shift_data;

		bit7 = (packet[2] & 0x80) >> 7;
		bit6 = (packet[2] & 0x40) >> 6;
		bit5 = (packet[2] & 0x20) >> 5;
		bit4 = (packet[2] & 0x10) >> 4;
		bit3 = (packet[2] & 0x08) >> 3;
		bit2 = (packet[2] & 0x04) >> 2;
		bit1 = (packet[2] & 0x02) >> 1;
		bit0 = (packet[2] & 0x01) ;
		C_C = (bit7 + bit6 + bit5 + bit4 + bit3 + bit2 + bit1 + bit0)%2;

		if((packet[0]&0x20) && (C_C == 1))	//chech C byte
			goto shift_data;
		else if(!(packet[0]&0x20) && (C_C == 0))
			goto shift_data;


		bit7 = (packet[3] & 0x80) >> 7;
		bit6 = (packet[3] & 0x40) >> 6;
		bit5 = (packet[3] & 0x20) >> 5;
		bit4 = (packet[3] & 0x10) >> 4;
		bit3 = (packet[3] & 0x08) >> 3;
		bit2 = (packet[3] & 0x04) >> 2;
		bit1 = (packet[3] & 0x02) >> 1;
		bit0 = (packet[3] & 0x01) ;
		C_D = (bit7 + bit6 + bit5 + bit4 + bit3 + bit2 + bit1 + bit0)%2;

		if((packet[0]&0x04) && (C_D == 1))	//chech D byte
			goto shift_data;
		else if(!(packet[0]&0x04) && (C_D == 0))
			goto shift_data;

//-----------------out check byte data--------------------------------------


		z = ((packet[0] & 0xC0) >> 6);
		if(z==0){
			input_report_abs(dev,ABS_PRESSURE,0);
			count=0;

		}

		button=packet[0] & 0x03;
                rock_switch = ((packet[1] & 0xF0) >> 4);

		x1 = ((packet[1] & 0x0c) << 6) | packet[2];
		y1 = 0x1FF - (((packet[1] & 0x03) << 8) | packet[3]);

	        //printk(KERN_DEBUG "+psmouse_ret_t elantech_process_byte psmouse->fw_version=%d\n",psmouse->fw_version);



		if((x1 != x1_old )||(y1 != y1_old)){
			x1_old=x1;
			y1_old=y1;

			if (z == 2) {
				count = 0;
				input_report_abs(dev, ABS_HAT0X,x1);
				input_report_abs(dev, ABS_HAT0Y,y1);
				input_report_abs(dev,ABS_PRESSURE,z1);

			//printk(KERN_DEBUG "Data= %x %x %x %x\n",packet[0],packet[1],packet[2],packet[3]);
			//printk(KERN_DEBUG "x1=%.2x y1=%.2x z1=%.2x\n",x1,y1,z1);
			//printk(KERN_DEBUG "x2=%.2x y2=%.2x count=%d \n",x2,y2,count);

			/*input_report_abs(dev, ABS_X,((packet[1] & 0x0c) << 6) | packet[2]);
			  input_report_abs(dev, ABS_Y,ETP_YMIN + ETP_YMAX - (((packet[1] & 0x03) << 8) | packet[3]));*/
			}
			if (z == 3){

                                input_report_abs(dev, ABS_HAT2X,x1);
                                input_report_abs(dev, ABS_HAT2Y,y1);
                                input_report_abs(dev,ABS_PRESSURE,z1);
				count = 0;


			}
			if (z == 1) {
				count++;
				if(count < 3){
				  input_report_abs(dev,ABS_PRESSURE,0);
				goto out_finger_one;
				}
				input_report_abs(dev, ABS_X,x1);
				input_report_abs(dev, ABS_Y,y1);
				input_report_abs(dev,ABS_PRESSURE,z1);
				count=4;

			 //printk(KERN_DEBUG "Data= %x %x %x %x\n",packet[0],packet[1],packet[2],packet[3]);
			 //printk(KERN_DEBUG "x1=%.2x y1=%.2x z1=%.2x\n",x1,y1,z1);
			 //printk(KERN_DEBUG "x2=%.2x y2=%.2x count=%d \n",x2,y2,count);


			}
                        x1=y1=x2=y2=x3=y3=x4=y4=0;

		}


		input_report_key(dev, BTN_TOUCH,                z >  0);
		input_report_key(dev, BTN_TOOL_FINGER,          z == 1);
		input_report_key(dev, BTN_TOOL_DOUBLETAP,       z == 2);
		input_report_key(dev, BTN_TOOL_TRIPLETAP,       z == 3);
out_finger_one:
		input_report_key(dev, BTN_LEFT,                 button == 1);
		input_report_key(dev, BTN_RIGHT,                button == 2);
		input_report_key(dev, BTN_MIDDLE,               button == 3);
		input_report_key(dev, BTN_FORWARD,              rock_switch == 1);
                input_report_key(dev, BTN_BACK,                 rock_switch == 2);
                input_report_key(dev, BTN_0,                    rock_switch == 4);
                input_report_key(dev, BTN_1,                    rock_switch == 8);





	input_sync(dev);
	//printk(KERN_DEBUG "-psmouse_ret_t elantech_process_byte\n");
	return PSMOUSE_FULL_PACKET;

shift_data:
	packet[0] = B;
	packet[1] = C;
	packet[2] = D;
	psmouse->pktcnt = psmouse->pktcnt-1;
	return PSMOUSE_GOOD_DATA;
}

static psmouse_ret_t elantech_process_4byte_EF019(struct psmouse *psmouse)
{


	struct input_dev 		*dev = psmouse->dev;
	unsigned char 			*packet = psmouse->packet;
	unsigned int 			z,button,rock_switch;
	int				x1,y1,z1=60,x2,y2,x3,y3,x4,y4;
	static int			x1_old,y1_old,count;
	unsigned char			bit7,bit6,bit5,bit4,bit3,bit2,bit1,bit0;
	unsigned char			SA,B,C,D;
	int				C_B,C_C,C_D;




	x1=y1=x2=y2=x3=y3=x4=y4=0;
	//printk(KERN_DEBUG "+psmouse_ret_t elantech_process_byte\n");
	//printk(KERN_ERR "elantech.c: psmouse->fw_version %d\n",psmouse->fw_version);




		if (psmouse->pktcnt < 4)
			return PSMOUSE_GOOD_DATA;
//-----------------in check byte data--------------------------------------

		SA = packet[0];
		B  = packet[1];
		C  = packet[2];
		D  = packet[3];

		if((packet[0] & 0x08))			//chech SA byte
			goto shift_data;

		bit7 = (packet[1] & 0x80) >> 7;
		bit6 = (packet[1] & 0x40) >> 6;
		bit5 = (packet[1] & 0x20) >> 5;
		bit4 = (packet[1] & 0x10) >> 4;
		bit3 = (packet[1] & 0x08) >> 3;
		bit2 = (packet[1] & 0x04) >> 2;
		bit1 = (packet[1] & 0x02) >> 1;
		bit0 = (packet[1] & 0x01) ;
		C_B = (bit7 + bit6 + bit5 + bit4 + bit3 + bit2 + bit1 + bit0)%2;

		if((packet[0]&0x10) && (C_B == 1))	//chech B byte
			goto shift_data;
		else if(!(packet[0]&0x10) && (C_B == 0))
			goto shift_data;

		bit7 = (packet[2] & 0x80) >> 7;
		bit6 = (packet[2] & 0x40) >> 6;
		bit5 = (packet[2] & 0x20) >> 5;
		bit4 = (packet[2] & 0x10) >> 4;
		bit3 = (packet[2] & 0x08) >> 3;
		bit2 = (packet[2] & 0x04) >> 2;
		bit1 = (packet[2] & 0x02) >> 1;
		bit0 = (packet[2] & 0x01) ;
		C_C = (bit7 + bit6 + bit5 + bit4 + bit3 + bit2 + bit1 + bit0)%2;

		if((packet[0]&0x20) && (C_C == 1))	//chech C byte
			goto shift_data;
		else if(!(packet[0]&0x20) && (C_C == 0))
			goto shift_data;


		bit7 = (packet[3] & 0x80) >> 7;
		bit6 = (packet[3] & 0x40) >> 6;
		bit5 = (packet[3] & 0x20) >> 5;
		bit4 = (packet[3] & 0x10) >> 4;
		bit3 = (packet[3] & 0x08) >> 3;
		bit2 = (packet[3] & 0x04) >> 2;
		bit1 = (packet[3] & 0x02) >> 1;
		bit0 = (packet[3] & 0x01) ;
		C_D = (bit7 + bit6 + bit5 + bit4 + bit3 + bit2 + bit1 + bit0)%2;

		if((packet[0]&0x04) && (C_D == 1))	//chech D byte
			goto shift_data;
		else if(!(packet[0]&0x04) && (C_D == 0))
			goto shift_data;

//-----------------out check byte data--------------------------------------


		z = ((packet[0] & 0xC0) >> 6);
		if(z==0){
			input_report_abs(dev,ABS_PRESSURE,0);
			count=0;

		}

		button=packet[0] & 0x03;
                rock_switch = ((packet[1] & 0xF0) >> 4);

		x1 = ((packet[1] & 0x0c) << 6) | packet[2];
		y1 = 0x1FF - (((packet[1] & 0x03) << 8) | packet[3]);

	        //printk(KERN_DEBUG "+psmouse_ret_t elantech_process_byte psmouse->fw_version=%d\n",psmouse->fw_version);



		if((x1 != x1_old )||(y1 != y1_old)){
			x1_old=x1;
			y1_old=y1;

			if (z == 2) {
				count = 0;
				input_report_abs(dev, ABS_HAT0X,x1);
				input_report_abs(dev, ABS_HAT0Y,y1);
				input_report_abs(dev,ABS_PRESSURE,z1);

			//printk(KERN_DEBUG "Data= %x %x %x %x\n",packet[0],packet[1],packet[2],packet[3]);
			//printk(KERN_DEBUG "x1=%.2x y1=%.2x z1=%.2x\n",x1,y1,z1);
			//printk(KERN_DEBUG "x2=%.2x y2=%.2x count=%d \n",x2,y2,count);

			/*input_report_abs(dev, ABS_X,((packet[1] & 0x0c) << 6) | packet[2]);
			  input_report_abs(dev, ABS_Y,ETP_YMIN + ETP_YMAX - (((packet[1] & 0x03) << 8) | packet[3]));*/
			}
			if (z == 3){

                                input_report_abs(dev, ABS_HAT2X,x1);
                                input_report_abs(dev, ABS_HAT2Y,y1);
                                input_report_abs(dev,ABS_PRESSURE,z1);
				count = 0;


			}
			if (z == 1) {
				count++;
				if(count < 3){
				  input_report_abs(dev,ABS_PRESSURE,0);
				goto out_finger_one;
				}
				input_report_abs(dev, ABS_X,x1);
				input_report_abs(dev, ABS_Y,y1);
				input_report_abs(dev,ABS_PRESSURE,z1);
				count=4;

			 //printk(KERN_DEBUG "Data= %x %x %x %x\n",packet[0],packet[1],packet[2],packet[3]);
			 //printk(KERN_DEBUG "x1=%.2x y1=%.2x z1=%.2x\n",x1,y1,z1);
			 //printk(KERN_DEBUG "x2=%.2x y2=%.2x count=%d \n",x2,y2,count);


			}
                        x1=y1=x2=y2=x3=y3=x4=y4=0;

		}


		input_report_key(dev, BTN_TOUCH,                z >  0);
		input_report_key(dev, BTN_TOOL_FINGER,          z == 1);
		input_report_key(dev, BTN_TOOL_DOUBLETAP,       z == 2);
		input_report_key(dev, BTN_TOOL_TRIPLETAP,       z == 3);
out_finger_one:
		input_report_key(dev, BTN_LEFT,                 button == 1);
		input_report_key(dev, BTN_RIGHT,                button == 2);
		input_report_key(dev, BTN_MIDDLE,               button == 3);
		input_report_key(dev, BTN_FORWARD,              rock_switch == 1);
                input_report_key(dev, BTN_BACK,                 rock_switch == 2);
                input_report_key(dev, BTN_0,                    rock_switch == 4);
                input_report_key(dev, BTN_1,                    rock_switch == 8);





	input_sync(dev);
	//printk(KERN_DEBUG "-psmouse_ret_t elantech_process_byte\n");
	return PSMOUSE_FULL_PACKET;

shift_data:
	packet[0] = B;
	packet[1] = C;
	packet[2] = D;
	psmouse->pktcnt = psmouse->pktcnt-1;
	return PSMOUSE_GOOD_DATA;
}

static psmouse_ret_t elantech_process_6byte_EF113(struct psmouse *psmouse)
{


	struct input_dev 		*dev = psmouse->dev;
	unsigned char 			*packet = psmouse->packet;
	unsigned char			SA1,A1,B1,SB1,C1,D1;
	unsigned int 			z,button;
	int				x1,y1,z1=60,x2,y2;


	x1=y1=x2=y2=0;
	//printk(KERN_DEBUG "+psmouse_ret_t elantech_process_byte\n");



	//printk(KERN_DEBUG "+psmouse_ret_t elantech_process_byte psmouse->fw_version=%d\n",psmouse->fw_version);
	//printk(KERN_DEBUG "Data= %.2x %.2x %.2x %.2x %.2x %.2x\n",packet[0],packet[1],packet[2],packet[3],packet[4],packet[5]);

//-----------------in check byte data--------------------------------------
		if (psmouse->pktcnt < 6)
			return PSMOUSE_GOOD_DATA;

		SA1= packet[0];
		A1 = packet[1];
		B1 = packet[2];
		SB1= packet[3];
		C1 = packet[4];
		D1 = packet[5];

	if( (((SA1 & 0x3C) != 0x3C) && ((SA1 & 0xC0) != 0x80))  || // check Byte 1
	    (((SA1 & 0x0C) != 0x0C) && ((SA1 & 0xC0) == 0x80))  || // check Byte 1
	    (((SA1 & 0xC0) != 0x80) && (( A1 & 0xF0) != 0x00))  || // check Byte 2
	    (((SB1 & 0x3E) != 0x38) && ((SA1 & 0xC0) != 0x80))  || // check Byte 4
	    (((SB1 & 0x0E) != 0x08) && ((SA1 & 0xC0) == 0x80))  || // check Byte 4
	    (((SA1 & 0xC0) != 0x80) && (( C1 & 0xF0) != 0x00))    ) // check Byte5
	{
		packet[0] = A1;
		packet[1] = B1;
		packet[2] = SB1;
		packet[3] = C1;
		packet[4] = D1;
		psmouse->pktcnt = psmouse->pktcnt - 1;
		return PSMOUSE_GOOD_DATA;
	}




//-----------------out check byte data--------------------------------------
		//printk(KERN_DEBUG "Data= %.2x %.2x %.2x %.2x %.2x %.2x\n",packet[0],packet[1],packet[2],packet[3],packet[4],packet[5]);


                z = ((packet[0] & 0xC0) >> 6);

                button = ((packet[3]&0x01 << 2) | (packet[0] & 0x03));

                if(z==0)
                        input_report_abs(dev,ABS_PRESSURE,0);

		if(z==1){

			x1=((packet[1] << 8) | packet[2]);
			y1= 0x2D0 - ((packet[4] << 8) | packet[5]);
			x2=(x1*420)/100 + 1400;
			y2=(y1*562)/100 + 1400;
			input_report_abs(dev, ABS_X,x2);
			input_report_abs(dev, ABS_Y,y2);
			input_report_abs(dev,ABS_PRESSURE,z1);

			//printk(KERN_DEBUG "Data= %.2x %.2x %.2x %.2x %.2x %.2x\n",packet[0],packet[1],packet[2],packet[3],packet[4],packet[5]);
			//printk(KERN_DEBUG "x1=%.2x y1=%.2x z1=%.2x\n",x1,y1,z1);
			//printk(KERN_DEBUG "x2=%.2x y2=%.2x\n",x2,y2);

		}

		if(z==2){
			//input_report_key(dev, BTN_C,z > 0);
			x1=((packet[0] & 0x10) << 4) | packet[1];
			y1=0xFF -(((packet[0] & 0x20) << 3) | packet[2]);
			x2=((packet[3] & 0x10) << 4) | packet[4];
			y2=0xFF -(((packet[3] & 0x20) << 3 )| packet[5]);

			input_report_abs(dev, ABS_HAT0X,x1);
			input_report_abs(dev, ABS_HAT0Y,y1);
			input_report_abs(dev, ABS_HAT1X,x2);
			input_report_abs(dev, ABS_HAT1Y,y2);
			input_report_abs(dev,ABS_PRESSURE,z1);

			//printk(KERN_DEBUG "Data= %.2x %.2x %.2x %.2x %.2x %.2x\n",packet[0],packet[1],packet[2],packet[3],packet[4],packet[5]);
			//printk(KERN_DEBUG "x1=%.2x y1=%.2x z1=%.2x\n",x1,y1,z1);
			//printk(KERN_DEBUG "x2=%.2x %d  y2=%.2x %d\n",x2,x2,y2,y2);

			//printk(KERN_DEBUG "x3=%.2x y3=%.2x z3=%.2x\n",x3,y3,z1);
			//printk(KERN_DEBUG "x4=%.2x %d  y4=%.2x %d\n",x4,x4,y4,y4);

		}
                if(z==3){
                        x1=((packet[1] << 8) | packet[2]);
                        y1= 0x7FF - ((packet[4] << 8) | packet[5]);
                        //x2=(x1*420)/100 + 1126;
                        //y2=(y1*562)/100 + 897;
                        input_report_abs(dev, ABS_HAT2X,x1);
                        input_report_abs(dev, ABS_HAT2Y,y1);
                        input_report_abs(dev,ABS_PRESSURE,z1);
                }



        input_report_key(dev, BTN_TOUCH, z > 0);
        input_report_key(dev, BTN_TOOL_FINGER, z == 1);
        input_report_key(dev, BTN_TOOL_DOUBLETAP, z == 2);
        input_report_key(dev, BTN_TOOL_TRIPLETAP, z == 3);



		input_report_key(dev, BTN_LEFT,                 button == 1);
		input_report_key(dev, BTN_RIGHT,                button == 2);
		input_report_key(dev, BTN_MIDDLE,               button == 3);
		input_report_key(dev, BTN_FORWARD,              button == 4);
		input_report_key(dev, BTN_BACK,                 button == 5);
		input_report_key(dev, BTN_0,                    button == 6);
		input_report_key(dev, BTN_1,                    button == 7);
		input_report_key(dev, BTN_2,                    button == 1);
		input_report_key(dev, BTN_3,                    button == 2);
		input_report_key(dev, BTN_4,                    button == 3);

	  packet[0]=0;
	  packet[1]=0;
	  packet[2]=0;
	  packet[3]=0;
	  packet[4]=0;
	  packet[5]=0;
	input_sync(dev);
	//printk(KERN_DEBUG "-psmouse_ret_t elantech_process_byte\n");
	return PSMOUSE_FULL_PACKET;


}


static psmouse_ret_t elantech_process_6byte_EF123(struct psmouse *psmouse)
{


	struct input_dev 		*dev = psmouse->dev;
	unsigned char 			*packet = psmouse->packet;
	unsigned char			SA1,A1,B1,SB1,C1,D1;
	static unsigned char		SA1_O,A1_O,B1_O,SB1_O,C1_O,D1_O;
	unsigned int 			fingers,button;
	int				x1,y1,z1,x2,y2,w1;
	int				MKY,VF;
	static int 		        Debug;


	x1=y1=x2=y2=0;
	//printk(KERN_DEBUG "+psmouse_ret_t elantech_process_byte\n");



	//printk(KERN_DEBUG "+psmouse_ret_t elantech_process_byte psmouse->fw_version=%d\n",psmouse->fw_version);
	//printk(KERN_DEBUG "Data= %.2x %.2x %.2x %.2x %.2x %.2x\n",packet[0],packet[1],packet[2],packet[3],packet[4],packet[5]);

//-----------------in check byte data--------------------------------------
	if (psmouse->pktcnt < 6)
		return PSMOUSE_GOOD_DATA;
		//printk(KERN_DEBUG "Data= %.2x %.2x %.2x %.2x %.2x %.2x\n",packet[0],packet[1],packet[2],packet[3],packet[4],packet[5]);

		SA1= packet[0];
		A1 = packet[1];
		B1 = packet[2];
		SB1= packet[3];
		C1 = packet[4];
		D1 = packet[5];

	if( ((SA1 & 0x0C) != 0x04) || // check Byte 1
	    ((SB1 & 0x0f) != 0x02) ) // check Byte  4
	{
		packet[0] = A1;
		packet[1] = B1;
		packet[2] = SB1;
		packet[3] = C1;
		packet[4] = D1;
		psmouse->pktcnt = psmouse->pktcnt - 1;
		return PSMOUSE_GOOD_DATA;
	}




//-----------------out check byte data--------------------------------------
		//printk(KERN_DEBUG "Data2= %.2x %.2x %.2x %.2x %.2x %.2x\n",packet[0],packet[1],packet[2],packet[3],packet[4],packet[5]);


	if( SA1 !=  SA1_O ||
	    A1  !=  A1_O  ||
	    B1  !=  B1_O  ||
	    SB1 !=  SB1_O ||
	    C1  !=  C1_O  ||
	    D1  !=  D1_O)
	{
                fingers = ((packet[0] & 0xC0) >> 6);
                button =  (packet[0] & 0x03);
		w1 = (((packet[0]&0x30)>>2)|((packet[3]&0x30)>>4)) & 0x0f;
		MKY = ((packet[3] & 0x80) >> 7);
		VF = ((packet[3] & 0x40) >> 6);


		//printk(KERN_DEBUG "w1= %d \n",w1);
                if(fingers==0)
                        input_report_abs(dev,ABS_PRESSURE,0);

		if(fingers==1){

			x1=(((packet[1] & 0x0f) << 8) | packet[2]);
			y1= 0x2F0 -(((packet[4] & 0x0f) << 8) | packet[5]);
			x2=(x1*420)/100 + 1400;
			y2=(y1*562)/100 + 1400;
			z1 = ((packet[1]&0xf0)|((packet[4]&0xf0)>>4));


			input_report_abs(dev, ABS_X,x2);
			input_report_abs(dev, ABS_Y,y2);
			input_report_abs(dev,ABS_PRESSURE,z1);


			//printk(KERN_DEBUG "Data= %.2x %.2x %.2x %.2x %.2x %.2x\n",packet[0],packet[1],packet[2],packet[3],packet[4],packet[5]);
			//printk(KERN_DEBUG "x1=%.2x y1=%.2x z1=%.2x\n",x1,y1,z1);
			//printk(KERN_DEBUG "x2=%.2x y2=%.2x\n",x2,y2);

		}

		if(fingers==2){
			//input_report_key(dev, BTN_C,z > 0);
			x1=((packet[0] & 0x10) << 4) | packet[1];
			y1=0xFF -(((packet[0] & 0x20) << 3) | packet[2]);
			x2=((packet[3] & 0x10) << 4) | packet[4];
			y2=0xFF -(((packet[3] & 0x20) << 3 )| packet[5]);
			z1=61;
			input_report_abs(dev, ABS_HAT0X,x1);
			input_report_abs(dev, ABS_HAT0Y,y1);
			input_report_abs(dev, ABS_HAT1X,x2);
			input_report_abs(dev, ABS_HAT1Y,y2);
			input_report_abs(dev,ABS_PRESSURE,z1);

			//printk(KERN_DEBUG "Data= %.2x %.2x %.2x %.2x %.2x %.2x\n",packet[0],packet[1],packet[2],packet[3],packet[4],packet[5]);
			//printk(KERN_DEBUG "x1=%.2x y1=%.2x z1=%.2x\n",x1,y1,z1);
			//printk(KERN_DEBUG "x2=%.2x %d  y2=%.2x %d\n",x2,x2,y2,y2);

			//printk(KERN_DEBUG "x3=%.2x y3=%.2x z3=%.2x\n",x3,y3,z1);
			//printk(KERN_DEBUG "x4=%.2x %d  y4=%.2x %d\n",x4,x4,y4,y4);

		}
                if(fingers==3){
                        x1=(((packet[1] & 0x0f) << 8) | packet[2]);
                        y1= 0x7FF - (((packet[4] & 0x0f) << 8) | packet[5]);
			z1 = ((packet[1]&0xf0)|((packet[4]&0xf0)>>4));

                        //x2=(x1*420)/100 + 1126;
                        //y2=(y1*562)/100 + 897;
                        input_report_abs(dev, ABS_HAT2X,x1);
                        input_report_abs(dev, ABS_HAT2Y,y1);
                        input_report_abs(dev,ABS_PRESSURE,z1);
                }



		input_report_abs(dev,ABS_TOOL_WIDTH,w1);
		input_report_key(dev, BTN_TOUCH, fingers > 0);
		input_report_key(dev, BTN_TOOL_FINGER, fingers == 1);
		input_report_key(dev, BTN_TOOL_DOUBLETAP, fingers == 2);
		input_report_key(dev, BTN_TOOL_TRIPLETAP, fingers == 3);


		input_report_key(dev, BTN_LEFT,                 button == 1);
		input_report_key(dev, BTN_RIGHT,                button == 2);
		input_report_key(dev, BTN_MIDDLE,               button == 3);
		input_report_key(dev, BTN_FORWARD,              button == 4);
		input_report_key(dev, BTN_BACK,                 button == 5);
		input_report_key(dev, BTN_0,                    button == 6);
		input_report_key(dev, BTN_1,                    button == 7);
		input_report_key(dev, BTN_2,                    button == 1);
		input_report_key(dev, BTN_3,                    button == 2);
		input_report_key(dev, BTN_4,                    button == 3);
		input_report_key(dev, BTN_5,                    MKY == 1);
		input_report_key(dev, BTN_6,                    VF == 1);

	        SA1_O = SA1;
		A1_O  = A1;
	        B1_O  = B1;
	        SB1_O = SB1;
		C1_O  = C1;
	        D1_O  = D1;
	}
	input_sync(dev);
	//printk(KERN_DEBUG "-psmouse_ret_t elantech_process_byte\n");
	return PSMOUSE_FULL_PACKET;


}

static int EF023_DEBUG_SOLUTION(int fingers,int button,int w)
{
		static int 	can_bug=0,debug_condition_1=0,count_report=0,debug_condition_3=0,debug_condition_5=0;
		static int	solution=0;



		//printk(KERN_DEBUG "\n solution=%d  fingers=%d button=%d\n",solution,fingers,button);
		//printk(KERN_DEBUG "1 count_report=%d \n", count_report);


		if(solution==3)
			return 0;

		if(fingers >=2 || (fingers == 1 && button > 0))
			can_bug=1;


		if(can_bug && button > 0)
			debug_condition_1=1;

		if(fingers <= 1 && can_bug && button==0)
			can_bug=0;

		if(debug_condition_1&& fingers ==0 && button > 0){
			count_report++;
			//printk(KERN_DEBUG "2 count_report=%d \n", count_report);
			if(count_report==4){
				debug_condition_1=0;
				EF_023_DEBUG=2;
				solution=2;
				return 1;

			}

		}
		else if(debug_condition_1  && fingers == 1 && button > 0)
		{
			count_report=0;
		}
		else if(debug_condition_1 &&  count_report > 0 &&  count_report < 4  && button == 0)
		{
			EF_023_DEBUG=1;
			solution=1;
			debug_condition_1=0;
			count_report=0;
			return 1;
		}
		else if(fingers ==0 && button == 0)
		{
			count_report=0;
			debug_condition_1=0;

		}



		//if(debug_condition_5 != 0)
			//printk(KERN_DEBUG "debug_condition_5=%d  fingers=%d button=%d\n",debug_condition_5,fingers,button);


		if(debug_condition_5==1 &&fingers ==1 && button ==0){
			EF_023_DEBUG=4;
			debug_condition_5=2;
			return 1;
		}else if(debug_condition_5==2){
		      if(fingers ==0 && button > 0){
			 EF_023_DEBUG=5;
			 debug_condition_5=3;
		      }else{
			EF_023_DEBUG=5;
			debug_condition_5=5;
		      }
			return 1;
		}else if(debug_condition_5==3 && fingers < 2 && button==0){
			debug_condition_5=4;
		}else if(debug_condition_5 ==4 && fingers ==1 && button==0 && w !=4){
			solution=3;
		}else if(debug_condition_5==5){
			EF_023_DEBUG=1;
			debug_condition_5=0;
			return 1;
		}else if (debug_condition_5 !=0 ){

			if((w ==4 && fingers==1)||(fingers > 1 && debug_condition_5 >= 3))
			{
				EF_023_DEBUG=1;
				debug_condition_5=0;
				return 1;
			}
			debug_condition_5=0;
		}else if(debug_condition_5==0 && button ==0){
			debug_condition_5=1;
		}



		if(fingers ==1 && button ==0){
			debug_condition_3=1;
		}else if(debug_condition_3==1 && fingers ==0 && button ==1){
			solution = 3;
		}else {
			debug_condition_3=0;
		}


		return 0;
}


static psmouse_ret_t elantech_process_6byte_EF023(struct psmouse *psmouse)
{


	struct input_dev 		*dev = psmouse->dev;
	unsigned char 			*packet = psmouse->packet;
	unsigned char			SA1,A1,B1,SB1,C1,D1;
	static unsigned char		SA1_O,A1_O,B1_O,SB1_O,C1_O,D1_O;
	unsigned int 			fingers,button;
	int				x1,y1,z1,x2,y2,w1;
	static int 		       Debug;


	x1=y1=x2=y2=0;
	//printk(KERN_DEBUG "+psmouse_ret_t elantech_process_byte\n");



	//printk(KERN_DEBUG "+psmouse_ret_t elantech_process_byte psmouse->fw_version=%d\n",psmouse->fw_version);
	//printk(KERN_DEBUG "Data= %.2x %.2x %.2x %.2x %.2x %.2x\n",packet[0],packet[1],packet[2],packet[3],packet[4],packet[5]);

//-----------------in check byte data--------------------------------------
	if (psmouse->pktcnt < 6)
		return PSMOUSE_GOOD_DATA;
		//printk(KERN_DEBUG "Data= %.2x %.2x %.2x %.2x %.2x %.2x\n",packet[0],packet[1],packet[2],packet[3],packet[4],packet[5]);

		SA1= packet[0];
		A1 = packet[1];
		B1 = packet[2];
		SB1= packet[3];
		C1 = packet[4];
		D1 = packet[5];

	if( ((SA1 & 0x0C) != 0x04) || // check Byte 1
	    ((SB1 & 0x0f) != 0x02) ) // check Byte  4
	{
		packet[0] = A1;
		packet[1] = B1;
		packet[2] = SB1;
		packet[3] = C1;
		packet[4] = D1;
		psmouse->pktcnt = psmouse->pktcnt - 1;
		return PSMOUSE_GOOD_DATA;
	}




//-----------------out check byte data--------------------------------------
		//printk(KERN_DEBUG "Data2= %.2x %.2x %.2x %.2x %.2x %.2x\n",packet[0],packet[1],packet[2],packet[3],packet[4],packet[5]);
	fingers = ((packet[0] & 0xC0) >> 6);
        button = ((packet[3]&0x01 << 2) | (packet[0] & 0x03));
	w1 = (((packet[0]&0x30)>>2)|((packet[3]&0x30)>>4)) & 0x0f;
	Debug = EF023_DEBUG_SOLUTION(fingers,button,w1);
	if(Debug){
		psmouse->resetafter = psmouse->out_of_sync+1;
		return PSMOUSE_BAD_DATA;
	}

	if( SA1 !=  SA1_O ||
	    A1  !=  A1_O  ||
	    B1  !=  B1_O  ||
	    SB1 !=  SB1_O ||
	    C1  !=  C1_O  ||
	    D1  !=  D1_O)
	{
                fingers = ((packet[0] & 0xC0) >> 6);
                button = ((packet[3]&0x01 << 2) | (packet[0] & 0x03));
		w1 = (((packet[0]&0x30)>>2)|((packet[3]&0x30)>>4)) & 0x0f;
	#if 0
		Debug = EF023_DEBUG_SOLUTION(fingers,button,w1);
		if(Debug){
			psmouse->resetafter = psmouse->out_of_sync+1;
			return PSMOUSE_BAD_DATA;
		}
	#endif


		//printk(KERN_DEBUG "w1= %d \n",w1);
                if(fingers==0)
                        input_report_abs(dev,ABS_PRESSURE,0);

		if(fingers==1){

			x1=(((packet[1] & 0x0f) << 8) | packet[2]);
			y1= 0x2F0 -(((packet[4] & 0x0f) << 8) | packet[5]);
			x2=(x1*420)/100 + 1400;
			y2=(y1*562)/100 + 1400;
			z1 = ((packet[1]&0xf0)|((packet[4]&0xf0)>>4));


			input_report_abs(dev, ABS_X,x2);
			input_report_abs(dev, ABS_Y,y2);
			input_report_abs(dev,ABS_PRESSURE,z1);


			//printk(KERN_DEBUG "Data= %.2x %.2x %.2x %.2x %.2x %.2x\n",packet[0],packet[1],packet[2],packet[3],packet[4],packet[5]);
			//printk(KERN_DEBUG "x1=%.2x y1=%.2x z1=%.2x\n",x1,y1,z1);
			//printk(KERN_DEBUG "x2=%.2x y2=%.2x\n",x2,y2);

		}

		if(fingers==2){
			//input_report_key(dev, BTN_C,z > 0);
			x1=((packet[0] & 0x10) << 4) | packet[1];
			y1=0xFF -(((packet[0] & 0x20) << 3) | packet[2]);
			x2=((packet[3] & 0x10) << 4) | packet[4];
			y2=0xFF -(((packet[3] & 0x20) << 3 )| packet[5]);
			z1=61;
			input_report_abs(dev, ABS_HAT0X,x1);
			input_report_abs(dev, ABS_HAT0Y,y1);
			input_report_abs(dev, ABS_HAT1X,x2);
			input_report_abs(dev, ABS_HAT1Y,y2);
			input_report_abs(dev,ABS_PRESSURE,z1);

			//printk(KERN_DEBUG "Data= %.2x %.2x %.2x %.2x %.2x %.2x\n",packet[0],packet[1],packet[2],packet[3],packet[4],packet[5]);
			//printk(KERN_DEBUG "x1=%.2x y1=%.2x z1=%.2x\n",x1,y1,z1);
			//printk(KERN_DEBUG "x2=%.2x %d  y2=%.2x %d\n",x2,x2,y2,y2);

			//printk(KERN_DEBUG "x3=%.2x y3=%.2x z3=%.2x\n",x3,y3,z1);
			//printk(KERN_DEBUG "x4=%.2x %d  y4=%.2x %d\n",x4,x4,y4,y4);

		}
                if(fingers==3){
                        x1=(((packet[1] & 0x0f) << 8) | packet[2]);
                        y1= 0x7FF - (((packet[4] & 0x0f) << 8) | packet[5]);
			z1 = ((packet[1]&0xf0)|((packet[4]&0xf0)>>4));

                        //x2=(x1*420)/100 + 1126;
                        //y2=(y1*562)/100 + 897;
                        input_report_abs(dev, ABS_HAT2X,x1);
                        input_report_abs(dev, ABS_HAT2Y,y1);
                        input_report_abs(dev,ABS_PRESSURE,z1);
                }



		input_report_abs(dev,ABS_TOOL_WIDTH,w1);
		input_report_key(dev, BTN_TOUCH, fingers > 0);
		input_report_key(dev, BTN_TOOL_FINGER, fingers == 1);
		input_report_key(dev, BTN_TOOL_DOUBLETAP, fingers == 2);
		input_report_key(dev, BTN_TOOL_TRIPLETAP, fingers == 3);


		input_report_key(dev, BTN_LEFT,                 button == 1);
		input_report_key(dev, BTN_RIGHT,                button == 2);
		input_report_key(dev, BTN_MIDDLE,               button == 3);
		input_report_key(dev, BTN_FORWARD,              button == 4);
		input_report_key(dev, BTN_BACK,                 button == 5);
		input_report_key(dev, BTN_0,                    button == 6);
		input_report_key(dev, BTN_1,                    button == 7);
	        input_report_key(dev, BTN_2,                    button == 1);
		input_report_key(dev, BTN_3,                    button == 2);
		input_report_key(dev, BTN_4,                    button == 3);

	        SA1_O = SA1;
		A1_O  = A1;
	        B1_O  = B1;
	        SB1_O = SB1;
		C1_O  = C1;
	        D1_O  = D1;
	}
	input_sync(dev);
	//printk(KERN_DEBUG "-psmouse_ret_t elantech_process_byte\n");
	return PSMOUSE_FULL_PACKET;


}


static psmouse_ret_t elantech_process_6byte_EF215(struct psmouse *psmouse)
{


	struct input_dev 		*dev = psmouse->dev;
	unsigned char 			*packet = psmouse->packet;
	unsigned char			SA1,A1,B1,SB1,C1,D1;
	static unsigned char		SA1_O,A1_O,B1_O,SB1_O,C1_O,D1_O;
	unsigned int 			fingers,button;
	int				x1,y1,z1,x2,y2,w1;


	x1=y1=x2=y2=0;
	//printk(KERN_DEBUG "+psmouse_ret_t elantech_process_byte\n");



	//printk(KERN_DEBUG "+psmouse_ret_t elantech_process_byte psmouse->fw_version=%d\n",psmouse->fw_version);
	//printk(KERN_DEBUG "Data= %.2x %.2x %.2x %.2x %.2x %.2x\n",packet[0],packet[1],packet[2],packet[3],packet[4],packet[5]);

//-----------------in check byte data--------------------------------------
		if (psmouse->pktcnt < 6)
			return PSMOUSE_GOOD_DATA;
		//printk(KERN_DEBUG "Data= %.2x %.2x %.2x %.2x %.2x %.2x\n",packet[0],packet[1],packet[2],packet[3],packet[4],packet[5]);

		SA1= packet[0];
		A1 = packet[1];
		B1 = packet[2];
		SB1= packet[3];
		C1 = packet[4];
		D1 = packet[5];

	if( ((SA1 & 0x0C) != 0x04) || // check Byte 1
	    ((SB1 & 0x0f) != 0x02) ) // check Byte  4
	{
		packet[0] = A1;
		packet[1] = B1;
		packet[2] = SB1;
		packet[3] = C1;
		packet[4] = D1;
		psmouse->pktcnt = psmouse->pktcnt - 1;
		return PSMOUSE_GOOD_DATA;
	}




//-----------------out check byte data--------------------------------------
		//printk(KERN_DEBUG "Data2= %.2x %.2x %.2x %.2x %.2x %.2x\n",packet[0],packet[1],packet[2],packet[3],packet[4],packet[5]);

	if( SA1 !=  SA1_O ||
	    A1  !=  A1_O  ||
	    B1  !=  B1_O  ||
	    SB1 !=  SB1_O ||
	    C1  !=  C1_O  ||
	    D1  !=  D1_O)
	{
                fingers = ((packet[0] & 0xC0) >> 6);

                button = ((packet[3]&0x01 << 2) | (packet[0] & 0x03));
		w1 = (((packet[0]&0x30)>>2)|((packet[3]&0x30)>>4)) & 0x0f;
		//printk(KERN_DEBUG "w1= %d \n",w1);
                if(fingers==0)
                        input_report_abs(dev,ABS_PRESSURE,0);

		if(fingers==1){

			x1=(((packet[1] & 0x0f) << 8) | packet[2]);
			y1= 0x2F0 -(((packet[4] & 0x0f) << 8) | packet[5]);
			x2=(x1*420)/100 + 1400;
			y2=(y1*562)/100 + 1400;
			z1 = ((packet[1]&0xf0)|((packet[4]&0xf0)>>4));


			input_report_abs(dev, ABS_X,x2);
			input_report_abs(dev, ABS_Y,y2);
			input_report_abs(dev,ABS_PRESSURE,z1);


			//printk(KERN_DEBUG "Data= %.2x %.2x %.2x %.2x %.2x %.2x\n",packet[0],packet[1],packet[2],packet[3],packet[4],packet[5]);
			//printk(KERN_DEBUG "x1=%.2x y1=%.2x z1=%.2x\n",x1,y1,z1);
			//printk(KERN_DEBUG "x2=%.2x y2=%.2x\n",x2,y2);

		}

		if(fingers==2){
			//input_report_key(dev, BTN_C,z > 0);
			x1=((packet[0] & 0x10) << 4) | packet[1];
			y1=0xFF -(((packet[0] & 0x20) << 3) | packet[2]);
			x2=((packet[3] & 0x10) << 4) | packet[4];
			y2=0xFF -(((packet[3] & 0x20) << 3 )| packet[5]);
			z1=61;
			input_report_abs(dev, ABS_HAT0X,x1);
			input_report_abs(dev, ABS_HAT0Y,y1);
			input_report_abs(dev, ABS_HAT1X,x2);
			input_report_abs(dev, ABS_HAT1Y,y2);
			input_report_abs(dev,ABS_PRESSURE,z1);

			//printk(KERN_DEBUG "Data= %.2x %.2x %.2x %.2x %.2x %.2x\n",packet[0],packet[1],packet[2],packet[3],packet[4],packet[5]);
			//printk(KERN_DEBUG "x1=%.2x y1=%.2x z1=%.2x\n",x1,y1,z1);
			//printk(KERN_DEBUG "x2=%.2x %d  y2=%.2x %d\n",x2,x2,y2,y2);

			//printk(KERN_DEBUG "x3=%.2x y3=%.2x z3=%.2x\n",x3,y3,z1);
			//printk(KERN_DEBUG "x4=%.2x %d  y4=%.2x %d\n",x4,x4,y4,y4);

		}
                if(fingers==3){
                        x1=(((packet[1] & 0x0f) << 8) | packet[2]);
                        y1= 0x7FF - (((packet[4] & 0x0f) << 8) | packet[5]);
			z1 = ((packet[1]&0xf0)|((packet[4]&0xf0)>>4));

                        //x2=(x1*420)/100 + 1126;
                        //y2=(y1*562)/100 + 897;
                        input_report_abs(dev, ABS_HAT2X,x1);
                        input_report_abs(dev, ABS_HAT2Y,y1);
                        input_report_abs(dev,ABS_PRESSURE,z1);
                }



		input_report_abs(dev,ABS_TOOL_WIDTH,w1);
		input_report_key(dev, BTN_TOUCH, fingers > 0);
		input_report_key(dev, BTN_TOOL_FINGER, fingers == 1);
		input_report_key(dev, BTN_TOOL_DOUBLETAP, fingers == 2);
		input_report_key(dev, BTN_TOOL_TRIPLETAP, fingers == 3);



		input_report_key(dev, BTN_LEFT,                 button == 1);
		input_report_key(dev, BTN_RIGHT,                button == 2);
		input_report_key(dev, BTN_MIDDLE,               button == 3);
		input_report_key(dev, BTN_FORWARD,              button == 4);
		input_report_key(dev, BTN_BACK,                 button == 5);
		input_report_key(dev, BTN_0,                    button == 6);
		input_report_key(dev, BTN_1,                    button == 7);
		input_report_key(dev, BTN_2,                    button == 1);
		input_report_key(dev, BTN_3,                    button == 2);
		input_report_key(dev, BTN_4,                    button == 3);

	        SA1_O = SA1;
		A1_O  = A1;
	        B1_O  = B1;
	        SB1_O = SB1;
		C1_O  = C1;
	        D1_O  = D1;
	}
	input_sync(dev);
	//printk(KERN_DEBUG "-psmouse_ret_t elantech_process_byte\n");
	return PSMOUSE_FULL_PACKET;


}

static psmouse_ret_t elantech_process_6byte_EF051(struct psmouse *psmouse)
{


	struct input_dev 		*dev = psmouse->dev;
	unsigned char 			*packet = psmouse->packet;
	unsigned char			SA1,A1,B1,SB1,C1,D1;
	static unsigned char		SA1_O,A1_O,B1_O,SB1_O,C1_O,D1_O;
	unsigned int 			fingers,button;
	int				x1,y1,z1,x2,y2,w1;


	x1=y1=x2=y2=0;
	//printk(KERN_DEBUG "+psmouse_ret_t elantech_process_byte\n");



	//printk(KERN_DEBUG "+psmouse_ret_t elantech_process_byte psmouse->fw_version=%d\n",psmouse->fw_version);
	//printk(KERN_DEBUG "Data= %.2x %.2x %.2x %.2x %.2x %.2x\n",packet[0],packet[1],packet[2],packet[3],packet[4],packet[5]);

//-----------------in check byte data--------------------------------------
		if (psmouse->pktcnt < 6)
			return PSMOUSE_GOOD_DATA;
		//printk(KERN_DEBUG "Data= %.2x %.2x %.2x %.2x %.2x %.2x\n",packet[0],packet[1],packet[2],packet[3],packet[4],packet[5]);

		SA1= packet[0];
		A1 = packet[1];
		B1 = packet[2];
		SB1= packet[3];
		C1 = packet[4];
		D1 = packet[5];

	if( ((SA1 & 0x0C) != 0x04) || // check Byte 1
	    ((SB1 & 0x0f) != 0x02) ) // check Byte  4
	{
		packet[0] = A1;
		packet[1] = B1;
		packet[2] = SB1;
		packet[3] = C1;
		packet[4] = D1;
		psmouse->pktcnt = psmouse->pktcnt - 1;
		return PSMOUSE_GOOD_DATA;
	}




//-----------------out check byte data--------------------------------------
		//printk(KERN_DEBUG "Data2= %.2x %.2x %.2x %.2x %.2x %.2x\n",packet[0],packet[1],packet[2],packet[3],packet[4],packet[5]);

	if( SA1 !=  SA1_O ||
	    A1  !=  A1_O  ||
	    B1  !=  B1_O  ||
	    SB1 !=  SB1_O ||
	    C1  !=  C1_O  ||
	    D1  !=  D1_O)
	{
                fingers = ((packet[0] & 0xC0) >> 6);

                button = ((packet[3]&0x01 << 2) | (packet[0] & 0x03));
		w1 = (((packet[0]&0x30)>>2)|((packet[3]&0x30)>>4)) & 0x0f;
		//printk(KERN_DEBUG "w1= %d \n",w1);
                if(fingers==0)
                        input_report_abs(dev,ABS_PRESSURE,0);

		if(fingers==1){

			x1=(((packet[1] & 0x0f) << 8) | packet[2]);
			y1= 0x2F0 -(((packet[4] & 0x0f) << 8) | packet[5]);
			x2=(x1*420)/100 + 1400;
			y2=(y1*562)/100 + 1400;
			z1 = ((packet[1]&0xf0)|((packet[4]&0xf0)>>4));


			input_report_abs(dev, ABS_X,x2);
			input_report_abs(dev, ABS_Y,y2);
			input_report_abs(dev,ABS_PRESSURE,z1);


			//printk(KERN_DEBUG "Data= %.2x %.2x %.2x %.2x %.2x %.2x\n",packet[0],packet[1],packet[2],packet[3],packet[4],packet[5]);
			//printk(KERN_DEBUG "x1=%.2x y1=%.2x z1=%.2x\n",x1,y1,z1);
			//printk(KERN_DEBUG "x2=%.2x y2=%.2x\n",x2,y2);

		}

		if(fingers==2){
			//input_report_key(dev, BTN_C,z > 0);
			x1=((packet[0] & 0x10) << 4) | packet[1];
			y1=0xFF -(((packet[0] & 0x20) << 3) | packet[2]);
			x2=((packet[3] & 0x10) << 4) | packet[4];
			y2=0xFF -(((packet[3] & 0x20) << 3 )| packet[5]);
			z1=61;
			input_report_abs(dev, ABS_HAT0X,x1);
			input_report_abs(dev, ABS_HAT0Y,y1);
			input_report_abs(dev, ABS_HAT1X,x2);
			input_report_abs(dev, ABS_HAT1Y,y2);
			input_report_abs(dev,ABS_PRESSURE,z1);

			//printk(KERN_DEBUG "Data= %.2x %.2x %.2x %.2x %.2x %.2x\n",packet[0],packet[1],packet[2],packet[3],packet[4],packet[5]);
			//printk(KERN_DEBUG "x1=%.2x y1=%.2x z1=%.2x\n",x1,y1,z1);
			//printk(KERN_DEBUG "x2=%.2x %d  y2=%.2x %d\n",x2,x2,y2,y2);

			//printk(KERN_DEBUG "x3=%.2x y3=%.2x z3=%.2x\n",x3,y3,z1);
			//printk(KERN_DEBUG "x4=%.2x %d  y4=%.2x %d\n",x4,x4,y4,y4);

		}
                if(fingers==3){
                        x1=(((packet[1] & 0x0f) << 8) | packet[2]);
                        y1= 0x7FF - (((packet[4] & 0x0f) << 8) | packet[5]);
			z1 = ((packet[1]&0xf0)|((packet[4]&0xf0)>>4));

                        //x2=(x1*420)/100 + 1126;
                        //y2=(y1*562)/100 + 897;
                        input_report_abs(dev, ABS_HAT2X,x1);
                        input_report_abs(dev, ABS_HAT2Y,y1);
                        input_report_abs(dev,ABS_PRESSURE,z1);
                }



		input_report_abs(dev,ABS_TOOL_WIDTH,w1);
		input_report_key(dev, BTN_TOUCH, fingers > 0);
		input_report_key(dev, BTN_TOOL_FINGER, fingers == 1);
		input_report_key(dev, BTN_TOOL_DOUBLETAP, fingers == 2);
		input_report_key(dev, BTN_TOOL_TRIPLETAP, fingers == 3);



		input_report_key(dev, BTN_LEFT,                 button == 1);
		input_report_key(dev, BTN_RIGHT,                button == 2);
		input_report_key(dev, BTN_MIDDLE,               button == 3);
		input_report_key(dev, BTN_FORWARD,              button == 4);
		input_report_key(dev, BTN_BACK,                 button == 5);
		input_report_key(dev, BTN_0,                    button == 6);
		input_report_key(dev, BTN_1,                    button == 7);
		input_report_key(dev, BTN_2,                    button == 1);
		input_report_key(dev, BTN_3,                    button == 2);
		input_report_key(dev, BTN_4,                    button == 3);

	        SA1_O = SA1;
		A1_O  = A1;
	        B1_O  = B1;
	        SB1_O = SB1;
		C1_O  = C1;
	        D1_O  = D1;
	}
	input_sync(dev);
	//printk(KERN_DEBUG "-psmouse_ret_t elantech_process_byte\n");
	return PSMOUSE_FULL_PACKET;


}

static psmouse_ret_t elantech_process_6byte_Scroll_EF051(struct psmouse *psmouse)
{


	struct input_dev 		*dev = psmouse->dev;
	unsigned char 			*packet = psmouse->packet;
	unsigned char			SA1,A1,B1,SB1,C1,D1;
	static unsigned char		SA1_O,A1_O,B1_O,SB1_O,C1_O,D1_O;
	unsigned int 			fingers,button;
	int				x1,y1,z1,x2,y2,w1;


	x1=y1=x2=y2=0;
	//printk(KERN_DEBUG "+psmouse_ret_t elantech_process_byte\n");



	//printk(KERN_DEBUG "+psmouse_ret_t elantech_process_byte psmouse->fw_version=%d\n",psmouse->fw_version);
	//printk(KERN_DEBUG "Data= %.2x %.2x %.2x %.2x %.2x %.2x\n",packet[0],packet[1],packet[2],packet[3],packet[4],packet[5]);

//-----------------in check byte data--------------------------------------
		if (psmouse->pktcnt < 6)
			return PSMOUSE_GOOD_DATA;
		//printk(KERN_DEBUG "Data= %.2x %.2x %.2x %.2x %.2x %.2x\n",packet[0],packet[1],packet[2],packet[3],packet[4],packet[5]);

		SA1= packet[0];
		A1 = packet[1];
		B1 = packet[2];
		SB1= packet[3];
		C1 = packet[4];
		D1 = packet[5];

	if( ((SA1 & 0x0C) != 0x04) || // check Byte 1
	    ((SB1 & 0x0f) != 0x02) ) // check Byte  4
	{
		packet[0] = A1;
		packet[1] = B1;
		packet[2] = SB1;
		packet[3] = C1;
		packet[4] = D1;
		psmouse->pktcnt = psmouse->pktcnt - 1;
		return PSMOUSE_GOOD_DATA;
	}




//-----------------out check byte data--------------------------------------
		//printk(KERN_DEBUG "Data2= %.2x %.2x %.2x %.2x %.2x %.2x\n",packet[0],packet[1],packet[2],packet[3],packet[4],packet[5]);

	if( SA1 !=  SA1_O ||
	    A1  !=  A1_O  ||
	    B1  !=  B1_O  ||
	    SB1 !=  SB1_O ||
	    C1  !=  C1_O  ||
	    D1  !=  D1_O)
	{
                fingers = ((packet[0] & 0xC0) >> 6);

                button = ((packet[3]&0x01 << 2) | (packet[0] & 0x03));
		w1 = (((packet[0]&0x30)>>2)|((packet[3]&0x30)>>4)) & 0x0f;
		//printk(KERN_DEBUG "w1= %d \n",w1);
                if(fingers==0)
                        input_report_abs(dev,ABS_PRESSURE,0);

		if(fingers==1){

			x1=(((packet[1] & 0x0f) << 8) | packet[2]);
			y1= 0x2F0 -(((packet[4] & 0x0f) << 8) | packet[5]);
			x2=(x1*420)/100 + 1400;
			y2=(y1*562)/100 + 1400;
			z1 = ((packet[1]&0xf0)|((packet[4]&0xf0)>>4));


			input_report_abs(dev, ABS_X,x2);
			input_report_abs(dev, ABS_Y,y2);
			input_report_abs(dev,ABS_PRESSURE,z1);


			//printk(KERN_DEBUG "Data= %.2x %.2x %.2x %.2x %.2x %.2x\n",packet[0],packet[1],packet[2],packet[3],packet[4],packet[5]);
			//printk(KERN_DEBUG "x1=%.2x y1=%.2x z1=%.2x\n",x1,y1,z1);
			//printk(KERN_DEBUG "x2=%.2x y2=%.2x\n",x2,y2);

		}

		if(fingers==2){
			//input_report_key(dev, BTN_C,z > 0);
			x1=((packet[0] & 0x10) << 4) | packet[1];
			y1=0xFF -(((packet[0] & 0x20) << 3) | packet[2]);
			x2=((packet[3] & 0x10) << 4) | packet[4];
			y2=0xFF -(((packet[3] & 0x20) << 3 )| packet[5]);
			z1=61;
			input_report_abs(dev, ABS_HAT0X,x1);
			input_report_abs(dev, ABS_HAT0Y,y1);
			input_report_abs(dev, ABS_HAT1X,x2);
			input_report_abs(dev, ABS_HAT1Y,y2);
			input_report_abs(dev,ABS_PRESSURE,z1);

			//printk(KERN_DEBUG "Data= %.2x %.2x %.2x %.2x %.2x %.2x\n",packet[0],packet[1],packet[2],packet[3],packet[4],packet[5]);
			//printk(KERN_DEBUG "x1=%.2x y1=%.2x z1=%.2x\n",x1,y1,z1);
			//printk(KERN_DEBUG "x2=%.2x %d  y2=%.2x %d\n",x2,x2,y2,y2);

			//printk(KERN_DEBUG "x3=%.2x y3=%.2x z3=%.2x\n",x3,y3,z1);
			//printk(KERN_DEBUG "x4=%.2x %d  y4=%.2x %d\n",x4,x4,y4,y4);

		}
                if(fingers==3){
                        x1=(((packet[1] & 0x0f) << 8) | packet[2]);
                        y1= 0x7FF - (((packet[4] & 0x0f) << 8) | packet[5]);
			z1 = ((packet[1]&0xf0)|((packet[4]&0xf0)>>4));

                        //x2=(x1*420)/100 + 1126;
                        //y2=(y1*562)/100 + 897;
                        input_report_abs(dev, ABS_HAT2X,x1);
                        input_report_abs(dev, ABS_HAT2Y,y1);
                        input_report_abs(dev,ABS_PRESSURE,z1);
                }



		input_report_abs(dev,ABS_TOOL_WIDTH,w1);
		input_report_key(dev, BTN_TOUCH, fingers > 0);
		input_report_key(dev, BTN_TOOL_FINGER, fingers == 1);
		input_report_key(dev, BTN_TOOL_DOUBLETAP, fingers == 2);
		input_report_key(dev, BTN_TOOL_TRIPLETAP, fingers == 3);



		input_report_key(dev, BTN_LEFT,                 button == 1);
		input_report_key(dev, BTN_RIGHT,                button == 2);
		input_report_key(dev, BTN_MIDDLE,               button == 3);
		input_report_key(dev, BTN_FORWARD,              button == 4);
		input_report_key(dev, BTN_BACK,                 button == 5);
		input_report_key(dev, BTN_0,                    button == 6);
		input_report_key(dev, BTN_1,                    button == 7);
		input_report_key(dev, BTN_2,                    button == 1);
		input_report_key(dev, BTN_3,                    button == 2);
		input_report_key(dev, BTN_4,                    button == 3);

	        SA1_O = SA1;
		A1_O  = A1;
	        B1_O  = B1;
	        SB1_O = SB1;
		C1_O  = C1;
	        D1_O  = D1;
	}
	input_sync(dev);
	//printk(KERN_DEBUG "-psmouse_ret_t elantech_process_byte\n");
	return PSMOUSE_FULL_PACKET;


}


/*
 * Initialise the touchpad to a default state. Because we don't know (yet)
 * how to read registers we need to write some default values so we can
 * report their contents when asked to.
 */


static int elantech_set_4byte_defaults_EF013(struct psmouse *psmouse)
{

	struct elantech_data 	*etd = psmouse->private;
	struct input_dev 	*dev = psmouse->dev;

	int 			err,repeat_run,checkrepeat_run;

	//printk(KERN_DEBUG "+elantech_set_defaults\n");
	/*
	 * For now, use the Elantech Windows driver default values
	 */
	repeat_run=checkrepeat_run=0;

repeat_com:



	etd->reg_10 = 0x14;
	if((elantech_write_reg(psmouse, 0x10, etd->reg_10) != 0) && (repeat_run<2)){
		repeat_run++;
		goto repeat_com;
	}



	etd->reg_11 = 0x8b;
	elantech_write_reg(psmouse, 0x11, etd->reg_11);

	err=elantech_read_reg(psmouse, 0x10, etd->reg_10);

	if((err != 0) && (checkrepeat_run<2)){
		checkrepeat_run++;
		goto repeat_com;
	}

	if(err==0){
	set_bit(EV_KEY,      dev->evbit);
	set_bit(BTN_LEFT,    dev->keybit);
	set_bit(BTN_MIDDLE,  dev->keybit);
	set_bit(BTN_RIGHT,   dev->keybit);
        set_bit(BTN_FORWARD, dev->keybit);
        set_bit(BTN_BACK,    dev->keybit);

	set_bit(BTN_TOUCH,          dev->keybit);
	set_bit(BTN_TOOL_FINGER,    dev->keybit);
	set_bit(BTN_TOOL_DOUBLETAP, dev->keybit);
	set_bit(BTN_TOOL_TRIPLETAP, dev->keybit);
	set_bit(BTN_TOOL_TRIPLETAP, dev->keybit);
	/* Corner taps */
	set_bit(BTN_0, dev->keybit);
	set_bit(BTN_1, dev->keybit);
	set_bit(BTN_2, dev->keybit);
	set_bit(BTN_3, dev->keybit);
        set_bit(BTN_4, dev->keybit);
        set_bit(BTN_5, dev->keybit);
        set_bit(BTN_6, dev->keybit);
        set_bit(BTN_7, dev->keybit);
	set_bit(BTN_TOOL_FINGER,     dev->keybit);
	set_bit(BTN_TOOL_DOUBLETAP,  dev->keybit);
	set_bit(BTN_TOOL_TRIPLETAP , dev->keybit);



	set_bit(EV_ABS, dev->evbit);
//	input_set_abs_params(dev, ABS_X, ETP_XMIN, ETP_XMAX, 0, 0);
//	input_set_abs_params(dev, ABS_Y, ETP_YMIN, ETP_YMAX, 0, 0);255

	//input_set_abs_params(dev, ABS_X, 1150,6032, 0, 0);
	//input_set_abs_params(dev, ABS_Y, 1019,5980, 0,0);

	input_set_abs_params(dev, ABS_X, 0,511, 0, 0);
	input_set_abs_params(dev, ABS_Y, 0,511, 0,0);
	input_set_abs_params(dev, ABS_HAT0X,0,511, 0, 0);
	input_set_abs_params(dev, ABS_HAT0Y,0,511, 0, 0);
        input_set_abs_params(dev, ABS_HAT1X,0,511, 0, 0);
        input_set_abs_params(dev, ABS_HAT1Y,0,511, 0, 0);
        input_set_abs_params(dev, ABS_HAT2X,0,511, 0, 0);
        input_set_abs_params(dev, ABS_HAT2Y,0,511, 0, 0);
	input_set_abs_params(dev, ABS_PRESSURE, 0, 127, 0, 0);

	}
	//printk(KERN_DEBUG "-elantech_set_defaults\n");

	return err;
}

static int elantech_set_4byte_defaults_EF019(struct psmouse *psmouse)
{

	struct elantech_data 	*etd = psmouse->private;
	struct input_dev 	*dev = psmouse->dev;

	int 			err,repeat_run,checkrepeat_run;

	//printk(KERN_DEBUG "+elantech_set_defaults\n");
	/*
	 * For now, use the Elantech Windows driver default values
	 */
	repeat_run=checkrepeat_run=0;

repeat_com:



	etd->reg_10 = 0x14;
	if((elantech_write_reg(psmouse, 0x10, etd->reg_10) != 0) && (repeat_run<2)){
		repeat_run++;
		goto repeat_com;
	}



	etd->reg_11 = 0x8b;
	elantech_write_reg(psmouse, 0x11, etd->reg_11);

	err=elantech_read_reg(psmouse, 0x10, etd->reg_10);

	if((err != 0) && (checkrepeat_run<2)){
		checkrepeat_run++;
		goto repeat_com;
	}

	if(err==0){
	set_bit(EV_KEY,      dev->evbit);
	set_bit(BTN_LEFT,    dev->keybit);
	set_bit(BTN_MIDDLE,  dev->keybit);
	set_bit(BTN_RIGHT,   dev->keybit);
        set_bit(BTN_FORWARD, dev->keybit);
        set_bit(BTN_BACK,    dev->keybit);

	set_bit(BTN_TOUCH,          dev->keybit);
	set_bit(BTN_TOOL_FINGER,    dev->keybit);
	set_bit(BTN_TOOL_DOUBLETAP, dev->keybit);
	set_bit(BTN_TOOL_TRIPLETAP, dev->keybit);
	set_bit(BTN_TOOL_TRIPLETAP, dev->keybit);
	/* Corner taps */
	set_bit(BTN_0, dev->keybit);
	set_bit(BTN_1, dev->keybit);
	set_bit(BTN_2, dev->keybit);
	set_bit(BTN_3, dev->keybit);
        set_bit(BTN_4, dev->keybit);
        set_bit(BTN_5, dev->keybit);
        set_bit(BTN_6, dev->keybit);
        set_bit(BTN_7, dev->keybit);
	set_bit(BTN_TOOL_FINGER,     dev->keybit);
	set_bit(BTN_TOOL_DOUBLETAP,  dev->keybit);
	set_bit(BTN_TOOL_TRIPLETAP , dev->keybit);



	set_bit(EV_ABS, dev->evbit);
//	input_set_abs_params(dev, ABS_X, ETP_XMIN, ETP_XMAX, 0, 0);
//	input_set_abs_params(dev, ABS_Y, ETP_YMIN, ETP_YMAX, 0, 0);255

	//input_set_abs_params(dev, ABS_X, 1150,6032, 0, 0);
	//input_set_abs_params(dev, ABS_Y, 1019,5980, 0,0);

	input_set_abs_params(dev, ABS_X, 0,511, 0, 0);
	input_set_abs_params(dev, ABS_Y, 0,511, 0,0);
	input_set_abs_params(dev, ABS_HAT0X,0,511, 0, 0);
	input_set_abs_params(dev, ABS_HAT0Y,0,511, 0, 0);
        input_set_abs_params(dev, ABS_HAT1X,0,511, 0, 0);
        input_set_abs_params(dev, ABS_HAT1Y,0,511, 0, 0);
        input_set_abs_params(dev, ABS_HAT2X,0,511, 0, 0);
        input_set_abs_params(dev, ABS_HAT2Y,0,511, 0, 0);
	input_set_abs_params(dev, ABS_PRESSURE, 0, 127, 0, 0);

	}
	//printk(KERN_DEBUG "-elantech_set_defaults\n");

	return err;
}

static int elantech_set_6byte_defaults_EF113(struct psmouse *psmouse)
{

	struct elantech_data 	*etd = psmouse->private;
	struct input_dev 	*dev = psmouse->dev;
	int 			err,repeat_run,checkrepeat_run;

	//printk(KERN_DEBUG "+elantech_set_defaults\n");
	/*
	 * For now, use the Elantech Windows driver default values
	 */
	repeat_run=checkrepeat_run=0;
	err=-1;

repeat_com:

	etd->reg_10 = 0x54;
	err=elantech_write_reg_new(psmouse, 0x10, etd->reg_10);
	if((err != 0) && (repeat_run<2)){
		repeat_run++;
		goto repeat_com;
	}

	etd->reg_11 = 0x8a;
	err=elantech_write_reg_new(psmouse, 0x11, etd->reg_11);
	//elantech_read_reg_new(psmouse, 0x11, etd->reg_11);

	/*etd->reg_21 = 0x60;
	elantech_write_reg_new(psmouse, 0x21, etd->reg_21);
	//elantech_read_reg_new(psmouse, 0x21, etd->reg_21);*/



	err = elantech_read_reg_new(psmouse, 0x10, etd->reg_10);

	if((err != 0) && (checkrepeat_run<2)){
		checkrepeat_run++;
		goto repeat_com;
	}

	if(err==0){
	set_bit(EV_KEY,      dev->evbit);
	set_bit(BTN_LEFT,    dev->keybit);
	set_bit(BTN_MIDDLE,  dev->keybit);
	set_bit(BTN_RIGHT,   dev->keybit);
        set_bit(BTN_FORWARD, dev->keybit);
        set_bit(BTN_BACK,    dev->keybit);

	set_bit(BTN_TOUCH,          dev->keybit);
	set_bit(BTN_TOOL_FINGER,    dev->keybit);
	set_bit(BTN_TOOL_DOUBLETAP, dev->keybit);
	set_bit(BTN_TOOL_TRIPLETAP, dev->keybit);
	set_bit(BTN_TOOL_TRIPLETAP, dev->keybit);
	/* Corner taps */
	set_bit(BTN_0, dev->keybit);
	set_bit(BTN_1, dev->keybit);
	set_bit(BTN_2, dev->keybit);
	set_bit(BTN_3, dev->keybit);
        set_bit(BTN_4, dev->keybit);
        set_bit(BTN_5, dev->keybit);
	set_bit(BTN_TOOL_FINGER,    dev->keybit);
	set_bit(BTN_TOOL_DOUBLETAP, dev->keybit);
	set_bit(BTN_TOOL_TRIPLETAP, dev->keybit);



	set_bit(EV_ABS, dev->evbit);
//	input_set_abs_params(dev, ABS_X, ETP_XMIN, ETP_XMAX, 0, 0);
//	input_set_abs_params(dev, ABS_Y, ETP_YMIN, ETP_YMAX, 0, 0);

	input_set_abs_params(dev, ABS_X,1202, 5876, 0, 0);
	input_set_abs_params(dev, ABS_Y, 999, 5112, 0,0);
	input_set_abs_params(dev, ABS_HAT0X, 4, 281, 0, 0);
	input_set_abs_params(dev, ABS_HAT0Y,4,187, 0, 0);
	input_set_abs_params(dev, ABS_HAT1X,4, 281, 0, 0);
	input_set_abs_params(dev, ABS_HAT1Y,4, 187, 0, 0);
	//input_set_abs_params(dev, ABS_TOOL_WIDTH,0,16,0,0);
	input_set_abs_params(dev, ABS_PRESSURE, 0, 255, 0, 0);
	input_set_abs_params(dev, ABS_HAT2X,0,2047, 0, 0);
	input_set_abs_params(dev, ABS_HAT2Y,0,2047, 0, 0);


/*
        input_set_abs_params(dev, ABS_X, 0,2047, 0, 0);
	input_set_abs_params(dev, ABS_Y, 0,2047, 0,0);
	input_set_abs_params(dev, ABS_HAT0X,0,255, 0, 0);
	input_set_abs_params(dev, ABS_HAT0Y,0,255, 0, 0);
	input_set_abs_params(dev, ABS_HAT1X,0,255, 0, 0);
	input_set_abs_params(dev, ABS_HAT1Y,0,255, 0, 0);
	input_set_abs_params(dev, ABS_HAT2X,0,2047, 0, 0);
	input_set_abs_params(dev, ABS_HAT2Y,0,2047, 0, 0);
	input_set_abs_params(dev, ABS_PRESSURE, 0, 127, 0, 0);
*/
	}
	//printk(KERN_DEBUG "-elantech_set_defaults\n");

	return err;
}

static int elantech_set_6byte_defaults_EF023(struct psmouse *psmouse)
{

	struct elantech_data 	*etd = psmouse->private;
	struct input_dev 	*dev = psmouse->dev;
	int 			err,repeat_run,checkrepeat_run;

	//printk(KERN_DEBUG "+elantech_set_defaults\n");
	/*
	 * For now, use the Elantech Windows driver default values
	 */
	repeat_run=checkrepeat_run=0;
	err=-1;

repeat_com:

	etd->reg_10 = 0xc4;
	err=elantech_write_reg_new(psmouse, 0x10, etd->reg_10);
	if((err != 0) && (repeat_run<2)){
		repeat_run++;
		goto repeat_com;
	}

	etd->reg_11 = 0x8a;
	err=elantech_write_reg_new(psmouse, 0x11, etd->reg_11);
	//elantech_read_reg_new(psmouse, 0x11, etd->reg_11);

	/*etd->reg_21 = 0x60;
	elantech_write_reg_new(psmouse, 0x21, etd->reg_21);
	//elantech_read_reg_new(psmouse, 0x21, etd->reg_21);*/



	err = elantech_read_reg_new(psmouse, 0x10, etd->reg_10);

	if((err != 0) && (checkrepeat_run<2)){
		checkrepeat_run++;
		goto repeat_com;
	}

	if(err==0){
	set_bit(EV_KEY,      dev->evbit);
	set_bit(BTN_LEFT,    dev->keybit);
	set_bit(BTN_MIDDLE,  dev->keybit);
	set_bit(BTN_RIGHT,   dev->keybit);
        set_bit(BTN_FORWARD, dev->keybit);
        set_bit(BTN_BACK,    dev->keybit);

	set_bit(BTN_TOUCH,          dev->keybit);
	set_bit(BTN_TOOL_FINGER,    dev->keybit);
	set_bit(BTN_TOOL_DOUBLETAP, dev->keybit);
	set_bit(BTN_TOOL_TRIPLETAP, dev->keybit);
	set_bit(BTN_TOOL_TRIPLETAP, dev->keybit);
	/* Corner taps */
	set_bit(BTN_0, dev->keybit);
	set_bit(BTN_1, dev->keybit);
	set_bit(BTN_2, dev->keybit);
	set_bit(BTN_3, dev->keybit);
        set_bit(BTN_4, dev->keybit);
        set_bit(BTN_5, dev->keybit);
	set_bit(BTN_TOOL_FINGER,    dev->keybit);
	set_bit(BTN_TOOL_DOUBLETAP, dev->keybit);
	set_bit(BTN_TOOL_TRIPLETAP, dev->keybit);



	set_bit(EV_ABS, dev->evbit);
//	input_set_abs_params(dev, ABS_X, ETP_XMIN, ETP_XMAX, 0, 0);
//	input_set_abs_params(dev, ABS_Y, ETP_YMIN, ETP_YMAX, 0, 0);

	input_set_abs_params(dev, ABS_X,1202, 5876, 0, 0);
	input_set_abs_params(dev, ABS_Y, 999, 5112, 0,0);
	input_set_abs_params(dev, ABS_HAT0X, 4, 281, 0, 0);
	input_set_abs_params(dev, ABS_HAT0Y,4,187, 0, 0);
	input_set_abs_params(dev, ABS_HAT1X,4, 281, 0, 0);
	input_set_abs_params(dev, ABS_HAT1Y,4, 187, 0, 0);
	input_set_abs_params(dev, ABS_HAT2X,0,2047, 0, 0);
	input_set_abs_params(dev, ABS_HAT2Y,0,2047, 0, 0);
	input_set_abs_params(dev, ABS_TOOL_WIDTH,0,16,0,0);
	input_set_abs_params(dev, ABS_PRESSURE, 0, 255, 0, 0);
/*
        input_set_abs_params(dev, ABS_X, 0,2047, 0, 0);
	input_set_abs_params(dev, ABS_Y, 0,2047, 0,0);
	input_set_abs_params(dev, ABS_HAT0X,0,255, 0, 0);
	input_set_abs_params(dev, ABS_HAT0Y,0,255, 0, 0);
	input_set_abs_params(dev, ABS_HAT1X,0,255, 0, 0);
	input_set_abs_params(dev, ABS_HAT1Y,0,255, 0, 0);
	input_set_abs_params(dev, ABS_HAT2X,0,2047, 0, 0);
	input_set_abs_params(dev, ABS_HAT2Y,0,2047, 0, 0);
	input_set_abs_params(dev, ABS_PRESSURE, 0, 127, 0, 0);
*/
	}
	//printk(KERN_DEBUG "-elantech_set_defaults\n");

	return err;
}

static int elantech_set_6byte_defaults_EF123(struct psmouse *psmouse)
{

	struct elantech_data 	*etd = psmouse->private;
	struct input_dev 	*dev = psmouse->dev;
	int 			err,repeat_run,checkrepeat_run;

	//printk(KERN_DEBUG "+elantech_set_defaults\n");
	/*
	 * For now, use the Elantech Windows driver default values
	 */
	repeat_run=checkrepeat_run=0;
	err=-1;

repeat_com:

	etd->reg_10 = 0xc4;
	err=elantech_write_reg_new(psmouse, 0x10, etd->reg_10);
	if((err != 0) && (repeat_run<2)){
		repeat_run++;
		goto repeat_com;
	}

	etd->reg_11 = 0x8a;
	err=elantech_write_reg_new(psmouse, 0x11, etd->reg_11);
	//elantech_read_reg_new(psmouse, 0x11, etd->reg_11);

	/*etd->reg_21 = 0x60;
	elantech_write_reg_new(psmouse, 0x21, etd->reg_21);
	//elantech_read_reg_new(psmouse, 0x21, etd->reg_21);*/



	err = elantech_read_reg_new(psmouse, 0x10, etd->reg_10);

	if((err != 0) && (checkrepeat_run<2)){
		checkrepeat_run++;
		goto repeat_com;
	}

	if(err==0){
	set_bit(EV_KEY,      dev->evbit);
	set_bit(BTN_LEFT,    dev->keybit);
	set_bit(BTN_MIDDLE,  dev->keybit);
	set_bit(BTN_RIGHT,   dev->keybit);
        set_bit(BTN_FORWARD, dev->keybit);
        set_bit(BTN_BACK,    dev->keybit);

	set_bit(BTN_TOUCH,          dev->keybit);
	set_bit(BTN_TOOL_FINGER,    dev->keybit);
	set_bit(BTN_TOOL_DOUBLETAP, dev->keybit);
	set_bit(BTN_TOOL_TRIPLETAP, dev->keybit);
	set_bit(BTN_TOOL_TRIPLETAP, dev->keybit);
	/* Corner taps */
	set_bit(BTN_0, dev->keybit);
	set_bit(BTN_1, dev->keybit);
	set_bit(BTN_2, dev->keybit);
	set_bit(BTN_3, dev->keybit);
        set_bit(BTN_4, dev->keybit);
        set_bit(BTN_5, dev->keybit);
	set_bit(BTN_TOOL_FINGER,    dev->keybit);
	set_bit(BTN_TOOL_DOUBLETAP, dev->keybit);
	set_bit(BTN_TOOL_TRIPLETAP, dev->keybit);



	set_bit(EV_ABS, dev->evbit);
//	input_set_abs_params(dev, ABS_X, ETP_XMIN, ETP_XMAX, 0, 0);
//	input_set_abs_params(dev, ABS_Y, ETP_YMIN, ETP_YMAX, 0, 0);

	input_set_abs_params(dev, ABS_X,1202, 5876, 0, 0);
	input_set_abs_params(dev, ABS_Y, 999, 5112, 0,0);
	input_set_abs_params(dev, ABS_HAT0X, 4, 281, 0, 0);
	input_set_abs_params(dev, ABS_HAT0Y,4,187, 0, 0);
	input_set_abs_params(dev, ABS_HAT1X,4, 281, 0, 0);
	input_set_abs_params(dev, ABS_HAT1Y,4, 187, 0, 0);
	input_set_abs_params(dev, ABS_HAT2X,0,2047, 0, 0);
	input_set_abs_params(dev, ABS_HAT2Y,0,2047, 0, 0);
	input_set_abs_params(dev, ABS_TOOL_WIDTH,0,16,0,0);
	input_set_abs_params(dev, ABS_PRESSURE, 0, 255, 0, 0);
/*
        input_set_abs_params(dev, ABS_X, 0,2047, 0, 0);
	input_set_abs_params(dev, ABS_Y, 0,2047, 0,0);
	input_set_abs_params(dev, ABS_HAT0X,0,255, 0, 0);
	input_set_abs_params(dev, ABS_HAT0Y,0,255, 0, 0);
	input_set_abs_params(dev, ABS_HAT1X,0,255, 0, 0);
	input_set_abs_params(dev, ABS_HAT1Y,0,255, 0, 0);
	input_set_abs_params(dev, ABS_HAT2X,0,2047, 0, 0);
	input_set_abs_params(dev, ABS_HAT2Y,0,2047, 0, 0);
	input_set_abs_params(dev, ABS_PRESSURE, 0, 127, 0, 0);
*/
	}
	//printk(KERN_DEBUG "-elantech_set_defaults\n");

	return err;
}

static int elantech_set_6byte_defaults_EF215(struct psmouse *psmouse)
{

	struct elantech_data 	*etd = psmouse->private;
	struct input_dev 	*dev = psmouse->dev;
	int 			err,repeat_run,checkrepeat_run;

	//printk(KERN_DEBUG "+elantech_set_defaults\n");
	/*
	 * For now, use the Elantech Windows driver default values
	 */
	repeat_run=checkrepeat_run=0;
	err=-1;

repeat_com:

	etd->reg_10 = 0xc4;
	err=elantech_write_reg_new(psmouse, 0x10, etd->reg_10);
	if((err != 0) && (repeat_run<2)){
		repeat_run++;
		goto repeat_com;
	}

	etd->reg_11 = 0x8a;
	err=elantech_write_reg_new(psmouse, 0x11, etd->reg_11);
	//elantech_read_reg_new(psmouse, 0x11, etd->reg_11);

	/*etd->reg_21 = 0x60;
	elantech_write_reg_new(psmouse, 0x21, etd->reg_21);
	//elantech_read_reg_new(psmouse, 0x21, etd->reg_21);*/



	err = elantech_read_reg_new(psmouse, 0x10, etd->reg_10);

	if((err != 0) && (checkrepeat_run<2)){
		checkrepeat_run++;
		goto repeat_com;
	}

	if(err==0){
	set_bit(EV_KEY,      dev->evbit);
	set_bit(BTN_LEFT,    dev->keybit);
	set_bit(BTN_MIDDLE,  dev->keybit);
	set_bit(BTN_RIGHT,   dev->keybit);
        set_bit(BTN_FORWARD, dev->keybit);
        set_bit(BTN_BACK,    dev->keybit);

	set_bit(BTN_TOUCH,          dev->keybit);
	set_bit(BTN_TOOL_FINGER,    dev->keybit);
	set_bit(BTN_TOOL_DOUBLETAP, dev->keybit);
	set_bit(BTN_TOOL_TRIPLETAP, dev->keybit);
	set_bit(BTN_TOOL_TRIPLETAP, dev->keybit);
	/* Corner taps */
	set_bit(BTN_0, dev->keybit);
	set_bit(BTN_1, dev->keybit);
	set_bit(BTN_2, dev->keybit);
	set_bit(BTN_3, dev->keybit);
        set_bit(BTN_4, dev->keybit);
        set_bit(BTN_5, dev->keybit);
	set_bit(BTN_TOOL_FINGER,    dev->keybit);
	set_bit(BTN_TOOL_DOUBLETAP, dev->keybit);
	set_bit(BTN_TOOL_TRIPLETAP, dev->keybit);



	set_bit(EV_ABS, dev->evbit);
//	input_set_abs_params(dev, ABS_X, ETP_XMIN, ETP_XMAX, 0, 0);
//	input_set_abs_params(dev, ABS_Y, ETP_YMIN, ETP_YMAX, 0, 0);

	input_set_abs_params(dev, ABS_X,1202, 5876, 0, 0);
	input_set_abs_params(dev, ABS_Y, 999, 5112, 0,0);
	input_set_abs_params(dev, ABS_HAT0X, 4, 281, 0, 0);
	input_set_abs_params(dev, ABS_HAT0Y,4,187, 0, 0);
	input_set_abs_params(dev, ABS_HAT1X,4, 281, 0, 0);
	input_set_abs_params(dev, ABS_HAT1Y,4, 187, 0, 0);
	input_set_abs_params(dev, ABS_HAT2X,0,2047, 0, 0);
	input_set_abs_params(dev, ABS_HAT2Y,0,2047, 0, 0);
	input_set_abs_params(dev, ABS_TOOL_WIDTH,0,16,0,0);
	input_set_abs_params(dev, ABS_PRESSURE, 0, 255, 0, 0);
/*
        input_set_abs_params(dev, ABS_X, 0,2047, 0, 0);
	input_set_abs_params(dev, ABS_Y, 0,2047, 0,0);
	input_set_abs_params(dev, ABS_HAT0X,0,255, 0, 0);
	input_set_abs_params(dev, ABS_HAT0Y,0,255, 0, 0);
	input_set_abs_params(dev, ABS_HAT1X,0,255, 0, 0);
	input_set_abs_params(dev, ABS_HAT1Y,0,255, 0, 0);
	input_set_abs_params(dev, ABS_HAT2X,0,2047, 0, 0);
	input_set_abs_params(dev, ABS_HAT2Y,0,2047, 0, 0);
	input_set_abs_params(dev, ABS_PRESSURE, 0, 127, 0, 0);
*/
	}
	//printk(KERN_DEBUG "-elantech_set_defaults\n");

	return err;
}


static int elantech_set_6byte_defaults_EF051(struct psmouse *psmouse)
{

	struct elantech_data 	*etd = psmouse->private;
	struct input_dev 	*dev = psmouse->dev;
	int 			err,repeat_run,checkrepeat_run;

	//printk(KERN_DEBUG "+elantech_set_defaults\n");
	/*
	 * For now, use the Elantech Windows driver default values
	 */
	repeat_run=checkrepeat_run=0;
	err=-1;

repeat_com:

	etd->reg_10 = 0xc4;
	err=elantech_write_reg_new(psmouse, 0x10, etd->reg_10);
	if((err != 0) && (repeat_run<2)){
		repeat_run++;
		goto repeat_com;
	}

	etd->reg_11 = 0x8a;
	err=elantech_write_reg_new(psmouse, 0x11, etd->reg_11);
	//elantech_read_reg_new(psmouse, 0x11, etd->reg_11);

	/*etd->reg_21 = 0x60;
	elantech_write_reg_new(psmouse, 0x21, etd->reg_21);
	//elantech_read_reg_new(psmouse, 0x21, etd->reg_21);*/



	err = elantech_read_reg_new(psmouse, 0x10, etd->reg_10);

	if((err != 0) && (checkrepeat_run<2)){
		checkrepeat_run++;
		goto repeat_com;
	}

	if(err==0){
	set_bit(EV_KEY,      dev->evbit);
	set_bit(BTN_LEFT,    dev->keybit);
	set_bit(BTN_MIDDLE,  dev->keybit);
	set_bit(BTN_RIGHT,   dev->keybit);
        set_bit(BTN_FORWARD, dev->keybit);
        set_bit(BTN_BACK,    dev->keybit);

	set_bit(BTN_TOUCH,          dev->keybit);
	set_bit(BTN_TOOL_FINGER,    dev->keybit);
	set_bit(BTN_TOOL_DOUBLETAP, dev->keybit);
	set_bit(BTN_TOOL_TRIPLETAP, dev->keybit);
	set_bit(BTN_TOOL_TRIPLETAP, dev->keybit);
	/* Corner taps */
	set_bit(BTN_0, dev->keybit);
	set_bit(BTN_1, dev->keybit);
	set_bit(BTN_2, dev->keybit);
	set_bit(BTN_3, dev->keybit);
        set_bit(BTN_4, dev->keybit);
        set_bit(BTN_5, dev->keybit);
	set_bit(BTN_TOOL_FINGER,    dev->keybit);
	set_bit(BTN_TOOL_DOUBLETAP, dev->keybit);
	set_bit(BTN_TOOL_TRIPLETAP, dev->keybit);



	set_bit(EV_ABS, dev->evbit);
//	input_set_abs_params(dev, ABS_X, ETP_XMIN, ETP_XMAX, 0, 0);
//	input_set_abs_params(dev, ABS_Y, ETP_YMIN, ETP_YMAX, 0, 0);

	input_set_abs_params(dev, ABS_X,1202, 5876, 0, 0);
	input_set_abs_params(dev, ABS_Y, 999, 5112, 0,0);
	input_set_abs_params(dev, ABS_HAT0X, 4, 281, 0, 0);
	input_set_abs_params(dev, ABS_HAT0Y,4,187, 0, 0);
	input_set_abs_params(dev, ABS_HAT1X,4, 281, 0, 0);
	input_set_abs_params(dev, ABS_HAT1Y,4, 187, 0, 0);
	input_set_abs_params(dev, ABS_HAT2X,0,2047, 0, 0);
	input_set_abs_params(dev, ABS_HAT2Y,0,2047, 0, 0);
	input_set_abs_params(dev, ABS_TOOL_WIDTH,0,16,0,0);
	input_set_abs_params(dev, ABS_PRESSURE, 0, 255, 0, 0);
/*
        input_set_abs_params(dev, ABS_X, 0,2047, 0, 0);
	input_set_abs_params(dev, ABS_Y, 0,2047, 0,0);
	input_set_abs_params(dev, ABS_HAT0X,0,255, 0, 0);
	input_set_abs_params(dev, ABS_HAT0Y,0,255, 0, 0);
	input_set_abs_params(dev, ABS_HAT1X,0,255, 0, 0);
	input_set_abs_params(dev, ABS_HAT1Y,0,255, 0, 0);
	input_set_abs_params(dev, ABS_HAT2X,0,2047, 0, 0);
	input_set_abs_params(dev, ABS_HAT2Y,0,2047, 0, 0);
	input_set_abs_params(dev, ABS_PRESSURE, 0, 127, 0, 0);
*/
	}
	//printk(KERN_DEBUG "-elantech_set_defaults\n");

	return err;
}

static int elantech_set_6byte_defaults_Scroll_EF051(struct psmouse *psmouse)
{

	struct elantech_data 	*etd = psmouse->private;
	struct input_dev 	*dev = psmouse->dev;
	int 			err,repeat_run,checkrepeat_run;

	//printk(KERN_DEBUG "+elantech_set_defaults\n");
	/*
	 * For now, use the Elantech Windows driver default values
	 */
	repeat_run=checkrepeat_run=0;
	err=-1;

repeat_com:

	etd->reg_10 = 0xc4;
	err=elantech_write_reg_new(psmouse, 0x10, etd->reg_10);
	if((err != 0) && (repeat_run<2)){
		repeat_run++;
		goto repeat_com;
	}

	etd->reg_11 = 0x8a;
	err=elantech_write_reg_new(psmouse, 0x11, etd->reg_11);
	//elantech_read_reg_new(psmouse, 0x11, etd->reg_11);

	/*etd->reg_21 = 0x60;
	elantech_write_reg_new(psmouse, 0x21, etd->reg_21);
	//elantech_read_reg_new(psmouse, 0x21, etd->reg_21);*/



	err = elantech_read_reg_new(psmouse, 0x10, etd->reg_10);

	if((err != 0) && (checkrepeat_run<2)){
		checkrepeat_run++;
		goto repeat_com;
	}

	if(err==0){
	set_bit(EV_KEY,      dev->evbit);
	set_bit(BTN_LEFT,    dev->keybit);
	set_bit(BTN_MIDDLE,  dev->keybit);
	set_bit(BTN_RIGHT,   dev->keybit);
        set_bit(BTN_FORWARD, dev->keybit);
        set_bit(BTN_BACK,    dev->keybit);

	set_bit(BTN_TOUCH,          dev->keybit);
	set_bit(BTN_TOOL_FINGER,    dev->keybit);
	set_bit(BTN_TOOL_DOUBLETAP, dev->keybit);
	set_bit(BTN_TOOL_TRIPLETAP, dev->keybit);
	set_bit(BTN_TOOL_TRIPLETAP, dev->keybit);
	/* Corner taps */
	set_bit(BTN_0, dev->keybit);
	set_bit(BTN_1, dev->keybit);
	set_bit(BTN_2, dev->keybit);
	set_bit(BTN_3, dev->keybit);
        set_bit(BTN_4, dev->keybit);
        set_bit(BTN_5, dev->keybit);
	set_bit(BTN_TOOL_FINGER,    dev->keybit);
	set_bit(BTN_TOOL_DOUBLETAP, dev->keybit);
	set_bit(BTN_TOOL_TRIPLETAP, dev->keybit);



	set_bit(EV_ABS, dev->evbit);
//	input_set_abs_params(dev, ABS_X, ETP_XMIN, ETP_XMAX, 0, 0);
//	input_set_abs_params(dev, ABS_Y, ETP_YMIN, ETP_YMAX, 0, 0);

	input_set_abs_params(dev, ABS_X,1202, 5876, 0, 0);
	input_set_abs_params(dev, ABS_Y, 999, 5112, 0,0);
	input_set_abs_params(dev, ABS_HAT0X, 4, 281, 0, 0);
	input_set_abs_params(dev, ABS_HAT0Y,4,187, 0, 0);
	input_set_abs_params(dev, ABS_HAT1X,4, 281, 0, 0);
	input_set_abs_params(dev, ABS_HAT1Y,4, 187, 0, 0);
	input_set_abs_params(dev, ABS_HAT2X,0,2047, 0, 0);
	input_set_abs_params(dev, ABS_HAT2Y,0,2047, 0, 0);
	input_set_abs_params(dev, ABS_TOOL_WIDTH,0,16,0,0);
	input_set_abs_params(dev, ABS_PRESSURE, 0, 255, 0, 0);
/*
        input_set_abs_params(dev, ABS_X, 0,2047, 0, 0);
	input_set_abs_params(dev, ABS_Y, 0,2047, 0,0);
	input_set_abs_params(dev, ABS_HAT0X,0,255, 0, 0);
	input_set_abs_params(dev, ABS_HAT0Y,0,255, 0, 0);
	input_set_abs_params(dev, ABS_HAT1X,0,255, 0, 0);
	input_set_abs_params(dev, ABS_HAT1Y,0,255, 0, 0);
	input_set_abs_params(dev, ABS_HAT2X,0,2047, 0, 0);
	input_set_abs_params(dev, ABS_HAT2Y,0,2047, 0, 0);
	input_set_abs_params(dev, ABS_PRESSURE, 0, 127, 0, 0);
*/
	}
	//printk(KERN_DEBUG "-elantech_set_defaults\n");

	return err;
}


struct elantech_attr_data {
	size_t		field_offset;
	unsigned char	reg;
};

/*
 * Display a register value by reading a sysfs entry
 */
static ssize_t elantech_show_int_attr(struct psmouse *psmouse, void *data, char *buf)
{

	struct elantech_data		*etd = psmouse->private;
	struct elantech_attr_data	*attr = data;
	unsigned char			*reg = (unsigned char *)
						etd + attr->field_offset;
	//printk(KERN_DEBUG "+ssize_t elantech_show_int_attr\n");
	//printk(KERN_DEBUG "-ssize_t elantech_show_int_attr\n");
	return sprintf(buf, "0x%02x\n", *reg);
}

/*
 * Write a register value by writing a sysfs entry
 */
static ssize_t elantech_set_int_attr(struct psmouse *psmouse, void *data,
						const char *buf, size_t count)
{

	struct elantech_data 		*etd = psmouse->private;
	struct elantech_attr_data 	*attr = data;
	unsigned char 			*reg = (unsigned char *)
						etd + attr->field_offset;
	unsigned long			value;
	char				*rest;
	//printk(KERN_DEBUG "+ssize_t elantech_set_int_attr\n");
	value = simple_strtoul(buf, &rest, 16);
	if (*rest || value > 255)
		return -EINVAL;

	/* Force 4 byte packet mode because driver expects this */
	if (attr->reg == 0x11)
		value |= ETP_4_BYTE_MODE;

	*reg = value;
	elantech_write_reg(psmouse, attr->reg, value);
	//printk(KERN_DEBUG "-ssize_t elantech_set_int_attr\n");
	return count;
}

#define ELANTECH_INT_ATTR(_name, _register)				\
	static struct elantech_attr_data elantech_attr_##_name = {	\
		.field_offset = offsetof(struct elantech_data, _name),	\
		.reg = _register,					\
	};								\
	PSMOUSE_DEFINE_ATTR(_name, S_IWUSR | S_IRUGO,			\
			    &elantech_attr_##_name,			\
			    elantech_show_int_attr,			\
			    elantech_set_int_attr)

ELANTECH_INT_ATTR(reg_10, 0x10);
ELANTECH_INT_ATTR(reg_11, 0x11);
ELANTECH_INT_ATTR(reg_20, 0x20);
ELANTECH_INT_ATTR(reg_21, 0x21);
ELANTECH_INT_ATTR(reg_22, 0x22);
ELANTECH_INT_ATTR(reg_23, 0x23);
ELANTECH_INT_ATTR(reg_24, 0x24);
ELANTECH_INT_ATTR(reg_25, 0x25);
ELANTECH_INT_ATTR(reg_26, 0x26);

static struct attribute *elantech_attrs[] = {
	&psmouse_attr_reg_10.dattr.attr,
	&psmouse_attr_reg_11.dattr.attr,
	&psmouse_attr_reg_20.dattr.attr,
	&psmouse_attr_reg_21.dattr.attr,
	&psmouse_attr_reg_22.dattr.attr,
	&psmouse_attr_reg_23.dattr.attr,
	&psmouse_attr_reg_24.dattr.attr,
	&psmouse_attr_reg_25.dattr.attr,
	&psmouse_attr_reg_26.dattr.attr,
	NULL
};

static struct attribute_group elantech_attr_group = {
	.attrs = elantech_attrs,
};

/*
 * Clean up sysfs entries when disconnecting
 */
static void elantech_disconnect(struct psmouse *psmouse)
{
	printk(KERN_DEBUG "elantech_disconnect\n");
	sysfs_remove_group(&psmouse->ps2dev.serio->dev.kobj,
				&elantech_attr_group);
	kfree(psmouse->private);
	psmouse->private = NULL;
	//printk(KERN_DEBUG "-elantech_disconnect\n");
}

/*
 * Use magic knock to detect Elantech touchpad
 */


static int elantech_reconnect_EF023(struct psmouse *psmouse)
{
	struct ps2dev		*ps2dev = &psmouse->ps2dev;
	int i;
	int err,debug_err;
	err=i8042_command(NULL,I8042_CMD_KBD_DISABLE);// tom +

	if(!err)
		printk(KERN_INFO"i8042_command I8042_CMD_KBD_DISABLE OK\n");

	switch(EF_023_DEBUG){

		case 1:
				ps2_command(ps2dev,  NULL, PSMOUSE_CMD_DISABLE);
				debug_err=elantech_write_reg_debug(psmouse, 0x12, 0x04);
				debug_err=elantech_write_reg_debug(psmouse, 0x60, 0x00);
				debug_err=elantech_write_reg_debug(psmouse, 0x12, 0x00);
				EF_023_DEBUG=0;

				break;

		case 2:
				ps2_command(ps2dev,  NULL, PSMOUSE_CMD_DISABLE);
				debug_err=elantech_write_reg_debug(psmouse, 0x12, 0x04);
				debug_err=elantech_write_reg_debug(psmouse, 0x1F, 0x00);
				debug_err=elantech_write_reg_debug(psmouse, 0x12, 0x00);
				EF_023_DEBUG=0;
				break;
		case 3:
				ps2_command(ps2dev,  NULL, PSMOUSE_CMD_DISABLE);
				debug_err=elantech_write_reg_debug(psmouse, 0x12, 0x04);
				debug_err=elantech_write_reg_debug(psmouse, 0x13, 0x08);
				debug_err=elantech_write_reg_debug(psmouse, 0x12, 0x00);
				EF_023_DEBUG=0;
				break;
		case 4:
				ps2_command(ps2dev,  NULL, PSMOUSE_CMD_DISABLE);
				debug_err=elantech_write_reg_debug(psmouse, 0x12, 0x04);
				debug_err=elantech_write_reg_debug(psmouse, 0x06, 0x07);
				debug_err=elantech_write_reg_debug(psmouse, 0x12, 0x00);
				EF_023_DEBUG=0;
				break;
		case 5:
				ps2_command(ps2dev,  NULL, PSMOUSE_CMD_DISABLE);
				debug_err=elantech_write_reg_debug(psmouse, 0x12, 0x04);
				debug_err=elantech_write_reg_debug(psmouse, 0x06, 0x17);
				debug_err=elantech_write_reg_debug(psmouse, 0x12, 0x00);
				EF_023_DEBUG=0;
				break;
#if 0
		case 6:
				ps2_command(ps2dev,  NULL, PSMOUSE_CMD_DISABLE);
				debug_err=elantech_write_reg_debug(psmouse, 0x12, 0x04);
				debug_err=elantech_write_reg_debug(psmouse, 0x06, 0x17);
				debug_err=elantech_write_reg_debug(psmouse, 0x12, 0x00);
				debug_err=elantech_write_reg_debug(psmouse, 0x12, 0x04);
				debug_err=elantech_write_reg_debug(psmouse, 0x60, 0x00);
				debug_err=elantech_write_reg_debug(psmouse, 0x12, 0x00);
				//ps2_command(ps2dev,  NULL, PSMOUSE_CMD_ENABLE);
				printk(KERN_DEBUG "EF_023_DEBUG=%d \n",EF_023_DEBUG);
				EF_023_DEBUG=0;
				printk(KERN_DEBUG "EF_023_DEBUG=%d\n",EF_023_DEBUG);
				break;
#endif

		default:

			for(i=0;i<3;i++){
				printk(KERN_INFO"elantech_reconnect\n");
				if (!elantech_detect(psmouse, 0)){
					if (!elantech_init(psmouse,0))
					break;
				}
				else
					continue;
			}

			if(i>=3){
				printk(KERN_INFO"elantech_reconnect fail\n");
				if(!err)
					err=i8042_command(NULL,I8042_CMD_KBD_ENABLE);// tom +
					if(!err)
					printk(KERN_INFO"i8042_command I8042_CMD_KBD_ENABLE OK\n");
					return -1;
			}
			break;

	}
	if(!err)
		err=i8042_command(NULL,I8042_CMD_KBD_ENABLE);// tom +
	if(!err)
		printk(KERN_INFO"i8042_command I8042_CMD_KBD_ENABLE OK\n");
	return 0;
}

static int elantech_reconnect(struct psmouse *psmouse)
{

	int i;
	int err;
	err=i8042_command(NULL,I8042_CMD_KBD_DISABLE);// tom +

	if(!err)
		printk(KERN_INFO"i8042_command I8042_CMD_KBD_DISABLE OK\n");
	for(i=0;i<3;i++){
		printk(KERN_INFO"elantech_reconnect\n");
		if (!elantech_detect(psmouse, 0)){
			if (!elantech_init(psmouse,0))
			  break;
		}
		else
			continue;
	}

	if(i>=3){
		printk(KERN_INFO"elantech_reconnect fail\n");
		if(!err)
			err=i8042_command(NULL,I8042_CMD_KBD_ENABLE);// tom +
		if(!err)
			printk(KERN_INFO"i8042_command I8042_CMD_KBD_ENABLE OK\n");
		return -1;
	}
	if(!err)
		err=i8042_command(NULL,I8042_CMD_KBD_ENABLE);// tom +
	if(!err)
		printk(KERN_INFO"i8042_command I8042_CMD_KBD_ENABLE OK\n");
	return 0;
}


int elantech_detect(struct psmouse *psmouse, int set_properties)
{

	struct ps2dev	*ps2dev = &psmouse->ps2dev;
	unsigned char	param[3];
	int		i;

	printk(KERN_INFO "2.6.2X-Elan-touchpad-2009-03-09\n");
	printk(KERN_DEBUG "+elantech_detect\n");

	for(i=0;i<3;i++)
	{
		if(!ps2_command(ps2dev,  NULL, PSMOUSE_CMD_DISABLE))
			break;
	}

	for(i=0;i<3;i++)
	{
			param[0]=0;
			param[1]=0;
			param[2]=0;
			if(!ps2_command(ps2dev,  param, 0x02ff))
			{
			printk(KERN_INFO "elantech.c: PSMOUSE_CMD_RESET_BAT  param[0]=%x param[1]=%x param[2]=%x\n",param[0],param[1],param[2]);
				break;
			}
	} //FF   ->reply FA AA 00

	if(i >= 3)
		printk(KERN_INFO "elantech.c: PSMOUSE_CMD_RESET_BAT fail  param[0]=%x param[1]=%x param[2]=%x\n",param[0],param[1],param[2]);



        for(i=0;i<3;i++)
	{
		if(!ps2_command(ps2dev,  NULL, PSMOUSE_CMD_SETSCALE11))
			break;
	}

	for(i=0;i<3;i++)
	{
		if(!ps2_command(ps2dev,  NULL, PSMOUSE_CMD_SETSCALE11))
			break;
	}

	for(i=0;i<3;i++)
	{
		if(!ps2_command(ps2dev,  NULL, PSMOUSE_CMD_SETSCALE11))
			break;
	}

	for(i=0;i<3;i++)
	{
		if(!ps2_command(ps2dev, param, PSMOUSE_CMD_GETINFO))
			break;
	}


	if ((param[0] != 0x3c) || (param[1] != 0x03) || (param[2] != 0xc8))
	{
		pr_info("elantech.c: unexpected magic knock result 0x%02x, 0x%02x, 0x%02x.\n",
			param[0], param[1], param[2]);
		return -1;
	}

	printk(KERN_INFO "elantech.c: param[0]=%x param[1]=%x param[2]=%x\n",param[0],param[1],param[2]);

	/* Why does the Elantech Windows driver try this?
	 * For now just report it and see if it makes sense
	 * when more people use this driver
	 */

		if (set_properties) {
			psmouse->vendor = "Elantech";
			psmouse->reconnect =elantech_reconnect;

			//psmouse->name = "Touchpad";
			//printk(KERN_DEBUG "-elantech_detect Ok\n");
	        }
	     //printk(KERN_DEBUG "-elantech_detect\n");
	return 0;

}

/*
 * Initialize the touchpad and create sysfs entries
 */
int elantech_init(struct psmouse *psmouse,int set_properties)
{

	struct elantech_data 	*priv;
	unsigned char	        param[3];
	unsigned char 		d;
	int			error,err,i,j,m;
	//printk(KERN_DEBUG "+elantech_init\n");

	priv = kzalloc(sizeof(struct elantech_data), GFP_KERNEL);
	psmouse->private = priv;

	if (!priv)
		return -1;

	error=err= -1;
        for(j=0;j<3;j++){
		for(i=0;i<4;i++){
			if(!ps2_command(&psmouse->ps2dev, NULL,PSMOUSE_CMD_SETSCALE11))
				break;

		}


		for (i = 6; i >= 0; i -= 2) {
		     d = (0x01 >> i) & 3;
		     for(m=0;m<4;m++){
			if (!ps2_command(&psmouse->ps2dev, &d, PSMOUSE_CMD_SETRES))
				break;
		     }
		}

		if(!ps2_command(&psmouse->ps2dev, param, PSMOUSE_CMD_GETINFO)){
			printk(KERN_DEBUG "+elantech_init param[0]=%x param[1]=%x param[2]=%x\n",param[0],param[1],param[2]);
			break;
		}else{
			for(i=0;i<3;i++){
				if(!ps2_command(&psmouse->ps2dev,  param, PSMOUSE_CMD_RESET_BAT)){
					printk(KERN_INFO "elantech_init: param[0]=%x param[1]=%x param[2]=%x\n",param[0],param[1],param[2]);
					break;
				}
			} //FF   ->reply FA AA 00
			printk(KERN_DEBUG "+elantech_init fw version error\n");

		}
	}

	if(param[0]==0x02 && param[1]==0x00 && param[2] == 0x22){
	        err=elantech_set_4byte_defaults_EF013(psmouse);
		if(err==0){
			printk(KERN_ERR "elantech.c:elantech_init 4 EF013\n");
			psmouse->protocol_handler = elantech_process_4byte_EF013;
			psmouse->pktsize = 4;
			psmouse->disconnect = elantech_disconnect;
			psmouse->name = "Elantech Touchpad 4byte mode EF013";

		}
	}
	else if(param[0]==0x02 && param[1]==0x06 && param[2] == 0x00){
	        err=elantech_set_4byte_defaults_EF019(psmouse);
		if(err==0){
			printk(KERN_ERR "elantech.c:elantech_init 4 EF019\n");
			psmouse->protocol_handler = elantech_process_4byte_EF019;
			psmouse->pktsize = 4;
			psmouse->disconnect = elantech_disconnect;
			psmouse->name = "Elantech Touchpad 4byte mode EF019";

		}
	}
	else if(param[0]==0x02 && param[1]==0x00 && param[2] == 0x30){
	        err=elantech_set_6byte_defaults_EF113(psmouse);
		if(err==0){
			printk(KERN_ERR "elantech.c:elantech_init 6 EF113\n");
			psmouse->protocol_handler = elantech_process_6byte_EF113;
			psmouse->pktsize = 6;
			psmouse->disconnect = elantech_disconnect;
			psmouse->name = "Elantech Touchpad 6byte mode EF113";

		}
	}
	else if(param[0]==0x02 && param[1]==0x08 && param[2] == 0x00){
	        err=elantech_set_6byte_defaults_EF023(psmouse);
		if(err==0){
			printk(KERN_ERR "elantech.c:elantech_init 6 EF023\n");
			psmouse->protocol_handler = elantech_process_6byte_EF023;
			psmouse->pktsize = 6;
			psmouse->disconnect = elantech_disconnect;
			psmouse->reconnect =elantech_reconnect_EF023;
			psmouse->name = "Elantech Touchpad 6byte mode EF023";

		}
	}
	else if(param[0]==0x02 && param[1]==0x08){
	        err=elantech_set_6byte_defaults_EF123(psmouse);
		if(err==0){
			printk(KERN_ERR "elantech.c:elantech_init 6 EF123\n");
			psmouse->protocol_handler = elantech_process_6byte_EF123;
			psmouse->pktsize = 6;
			psmouse->disconnect = elantech_disconnect;
			psmouse->name = "Elantech Touchpad 6byte mode EF123";

		}
	}
	else if(param[0]==0x02 && param[1]==0x0B && param[2] == 0x00){
	        err=elantech_set_6byte_defaults_EF215(psmouse);
		if(err==0){
			printk(KERN_ERR "elantech.c:elantech_init 6 EF215\n");
			psmouse->protocol_handler = elantech_process_6byte_EF215;
			psmouse->pktsize = 6;
			psmouse->disconnect = elantech_disconnect;
			psmouse->name = "Elantech Touchpad 6byte mode EF215";

		}
	}
	else if(param[0]==0x04 && param[1]==0x01){
	        err=elantech_set_6byte_defaults_Scroll_EF051(psmouse);
		if(err==0){
			printk(KERN_ERR "elantech.c:elantech_init 6 Scroll_EF051\n");
			psmouse->protocol_handler = elantech_process_6byte_Scroll_EF051;
			psmouse->pktsize = 6;
			psmouse->disconnect = elantech_disconnect;
			psmouse->name = "Elantech Touchpad 6byte mode Scroll_EF051";

		}
	}
	else if(param[0]==0x04 && param[1]==0x02){
	        err=elantech_set_6byte_defaults_EF051(psmouse);
		if(err==0){
			printk(KERN_ERR "elantech.c:elantech_init 6 EF051\n");
			psmouse->protocol_handler = elantech_process_6byte_EF051;
			psmouse->pktsize = 6;
			psmouse->disconnect = elantech_disconnect;
			psmouse->name = "Elantech Touchpad 6byte mode EF051";

		}
	}
        else {
              printk(KERN_DEBUG "elantech -- don't find fw version\n");
              err=1;
        }
	//EF_023_DEBUG=0;

        //err=1;//chris
	if(err){
		for(i=0;i<3;i++){
			if(!ps2_command(&psmouse->ps2dev,  param, PSMOUSE_CMD_RESET_BAT)){
				printk(KERN_ERR "elantech.c:err=%d param[0]=%x param[1]=%x param[2]=%x\n",err,param[0],param[1],param[2]);
				break;
			}
			else
				printk(KERN_ERR "elantech.c:err=%d FF command fail\n",err);
		}

	}else{
		if(set_properties)
			error = sysfs_create_group(&psmouse->ps2dev.serio->dev.kobj,&elantech_attr_group);
	}

	if ((error && set_properties)||err) {
		for(i=0;i<3;i++){
			if(!ps2_command(&psmouse->ps2dev,  param, PSMOUSE_CMD_RESET_BAT)){
				printk(KERN_ERR "elantech.c:error=%d param[0]=%x param[1]=%x param[2]=%x\n",error,param[0],param[1],param[2]);
				break;
			}
			else
				printk(KERN_ERR "elantech.c:error=%d FF command fail\n",error);
		}


                //printk(KERN_ERR "elantech.c: failed to create sysfs attributes, error: %d\n",error);
		psmouse->private=NULL;
		kfree(priv);
		return -1;
	}


	//printk(KERN_DEBUG "-elantech_init\n");
	return 0;
}

