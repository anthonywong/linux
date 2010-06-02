/*
 *  linux/arch/arm/mach-dove/idt5v49ee503.c
 *  EEPROM programmable clock generator (IDT5V49EE503) driver
 */

/* TODO: move to Dove's clock.c ? */

#include <linux/i2c.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <asm/string.h>

#include "idt5v49ee503.h"

#define IDT5V49EE503_BUS_ADDR			0x6A /* 7'b1101010 */

#define IDT5V49EE503_SW_MODE_CTL_REG		0x0
#define IDT5V49EE503_CFG_SEL_REG		0x1
#define IDT5V49EE503_OUT_SUSPEND_REG		0x3
#define IDT5V49EE503_PLL_SUSPEND_REG		0x4
#define IDT5V49EE503_XTCLKSEL_REG		0x5
#define IDT5V49EE503_XTAL_REG			0x7

/********************/
/* LOCLA DATA TYPES */
/********************/

/* Clock source IDs (MUX names) */
typedef enum
{
	IDT_CLK_SRC_ID_PRIM = 0,
	IDT_CLK_SRC_ID_0,
	IDT_CLK_SRC_ID_1,
	IDT_CLK_SRC_ID_2,
	IDT_CLK_SRC_ID_3,
	IDT_CLK_SRC_ID_6,
	IDT_CLK_SRC_ID_NUM

} idt_clock_src_id_t;

/* Clock sources (MUX selection) */
typedef enum
{
	IDT_CLK_SRC_DIV1 = 0x0,
	IDT_CLK_SRC_DIV3 = 0x1,
	IDT_CLK_SRC_REFIN= 0x2,
	IDT_CLK_SRC_RSRVD= 0x3,
	IDT_CLK_SRC_PLL0 = 0x4,
	IDT_CLK_SRC_PLL1 = 0x5,
	IDT_CLK_SRC_PLL2 = 0x6,
	IDT_CLK_SRC_PLL3 = 0x7

} idt_clock_src_t;

/* Clock sources (MUX selection) */
typedef enum
{
	IDT_PRM_CLK_CRYSTAL = 0,
	IDT_PRM_CLK_CLKIN   = 1
} idt_prim_clock_src_t;

/* Clock registers for variour PLL parameters */
typedef struct _idt_pll_reg_set_t
{
	unsigned char N_lreg;	/* feedback divider LSB N[7:0]  */
	unsigned char N_mreg;	/* feedback divider MSB N[11:8] */
	unsigned char N_mlshft;	/* N[11:8] nibble left-shift value */
	unsigned char D_reg;	/* reference divider D[6:0]  */

} idt_pll_reg_set_t;

/* Clock registers for MUX source configuration */
typedef struct _idt_src_reg_set_t
{
	unsigned char	lreg;		/* SRCx[1:0] or SRCx[2:0] register address (LSB) */
	unsigned char	lshift;		/* SRCx[1:0] or SRCx[2:0] data shift */
	unsigned char   lmask;		/* SRCx[1:0] or SRCx[2:0] data mask (width) */
	unsigned char	mreg;		/* SRCx[2] register address (MSB, only is separated from [1:0]) */
	unsigned char	mshift;		/* SRCx[2] data shift (MSB, only is separated from [1:0]) */
} idt_src_reg_set_t;

/* Fixed frequencies table entry */
typedef struct _idt_freq_ten_t
{
	unsigned long	f_out;	/* output frequency Hz */
	unsigned long 	f_in;	/* input (reference) freqency Hz */
	unsigned int	n;	/* N (feedback divider) is 12 bits wide in HW */
	unsigned int	d;	/* D (reference divider) is 7 bits wide in HW */
	unsigned int	odiv;	/* ODIV, total output divider 7 and 1 bits in HW */
	
} idt_freq_ten_t;

typedef struct _idt_drv_info_t
{
	struct i2c_adapter 	*i2c_ad;
	struct i2c_client 	*i2c_cl;
	int			valid;

} idt_drv_info_t;

