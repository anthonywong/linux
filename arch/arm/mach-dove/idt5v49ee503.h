/* IDT clock configurations header file */

#ifndef _INC_IDT5V49EE503_H
#define _INC_IDT5V49EE503_H

#define IDT5V49EE503_NUM_BUS	2	/* Number of supported i2c buses (max) */

/* PLL ID inside the clock chip */
typedef enum 
{
	IDT_PLL_0 = 0,
	IDT_PLL_1 = 1,
	IDT_PLL_2 = 2,
	IDT_PLL_3 = 3,
	IDT_PLL_NUM

} idt_clock_id_t;

/* Output pin ID where the PLL is connected */
typedef enum 
{
	IDT_OUT_ID_0 = 0,
	IDT_OUT_ID_1 = 1,
	IDT_OUT_ID_2 = 2,
	IDT_OUT_ID_3 = 3,
	IDT_OUT_ID_6 = 6,
	IDT_OUT_ID_INV

} idt_output_id_t;

/* Clock configuration ID */
typedef enum
{
	IDT_CLK_CFG_0 = 0,
	IDT_CLK_CFG_1 = 1,
	IDT_CLK_CFG_2 = 2,
	IDT_CLK_CFG_3 = 3,
	IDT_CLK_CFG_4 = 4,
	IDT_CLK_CFG_5 = 5,
	IDT_CLK_CFG_NUM

} idt_clock_cfg_id_t;

/* Clock output divider */
typedef enum
{
	IDT_OUT_DIV1 = 0,
	IDT_OUT_DIV2,
	IDT_OUT_DIV3,
	IDT_OUT_DIV6,
	IDT_OUT_DIV_NUM,
	IDT_OUT_DIV_INV = IDT_OUT_DIV_NUM

} idt_out_div_t;


typedef struct _idt_clock_cfg_t
{
	int			bus_id;		/* i2c bus/adapter ID to use with i2c_get_adapter() */\
	int			clk_src_clkin;  /* clock source is CLKIN(0) or XTAL/REFIN(0) */
	idt_clock_id_t		clock_id;	/* clock ID to configure (PLL0-PLL3) */
	idt_output_id_t		out_id;		/* output pin where PLL + output divider are connected */
	idt_clock_cfg_id_t	cfg_id;		/* clock internal configuration ID */
	unsigned int		cfg_act;	/* when !=0 this configuration is not just stored */
						/* in the clock EEPROM, but also set as active  and */
						/* PLL + output are enabled */
} idt_clock_cfg_t;
#if 0
int idt5v49ee503_set (unsigned long freq_set, 
		      idt_clock_cfg_t *clock_cfg);

int idt5v49ee503_set_act_cfg (idt_clock_cfg_t *clock_cfg);

int idt5v49ee503_save_regs (int bus_id);
int idt5v49ee503_restore_regs (int bus_id);

int idt5v49ee503_pll_enable (int            bus_id,
			     idt_clock_id_t pll_id,
			     int            enable);

int idt5v49ee503_out_enable (int             bus_id,
			     idt_output_id_t out_id,
			     int             enable);

int idt5v49ee503_sw_ctrl (int  bus_id,
			  int  sw_ctl);
#endif

#endif /* _INC_IDT5V49EE503_H */
