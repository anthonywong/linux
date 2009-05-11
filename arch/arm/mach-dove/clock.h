struct clk {
	unsigned long		*rate;
	u32			flags;
	__s8                    usecount;
	u32			mask;
};

/* Clock flags */
#define ALWAYS_ENABLED          1

int dove_devclks_init(void);
unsigned int  dove_tclk_get(void);
void clks_disable_all(int include_pci0, int include_pci1);