/***************/
/* STATIC DATA */
/***************/

/* Registers addresses in clock programming tables */
static idt_pll_reg_set_t idt5v49ee503_pll_regs[IDT_CLK_CFG_NUM][IDT_PLL_NUM] = {
	/*  N[7:0]  N[11:8]  Nsh  D[6:0] */
	{ { 0x18,   0x1C,    0,   0x10 }, /* PLL0 */
	  { 0x30,   0x34,    0,   0x28 }, /* PLL1 */
	  { 0x48,   0x4C,    0,   0x40 }, /* PLL2 */
	  { 0x64,   0x34,    4,   0x5C }  /* PLL3 */
	}, /* CFG0 */
	{ { 0x19,   0x1D,    0,   0x11 }, /* PLL0 */
	  { 0x31,   0x35,    0,   0x29 }, /* PLL1 */
	  { 0x49,   0x4D,    0,   0x41 }, /* PLL2 */
	  { 0x65,   0x35,    4,   0x5D }  /* PLL3 */
	}, /* CFG1 */
	{ { 0x1A,   0x1E,    0,   0x12 }, /* PLL0 */
	  { 0x32,   0x36,    0,   0x2A }, /* PLL1 */
	  { 0x4A,   0x4E,    0,   0x42 }, /* PLL2 */
	  { 0x66,   0x36,    4,   0x5E }  /* PLL3 */
	}, /* CFG2 */
	{ { 0x1B,   0x1F,    0,   0x13 }, /* PLL0 */
	  { 0x33,   0x37,    0,   0x2B }, /* PLL1 */
	  { 0x4B,   0x4F,    0,   0x43 }, /* PLL2 */
	  { 0x67,   0x37,    4,   0x5F }  /* PLL3 */
	}, /* CFG3 */
	{ { 0x16,   0x20,    0,   0x14 }, /* PLL0 */
	  { 0x2E,   0x38,    0,   0x2C }, /* PLL1 */
	  { 0x46,   0x50,    0,   0x44 }, /* PLL2 */
	  { 0x62,   0x38,    4,   0x60 }  /* PLL3 */
	}, /* CFG4 */
	{ { 0x17,   0x21,    0,   0x15 }, /* PLL0 */
	  { 0x2F,   0x39,    0,   0x2D }, /* PLL1 */
	  { 0x47,   0x51,    0,   0x45 }, /* PLL2 */
	  { 0x63,   0x39,    4,   0x61 }  /* PLL3 */
	}  /* CFG5 */
};

/* register addresses for output dividers */
static unsigned int idt5v49ee503_div_regs[IDT_CLK_CFG_NUM][IDT_OUT_DIV_NUM] = {
	/* DIV1  DIV2  DIV3  DIV6 */
	{  0x88, 0x90, 0x94, 0xA8 }, /* CFG0 */
	{  0x89, 0x91, 0x95, 0xA9 }, /* CFG1 */
	{  0x8A, 0x92, 0x96, 0xAA }, /* CFG2 */
	{  0x8B, 0x93, 0x97, 0xAB }, /* CFG3 */
	{  0x8C, 0x8E, 0x98, 0xA6 }, /* CFG4 */
	{  0x8D, 0x8F, 0x99, 0xA7 }  /* CFG5 */
};

