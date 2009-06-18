struct clk {
	unsigned int dummy;
	char *name;
};

void clks_register(struct clk *clks, size_t num);