/* Registers addresses for source selectors (0xFF - not valid) */
static idt_src_reg_set_t idt5v49ee503_src_regs[IDT_CLK_CFG_NUM][IDT_CLK_SRC_ID_NUM] = {
	/*  lreg  lshift  lmask mreg  mshift */
	{ { 0xC0, 0x0,    0x3,  0xFF, 0x0  }, /* IDT_CLK_SRC_ID_PRIM */
	  { 0xC0, 0x4,    0x3,  0xFF, 0x0  }, /* IDT_CLK_SRC_ID_0 */
	  { 0xC0, 0x6,    0x3,  0xC4, 0x0  }, /* IDT_CLK_SRC_ID_1 */
	  { 0xC4, 0x1,    0x7,  0xFF, 0x0  }, /* IDT_CLK_SRC_ID_2 */
	  { 0xC4, 0x4,    0x7,  0xFF, 0x0  }, /* IDT_CLK_SRC_ID_3 */
	  { 0xCC, 0x5,    0x7,  0xFF, 0x0  }, /* IDT_CLK_SRC_ID_6 */
	}, /* CFG0 */
	{ { 0xC1, 0x0,    0x3,  0xFF, 0x0  }, /* IDT_CLK_SRC_ID_PRIM */
	  { 0xC1, 0x4,    0x3,  0xFF, 0x0  }, /* IDT_CLK_SRC_ID_0 */
	  { 0xC1, 0x6,    0x3,  0xC5, 0x0  }, /* IDT_CLK_SRC_ID_1 */
	  { 0xC5, 0x1,    0x7,  0xFF, 0x0  }, /* IDT_CLK_SRC_ID_2 */
	  { 0xC5, 0x4,    0x7,  0xFF, 0x0  }, /* IDT_CLK_SRC_ID_3 */
	  { 0xCD, 0x5,    0x7,  0xFF, 0x0  }, /* IDT_CLK_SRC_ID_6 */
	}, /* CFG1 */
	{ { 0xC2, 0x0,    0x3,  0xFF, 0x0  }, /* IDT_CLK_SRC_ID_PRIM */
	  { 0xC2, 0x4,    0x3,  0xFF, 0x0  }, /* IDT_CLK_SRC_ID_0 */
	  { 0xC2, 0x6,    0x3,  0xC6, 0x0  }, /* IDT_CLK_SRC_ID_1 */
	  { 0xC6, 0x1,    0x7,  0xFF, 0x0  }, /* IDT_CLK_SRC_ID_2 */
	  { 0xC6, 0x4,    0x7,  0xFF, 0x0  }, /* IDT_CLK_SRC_ID_3 */
	  { 0xCE, 0x5,    0x7,  0xFF, 0x0  }, /* IDT_CLK_SRC_ID_6 */
	}, /* CFG2 */
	{ { 0xC3, 0x0,    0x3,  0xFF, 0x0  }, /* IDT_CLK_SRC_ID_PRIM */
	  { 0xC3, 0x4,    0x3,  0xFF, 0x0  }, /* IDT_CLK_SRC_ID_0 */
	  { 0xC3, 0x6,    0x3,  0xC7, 0x0  }, /* IDT_CLK_SRC_ID_1 */
	  { 0xC7, 0x1,    0x7,  0xFF, 0x0  }, /* IDT_CLK_SRC_ID_2 */
	  { 0xC7, 0x4,    0x7,  0xFF, 0x0  }, /* IDT_CLK_SRC_ID_3 */
	  { 0xCF, 0x5,    0x7,  0xFF, 0x0  }, /* IDT_CLK_SRC_ID_6 */
	}, /* CFG3 */
	{ { 0xBE, 0x0,    0x3,  0xFF, 0x0  }, /* IDT_CLK_SRC_ID_PRIM */
	  { 0xBE, 0x4,    0x3,  0xFF, 0x0  }, /* IDT_CLK_SRC_ID_0 */
	  { 0xBE, 0x6,    0x3,  0xC8, 0x0  }, /* IDT_CLK_SRC_ID_1 */
	  { 0xC8, 0x1,    0x7,  0xFF, 0x0  }, /* IDT_CLK_SRC_ID_2 */
	  { 0xC8, 0x4,    0x7,  0xFF, 0x0  }, /* IDT_CLK_SRC_ID_3 */
	  { 0xCA, 0x5,    0x7,  0xFF, 0x0  }, /* IDT_CLK_SRC_ID_6 */
	}, /* CFG4 */
	{ { 0xBF, 0x0,    0x3,  0xFF, 0x0  }, /* IDT_CLK_SRC_ID_PRIM */
	  { 0xBF, 0x4,    0x3,  0xFF, 0x0  }, /* IDT_CLK_SRC_ID_0 */
	  { 0xBF, 0x6,    0x3,  0xC9, 0x0  }, /* IDT_CLK_SRC_ID_1 */
	  { 0xC9, 0x1,    0x7,  0xFF, 0x0  }, /* IDT_CLK_SRC_ID_2 */
	  { 0xC9, 0x4,    0x7,  0xFF, 0x0  }, /* IDT_CLK_SRC_ID_3 */
	  { 0xCB, 0x5,    0x7,  0xFF, 0x0  }, /* IDT_CLK_SRC_ID_6 */
	}  /* CFG5 */
};

/* The following equation is used for parameters computation:
   Fout = (Fin * (M/D))/ODIV
   Where for PLL0:
	M = 2 * N + A + 1 (for A > 0)
	M = 2 * N (for A = 0)
   For PLL1, PLL2, PLL3:
	M = N
*/
/* Supported fixed frequencies table */
static idt_freq_ten_t idt5v49ee503_freq_tbl[] = {
	/*f_out      f_in      n    d   odiv */
	{ 148500000, 25000000, 891, 25,  6},
	{  64109000, 25000000, 877, 19, 18},
	{ 108883000, 25000000, 527, 11, 11},
	{ 160963000, 25000000, 734, 19,  6},
	{ 136358000, 25000000, 480, 19,  8},
	{         0,        0,   0,  0,  0},
};

/* local driver information */
static idt_drv_info_t idt5v49ee503_drv[IDT5V49EE503_NUM_BUS] = {{0}, {0}};

/****************************************************************************** 
   Write I2C register
   Input:
 	driver - driver control structure
 	addr   - register address
 	val    - register value
   Output:
      	NONE
 
   Returns:
	0 on success or error value
*******************************************************************************/
static int idt5v49ee503_write_reg (idt_drv_info_t *driver,
				   unsigned char  addr,
				   unsigned char  val)
{
 	unsigned char buf[3] = {0, addr, val}; /* fist byte is command (0) */
	
	return i2c_master_send(driver->i2c_cl, buf, 3);

} /* end of idt5v49ee503_read_reg */

/****************************************************************************** 
   Read I2C register
   Input:
 	driver - driver control structure
 	addr   - register address
 	val    - register value
   Output:
      	NONE
 
   Returns:
	0 on success or error value
*******************************************************************************/
static int idt5v49ee503_read_reg (idt_drv_info_t *driver,
				  unsigned char  addr,
				  unsigned char  *val)
{
	int           rval;
	unsigned char buf[3] = {0, addr, 0}; /* fist byte for WRITE is command (0)
	                                        or ID byte for READ (ignored) */
	
	rval = i2c_master_send(driver->i2c_cl, buf, 2);
	if (rval == 0)
	{	
		rval = i2c_master_recv(driver->i2c_cl, buf, 2);
		*val = buf[1];
	}

	return rval;

} /* end of idt5v49ee503_read_reg */

/****************************************************************************** 
   Read, modify and write back I2C register
   Input:
 	driver - driver control structure
 	addr   - register address
 	fval   - register's field value
 	shift  - number of bits to shift the field
 	mask   - the field's mask (before shifting)
   Output:
      	NONE
 
   Returns:
	0 on success or error value
*******************************************************************************/
static int idt5v49ee503_mod_reg (idt_drv_info_t *driver,
				 unsigned char  addr,
				 unsigned char  fval,
				 unsigned char  shift,
				 unsigned char	mask)
{
	int           rval;
	unsigned char val;
	
	if ((rval = idt5v49ee503_read_reg(driver, addr, &val)) != 0)
		return rval;

	val &= ~(mask << shift); 
	val |= (fval & mask) <<	shift;

	return idt5v49ee503_write_reg(driver, addr, val);

} /* end of idt5v49ee503_mod_reg */


/****************************************************************************** 
   Connect PLL to specific output pin and output divider
   Input:
 	driver - driver control structure
 	clock_cfg - clock configuration
   Output:
      	NONE
 
   Returns:
	Output divider ID
*******************************************************************************/
static idt_out_div_t idt5v49ee503_setup_output (idt_drv_info_t	*driver,
						idt_clock_cfg_t	*clock_cfg)
{
	idt_clock_src_id_t	clk_src_id[2]; /* maximum 2 clock sources to config */
	idt_clock_src_t		clk_src[2];
	idt_out_div_t		divider; /* currently only one level division support */
	idt_src_reg_set_t	*reg_set = idt5v49ee503_src_regs[clock_cfg->cfg_id];
	int			i;

	if (driver == NULL)
		return IDT_OUT_DIV_INV;

	/* second source MUX will usually not be used (excepting OUT0) */
	clk_src_id[1] = IDT_OUT_ID_INV;
	
	/* find out MUX ID and divider based on output pin - see datasheet page 2 */
	switch (clock_cfg->out_id)
	{
	case IDT_OUT_ID_0:
		clk_src_id[0] = IDT_CLK_SRC_ID_1; 
		clk_src_id[1] = IDT_CLK_SRC_ID_0;
		clk_src[1] = IDT_CLK_SRC_DIV1; /* always use DIV1 for OUT0 */
		divider = IDT_OUT_DIV1;
		break;

	case IDT_OUT_ID_1:
		clk_src_id[0] = IDT_CLK_SRC_ID_1;
		divider = IDT_OUT_DIV1;
		break;

	case IDT_OUT_ID_2:
		clk_src_id[0] = IDT_CLK_SRC_ID_2;
		divider = IDT_OUT_DIV2;
		break;

	case IDT_OUT_ID_3:
		clk_src_id[0] = IDT_CLK_SRC_ID_3;
		divider = IDT_OUT_DIV3;
		break;

	case IDT_OUT_ID_6:
		if (clock_cfg->clock_id != IDT_PLL_1)
			return IDT_OUT_DIV_INV;
		clk_src_id[0] = IDT_CLK_SRC_ID_6;
		divider = IDT_OUT_DIV6;
		break;
	default:
		return IDT_OUT_DIV_INV;

	} /* switch(clock_cfg->out_id) */

	/* find out MUX input selection based on PLL ID */
	switch (clock_cfg->clock_id)
	{
	case IDT_PLL_0:
		clk_src[0] = IDT_CLK_SRC_PLL0;
		break;

	case IDT_PLL_1:
		clk_src[0] = IDT_CLK_SRC_PLL1;
		break;

	case IDT_PLL_2:
		clk_src[0] = IDT_CLK_SRC_PLL2;
		break;

	case IDT_PLL_3:
		clk_src[0] = IDT_CLK_SRC_PLL3;
		break;

	default:
		return IDT_OUT_DIV_INV;

	} /* switch (clock_cfg->clock_id) */

	/* Write SRCx MUX configuration to clock registers */
	for (i = 0; i < 2; i++)
	{		
		if (clk_src_id[i] == IDT_OUT_ID_INV)
			continue;

		if (idt5v49ee503_mod_reg(driver,
				         reg_set[clk_src_id[i]].lreg,
				         clk_src[i],
				         reg_set[clk_src_id[i]].lshift,
				         reg_set[clk_src_id[i]].lmask) != 0)
			return IDT_OUT_DIV_INV;

		/* MSB bit is located in other than LSB register? */
		if (reg_set[clk_src_id[i]].mreg != 0xFF)
		{
			if (idt5v49ee503_mod_reg(driver,
						 reg_set[clk_src_id[i]].mreg,
						 clk_src[i],
						 reg_set[clk_src_id[i]].mshift,
						 0x1) != 0)
				return IDT_OUT_DIV_INV;
		}
	}	

	/* Configure primary clock source */
	if (idt5v49ee503_mod_reg(driver,
				 reg_set[IDT_CLK_SRC_ID_PRIM].lreg,
				 clock_cfg->clk_src_clkin != 0 ? 
					IDT_PRM_CLK_CLKIN : IDT_PRM_CLK_CRYSTAL,
				 reg_set[IDT_CLK_SRC_ID_PRIM].lshift,
				 reg_set[IDT_CLK_SRC_ID_PRIM].lmask) != 0)
		return IDT_OUT_DIV_INV;

	return divider;

} /* end of idt5v49ee503_setup_output */

/****************************************************************************** 
   Initilalize driver data
   Input:
 	bus_id  - bus ID
   Output:
      	NONE
 
   Returns:
	0	- success,
*******************************************************************************/
int idt5v49ee503_init (int bus_id)
{
	struct i2c_board_info 	i2c_brd;
	
	idt5v49ee503_drv[bus_id].i2c_ad = i2c_get_adapter(bus_id);
	if (idt5v49ee503_drv[bus_id].i2c_ad == NULL)
	{	
		printk(KERN_ERR "idt5v49ee503: failed to obtain "
				"adapter[%d] handler\n", bus_id);
		return -ENODEV;
	}

	/* configure board values for getting slave device handler */
	memset(&i2c_brd, 0, sizeof(struct i2c_board_info));
	i2c_brd.addr          = IDT5V49EE503_BUS_ADDR;
	i2c_brd.platform_data = "ext_vga_clock";
	strlcpy(i2c_brd.type, "idt5v49ee503", I2C_NAME_SIZE);

	idt5v49ee503_drv[bus_id].i2c_cl = 
		i2c_new_device(idt5v49ee503_drv[bus_id].i2c_ad, &i2c_brd);
	if (idt5v49ee503_drv[bus_id].i2c_cl == NULL)
	{	
		printk(KERN_ERR "idt5v49ee503: failed to obtain "
				"device handler\n");
		return -ENODEV;
	}

	idt5v49ee503_drv[bus_id].valid = 1;

	return 0;

} /* end of idt5v49ee503_init */

/****************************************************************************** 
   Write PLL parameters to clock HW according to specific frequency index
   Input:
 	freq_idx  - index to fixed frequencies table
 	clock_cfg - clock configuration
   Output:
      	NONE
 
   Returns:
	0	- success,
*******************************************************************************/
static int idt5v49ee503_write_cfg (unsigned int    freq_idx,
				   idt_clock_cfg_t *clock_cfg)
{
	idt_drv_info_t		*drv = NULL;
	idt_freq_ten_t		*freq = &idt5v49ee503_freq_tbl[freq_idx];
	idt_out_div_t		div_id; /* output divider */
	idt_pll_reg_set_t 	*pll_regs = idt5v49ee503_pll_regs[clock_cfg->cfg_id];
	unsigned int 		*div_regs = idt5v49ee503_div_regs[clock_cfg->cfg_id];
	unsigned int		div_val, n_lsb, n_msb = 0;

	/* Sanity checks */
	if (clock_cfg->bus_id >= IDT5V49EE503_NUM_BUS)
		return -EINVAL;

	drv = &idt5v49ee503_drv[clock_cfg->bus_id];
	if (drv->valid != 1)
		return -EPERM;

	/* Setup SRCx MUXes and primary clock source first */
	if ((div_id = idt5v49ee503_setup_output(drv, clock_cfg)) == IDT_OUT_DIV_INV)
		return -EIO;

	/* Configure clock parameters */
	if (idt5v49ee503_write_reg(drv,
				 pll_regs[clock_cfg->clock_id].D_reg,
				 freq->d) != 0)
		return -EIO;

	if (clock_cfg->clock_id == IDT_PLL_0)
	{ /* M = 2 * N (A=0) */
		n_lsb = (freq->n >> 1) & 0xFF;
		n_msb = (freq->n >> 9) & 0xF;
	} /* M = 2 * N (A=0) */
	else
	{ /* M =  N */
		n_lsb = freq->n & 0xFF;
		n_msb = (freq->n >> 8) & 0xF;
	} /* M =  N */

	if (idt5v49ee503_write_reg(drv,
				   pll_regs[clock_cfg->clock_id].N_lreg,
				   n_lsb) != 0)
		return -EIO;

	/* MSP part of N parameter shares register space with others */
	if (idt5v49ee503_mod_reg(drv,
				 pll_regs[clock_cfg->clock_id].N_mreg,
				 n_msb,
				 pll_regs[clock_cfg->clock_id].N_mlshft,
				 0xF) != 0)
		return -EIO;
	
	/* Configure output divider */
	if (freq->odiv == 1)
		div_val = 0xFF;
	else if (freq->odiv == 2)
		div_val = 0xFF;
	else /* ODIV = (Q[6:0] + 2) * 2 */
		div_val = (freq->odiv >> 1) - 2 + 0x90; 

	if (idt5v49ee503_write_reg(drv,
				   div_regs[div_id],
				   div_val) != 0)
		return -EIO;

	return 0;

} /* end of idt5v49ee503_write_cfg */

/****************************************************************************** 
   activate specific clock configuration
   Input:
	clock_cfg   - clock configuration
   Output:
      	NONE
 
   Returns:
 	0      - success,
 	-EPERM - the clock is not initialized
 	-EINVAL - bad configuration ID
*******************************************************************************/
int idt5v49ee503_set_act_cfg (idt_clock_cfg_t *clock_cfg)
{
	idt_drv_info_t	*drv = NULL;
	
	if (clock_cfg->bus_id >= IDT5V49EE503_NUM_BUS)
		return -EINVAL;

	drv = &idt5v49ee503_drv[clock_cfg->bus_id];
	if (drv->valid != 1)
		return -EPERM;

	if (clock_cfg->cfg_id >= IDT_CLK_CFG_NUM)
		return -EINVAL;

	/* Activate configuration */
	return idt5v49ee503_write_reg(drv,
				      IDT5V49EE503_CFG_SEL_REG,
				      clock_cfg->cfg_id);

} /* end of idt5v49ee503_act_cfg */

/****************************************************************************** 
   Save all clock registers to EEPROM
   Input:
 	bus_id  - bus ID
   Output:
      	NONE
 
   Returns:
 	0      - success,
*******************************************************************************/
int idt5v49ee503_save_regs (int bus_id)
{
	/* PROGSAVE command (1) */
	char		cmd = 0x1;
	idt_drv_info_t	*drv = NULL;

	if (bus_id >= IDT5V49EE503_NUM_BUS)
		return -EINVAL;

	drv = &idt5v49ee503_drv[bus_id];
	if (drv->valid != 1)
		return -EPERM;

	return i2c_master_send(drv->i2c_cl, &cmd, 1);

} /* end of idt5v49ee503_save_regs */

/****************************************************************************** 
   Save all clock registers to EEPROM
   Input:
 	bus_id  - bus ID
   Output:
      	NONE
 
   Returns:
 	0      - success,
*******************************************************************************/
int idt5v49ee503_restore_regs (int bus_id)
{
	/* PROGRESTORE command (2) */
	char		cmd = 0x2;
	idt_drv_info_t	*drv = NULL;

	if (bus_id >= IDT5V49EE503_NUM_BUS)
		return -EINVAL;

	drv = &idt5v49ee503_drv[bus_id];
	if (drv->valid != 1)
		return -EPERM;

	return i2c_master_send(drv->i2c_cl, &cmd, 1);

} /* end of idt5v49ee503_save_regs */

/****************************************************************************** 
   Enable or disable PLL
   Input:
 	bus_id  - bus ID
 	pll_id  - PLL ID
 	enable  - 1 for enable, 0 for disable
   Output:
      	NONE
 
   Returns:
 	0      - success,
*******************************************************************************/
int idt5v49ee503_pll_enable (int            bus_id,
			     idt_clock_id_t pll_id,
			     int            enable)
{
	idt_drv_info_t	*drv = NULL;

	if (bus_id >= IDT5V49EE503_NUM_BUS)
		return -EINVAL;

	drv = &idt5v49ee503_drv[bus_id];
	if (drv->valid != 1)
		return -EPERM;

	/* Enable specific PLL */
	return idt5v49ee503_mod_reg(drv,
				    IDT5V49EE503_PLL_SUSPEND_REG,
				    enable != 0 ? 1 : 0,
				    pll_id,
				    0x1);

} /* end of idt5v49ee503_pll_enable */

/****************************************************************************** 
   Enable or disable clock Output
   Input:
 	bus_id  - bus ID
 	out_id  - output ID
 	enable  - 1 for enable, 0 for disable
   Output:
      	NONE
 
   Returns:
 	0      - success,
*******************************************************************************/
int idt5v49ee503_out_enable (int             bus_id,
			     idt_output_id_t out_id,
			     int             enable)
{
	idt_drv_info_t	*drv = NULL;

	if (bus_id >= IDT5V49EE503_NUM_BUS)
		return -EINVAL;

	drv = &idt5v49ee503_drv[bus_id];
	if (drv->valid != 1)
		return -EPERM;

	/* Enable specific PLL */
	return idt5v49ee503_mod_reg(drv,
				    IDT5V49EE503_OUT_SUSPEND_REG,
				    enable != 0 ? 1 : 0,
				    out_id,
				    0x1);

} /* end of idt5v49ee503_out_enable */

/****************************************************************************** 
   Enable or disable clock Output
   Input:
 	bus_id  - bus ID
 	sw_ctl  - 1 - SW control, 0 - HW control
   Output:
      	NONE
 
   Returns:
 	0      - success,
*******************************************************************************/
int idt5v49ee503_sw_ctrl (int  bus_id,
			  int  sw_ctl)
{
	idt_drv_info_t	*drv = NULL;

	if (bus_id >= IDT5V49EE503_NUM_BUS)
		return -EINVAL;

	drv = &idt5v49ee503_drv[bus_id];
	if (drv->valid != 1)
		return -EPERM;

	/* Enable specific PLL */
	return idt5v49ee503_write_reg(drv,
				      IDT5V49EE503_SW_MODE_CTL_REG,
				      sw_ctl != 0 ? 1 : 0);

} /* end of idt5v49ee503_out_enable */

/****************************************************************************** 
   Program clock with specific frequency
   Input:
 	freq_set - frequency to be programmed
 		(only a number of fixed freqiencies is supported)
 	clock_cfg - clock configuration
   Output:
      	NONE
 
   Returns:
	0	- success,
	-EINVAL	- unsupported frequency
*******************************************************************************/
int idt5v49ee503_set (unsigned long	freq_set, 
		      idt_clock_cfg_t	*clock_cfg)
{
	int	i;
	int	rval;

	for (i = 0; idt5v49ee503_freq_tbl[i].f_out != 0; i++)
	{
		if (idt5v49ee503_freq_tbl[i].f_out == freq_set)
		{
			rval = idt5v49ee503_write_cfg(i, clock_cfg);

			if ((rval == 0) && (clock_cfg->cfg_act))
			{ /* activate clock and config */

				rval = idt5v49ee503_set_act_cfg(clock_cfg);
				/* enable PLL core */
				if (rval == 0)
				{	
					rval = idt5v49ee503_pll_enable(clock_cfg->bus_id,
								       clock_cfg->clock_id,
								       1);
				}
				/* enable clock output pin */
				if (rval == 0)
				{	
					rval = idt5v49ee503_out_enable(clock_cfg->bus_id,
								       clock_cfg->out_id,
								       1);
				}
				/* use SW control (i.e. values stored in clock registers) */
				return idt5v49ee503_sw_ctrl(clock_cfg->bus_id, 1);

			} /* activate clock and config */
		}
	}

	return -EINVAL;

} /* end of idt5v49ee503_set */

