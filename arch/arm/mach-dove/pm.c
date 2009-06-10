/*
 * pm.c
 *
 * Power Management functions for Marvell Dove System On Chip
 *
 * Maintainer: Tawfik Bayouk <tawfik@marvell.com>
 *
 * This file is licensed under  the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#undef DEBUG

#include <linux/module.h>
#include <linux/sysfs.h>
#include <linux/proc_fs.h>
#include <linux/interrupt.h>
#include <linux/suspend.h>
#include <linux/interrupt.h>
#include <asm/hardware/cache-tauros2.h>
#include <asm/mach/arch.h>
#include <asm-arm/vfp.h>
#include "mvOs.h"
#include "pmu/mvPmu.h"
#include "pmu/mvPmuRegs.h"
#include "ctrlEnv/mvCtrlEnvSpec.h"
#include "common.h"
#include <asm/cpu-single.h>

#define CONFIG_PMU_PROC

/*
 * Global holding the target PM state
 * This should be replaced by the PMU flag register
 */
static suspend_state_t dove_target_pm_state = PM_SUSPEND_ON;
static void cpu_do_idle_enabled(void);
static void cpu_do_idle_disabled(void);

#ifdef CONFIG_PMU_PROC
extern MV_STATUS mvPmuDvs (MV_U32 pSet, MV_U32 vSet, MV_U32 rAddr, MV_U32 sAddr);
extern MV_STATUS mvPmuCpuFreqScale (MV_PMU_CPU_SPEED cpuSpeed);
void dove_pm_cpuidle_deepidle (void);
static MV_U32 deepIdleCtr = 0;
int pmu_proc_write(struct file *file, const char *buffer,unsigned long count,
		     void *data)
{
	int len = 0;
	char *str;
	unsigned long ints;
	unsigned int mc, mc2;
	int dummy;
	MV_U32 reg;

	str = "dvs ";
	if(!strncmp(buffer+len, str,strlen(str))) {
		len += strlen(str);
		str = "+10";
		if(!strncmp(buffer+len, str,strlen(str))) {
			if (mvPmuDvs(15, 0x8, 0x2, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}
		str = "+7.5";
		if(!strncmp(buffer+len, str,strlen(str))) {
			if (mvPmuDvs(14, 0x8, 0x2, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}
		str = "+5";
		if(!strncmp(buffer+len, str,strlen(str))) {
			if (mvPmuDvs(13, 0x8, 0x2, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}
		str = "+2.5";
		if(!strncmp(buffer+len, str,strlen(str))) {
			if (mvPmuDvs(12, 0x8, 0x2, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}
		str = "0";
		if(!strncmp(buffer+len, str,strlen(str))) {
			if (mvPmuDvs(0, 0, 0x2, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}
		str = "-2.5";
		if(!strncmp(buffer+len, str,strlen(str))) {
			if (mvPmuDvs(11, 0x8, 0x2, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}
		str = "-5";
		if(!strncmp(buffer+len, str,strlen(str))) {
			if (mvPmuDvs(10, 0x8, 0x2, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}
		str = "-7.5";
		if(!strncmp(buffer+len, str,strlen(str))) {
			if (mvPmuDvs(9, 0x8, 0x2, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}
		str = "-10";
		if(!strncmp(buffer+len, str,strlen(str))) {
			if (mvPmuDvs(8, 0x8, 0x2, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}
		str = "1.2";
		if(!strncmp(buffer+len, str,strlen(str))) {
			if (mvPmuDvs(0, 0x9, 0x2, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}
		str = "1.0";
		if(!strncmp(buffer+len, str,strlen(str))) {
			if (mvPmuDvs(0, 0x8, 0x2, 0x5) != MV_OK)
				printk(">>>>>>>>>>>>> FAILED\n");
		}

		goto done;
	}

	str = "cpudfs ";
	if(!strncmp(buffer+len, str,strlen(str))) {
		len += strlen(str);
		str = "turbo";
		if(!strncmp(buffer+len, str,strlen(str))) {
			len += strlen(str);
			printk("Set CPU Frequency to TURBO ");
			local_irq_save(ints);
			mc = MV_REG_READ(0x20210);
			MV_REG_WRITE(0x20214, (mc | 0x2));
			if (mvPmuCpuFreqScale (CPU_CLOCK_TURBO) != MV_OK)
				printk(">>>>>> FAILED\n");
			else
				printk("\n");
			MV_REG_WRITE(0x20214, mc);
			local_irq_restore(ints);
			goto done;
		}
		str = "ddr";
		if(!strncmp(buffer+len, str,strlen(str))) {
			len += strlen(str);
			printk("Set CPU Frequency to DDR frequency ");
			local_irq_save(ints);
			mc = MV_REG_READ(0x20210);
			MV_REG_WRITE(0x20214, (mc | 0x2));
			if (mvPmuCpuFreqScale (CPU_CLOCK_SLOW) != MV_OK)
				printk(">>>>>> FAILED\n");
			else
				printk("\n");
			MV_REG_WRITE(0x20214, mc);
			local_irq_restore(ints);
			goto done;
		}
		goto done;
	}

	str = "idle ";
	if(!strncmp(buffer+len, str,strlen(str))) {
		len += strlen(str);
		str = "deep";
		if(!strncmp(buffer+len, str,strlen(str))) {
			len += strlen(str);
			printk("Use Deep Idle for OS IDLE.\n");
			pm_idle = dove_pm_cpuidle_deepidle;
			goto done;
		}
		str = "wfill";
		if(!strncmp(buffer+len, str,strlen(str))) {
			len += strlen(str);
			printk("Use WFI with L1 and L2 low leakage for OS IDLE.\n");
			pm_idle = cpu_do_idle_enabled;
			reg = MV_REG_READ(PMU_CTRL_REG);
			reg &= ~PMU_CTRL_L2_LOWLEAK_EN_MASK;
			MV_REG_WRITE(PMU_CTRL_REG, reg);
			goto done;
		}
		str = "wfi";
		if(!strncmp(buffer+len, str,strlen(str))) {
			len += strlen(str);
			printk("Use WFI for OS IDLE.\n");
			pm_idle = cpu_do_idle_enabled;
			reg = MV_REG_READ(PMU_CTRL_REG);
			reg |= PMU_CTRL_L2_LOWLEAK_EN_MASK;
			MV_REG_WRITE(PMU_CTRL_REG, reg);
			goto done;
		}
		str = "none";
		if(!strncmp(buffer+len, str,strlen(str))) {
			len += strlen(str);
			printk("Don't use WFI for OS IDLE.\n");	
			pm_idle = cpu_do_idle_disabled;
			goto done;
		}
		goto done;
	}

	str = "sysdfs ";
	if(!strncmp(buffer+len, str,strlen(str))) {
		MV_U32 cpuFreq, l2Freq, ddrFreq;

		len += strlen(str);
		sscanf (buffer+len, "%d %d %d", &cpuFreq, &l2Freq, &ddrFreq);

		printk("Set New System Frequencies to CPU %dMhz, L2 %dMhz, DDR %dMhz", cpuFreq, l2Freq, ddrFreq);
		local_irq_save(ints);
		mc = MV_REG_READ(0x20210);
		MV_REG_WRITE(0x20214, (mc | 0x2));	/* PMU Interrupt Enable */
		if (mvPmuSysFreqScale (ddrFreq, l2Freq, cpuFreq) != MV_OK)
			printk(">>>>>> FAILED\n");
		else
			printk("\n");
		MV_REG_WRITE(0x20214, mc);
		local_irq_restore(ints);
		goto done;
	}

	str = "deepidle";
	if(!strncmp(buffer+len, str,strlen(str))) {
		dove_pm_cpuidle_deepidle();
		goto done;
	}

	str = "deepidle_block";
	if(!strncmp(buffer+len, str,strlen(str))) {
		printk("Enter DeepIdle mode ");
		mc = MV_REG_READ(0x20204);
		mc2 = MV_REG_READ(0x20214);
		MV_REG_WRITE(0x20204, 0x100); /* disable all interrupts except for the serial port */
		MV_REG_WRITE(0x20214, 0x0);
		dove_pm_cpuidle_deepidle();
		MV_REG_WRITE(0x20204, mc);
		MV_REG_WRITE(0x20214, mc2);
		goto done;
	}

	str = "standby";
	if(!strncmp(buffer+len, str,strlen(str))) {
		printk("Enter Standby mode ");

		if (mvPmuStandby() != MV_OK)
			printk(">>>>>> FAILED\n");
		else
			printk("\n");
		goto done;
	}

	str = "wfi_block";
	if(!strncmp(buffer+len, str,strlen(str))) {
		printk("Enter WFI mode ");
		mc = MV_REG_READ(0x20204);
		mc2 = MV_REG_READ(0x20214);
		MV_REG_WRITE(0x20204, 0x100); /* disable all interrupts except for the serial port */
		MV_REG_WRITE(0x20214, 0x0);		

		__asm__ __volatile__("mcr p15, 0, %0, c7, c0, 4\n" : "=r" (dummy));

		MV_REG_WRITE(0x20204, mc);
		MV_REG_WRITE(0x20214, mc2);
		goto done;
	}

	str = "block";
	if(!strncmp(buffer+len, str,strlen(str))) {
		mc = MV_REG_READ(0x20204);
		mc2 = MV_REG_READ(0x20214);
		MV_REG_WRITE(0x20204, 0x0);
		MV_REG_WRITE(0x20214, 0x0);
		while(1);
		goto done;
	}

	str = "dvfs ";
	if(!strncmp(buffer+len, str,strlen(str))) {
		len += strlen(str);
		str = "hi";
		if(!strncmp(buffer+len, str,strlen(str))) {
			len += strlen(str);
			printk("Going to hi gear (CPU=TURBO, V=1.10) ");
			/* Upscale Voltage +10% */
			if (mvPmuDvs(15, 0x8, 0x2, 0x5) != MV_OK)
				printk("Volatge up-scaling failed\n");

			/* Upscale frequency */
			local_irq_save(ints);
			mc = MV_REG_READ(0x20210);
			MV_REG_WRITE(0x20214, (mc | 0x2));
			if (mvPmuCpuFreqScale (CPU_CLOCK_TURBO) != MV_OK)
				printk(">>>>>> FAILED\n");
			else
				printk("\n");
			MV_REG_WRITE(0x20214, mc);
			local_irq_restore(ints);
			goto done;
		}
		str = "lo";
		if(!strncmp(buffer+len, str,strlen(str))) {
			len += strlen(str);
			printk("Going to low gear (CPU=DDR, V=0.975) ");
			/* Downscale frequency */
			local_irq_save(ints);
			mc = MV_REG_READ(0x20210);
			MV_REG_WRITE(0x20214, (mc | 0x2));
			if (mvPmuCpuFreqScale (CPU_CLOCK_SLOW) != MV_OK)
				printk(">>>>>> FAILED\n");
			else
				printk("\n");
			MV_REG_WRITE(0x20214, mc);
			local_irq_restore(ints);

			/* Downscale Voltage -2.5% */
			if (mvPmuDvs(11, 0x8, 0x2, 0x5) != MV_OK)
				printk("Volatge down-scaling failed\n");
			goto done;
		}
		goto done;
	}

	str = "vpu ";
	if(!strncmp(buffer+len, str,strlen(str))) {
		len += strlen(str);
		str = "off";
		if(!strncmp(buffer+len, str,strlen(str))) {
			len += strlen(str);
			printk("Setting VPU power OFF.\n");
			/* enable isolators */
			reg = MV_REG_READ(PMU_ISO_CTRL_REG);
			reg &= ~PMU_ISO_VIDEO_MASK;
			MV_REG_WRITE(PMU_ISO_CTRL_REG, reg);
			/* reset unit */
			reg = MV_REG_READ(PMU_SW_RST_CTRL_REG);
			reg &= ~PMU_SW_RST_VIDEO_MASK;
			MV_REG_WRITE(PMU_SW_RST_CTRL_REG, reg);
			/* power off */
			reg = MV_REG_READ(PMU_PWR_SUPLY_CTRL_REG);
			reg |= PMU_PWR_VPU_PWR_DWN_MASK;
			MV_REG_WRITE(PMU_PWR_SUPLY_CTRL_REG, reg);
			goto done;
		}
		str = "on";
		if(!strncmp(buffer+len, str,strlen(str))) {
			len += strlen(str);
			printk("Setting VPU power ON.\n");
			/* power on */
			reg = MV_REG_READ(PMU_PWR_SUPLY_CTRL_REG);
			reg &= ~PMU_PWR_VPU_PWR_DWN_MASK;
			MV_REG_WRITE(PMU_PWR_SUPLY_CTRL_REG, reg);
			/* un-reset unit */
			reg = MV_REG_READ(PMU_SW_RST_CTRL_REG);
			reg |= PMU_SW_RST_VIDEO_MASK;
			MV_REG_WRITE(PMU_SW_RST_CTRL_REG, reg);
			/* disable isolators */
			reg = MV_REG_READ(PMU_ISO_CTRL_REG);
			reg |= PMU_ISO_VIDEO_MASK;
			MV_REG_WRITE(PMU_ISO_CTRL_REG, reg);
			goto done;
		}
		goto done;
	}

	str = "gpu ";
	if(!strncmp(buffer+len, str,strlen(str))) {
		len += strlen(str);
		str = "off";
		if(!strncmp(buffer+len, str,strlen(str))) {
			len += strlen(str);
			printk("Setting GPU power OFF.\n");
			/* enable isolators */
			reg = MV_REG_READ(PMU_ISO_CTRL_REG);
			reg &= ~PMU_ISO_GPU_MASK;
			MV_REG_WRITE(PMU_ISO_CTRL_REG, reg);
			/* reset unit */
			reg = MV_REG_READ(PMU_SW_RST_CTRL_REG);
			reg &= ~PMU_SW_RST_GPU_MASK;
			MV_REG_WRITE(PMU_SW_RST_CTRL_REG, reg);
			/* power off */
			reg = MV_REG_READ(PMU_PWR_SUPLY_CTRL_REG);
			reg |= PMU_PWR_GPU_PWR_DWN_MASK;
			MV_REG_WRITE(PMU_PWR_SUPLY_CTRL_REG, reg);
			goto done;
		}
		str = "on";
		if(!strncmp(buffer+len, str,strlen(str))) {
			len += strlen(str);
			printk("Setting GPU power ON.\n");
			/* power on */
			reg = MV_REG_READ(PMU_PWR_SUPLY_CTRL_REG);
			reg &= ~PMU_PWR_GPU_PWR_DWN_MASK;
			MV_REG_WRITE(PMU_PWR_SUPLY_CTRL_REG, reg);
			/* un-reset unit */
			reg = MV_REG_READ(PMU_SW_RST_CTRL_REG);
			reg |= PMU_SW_RST_GPU_MASK;
			MV_REG_WRITE(PMU_SW_RST_CTRL_REG, reg);
			/* disable isolators */
			reg = MV_REG_READ(PMU_ISO_CTRL_REG);
			reg |= PMU_ISO_GPU_MASK;
			MV_REG_WRITE(PMU_ISO_CTRL_REG, reg);
			goto done;
		}
		goto done;
	}

	str = "wlan ";
	if(!strncmp(buffer+len, str,strlen(str))) {
		len += strlen(str);
		str = "off";
		if(!strncmp(buffer+len, str,strlen(str))) {
			len += strlen(str);
			printk("Setting WLAN SC in low power.\n");
			/* Set level */
			reg = MV_REG_READ(0xD0400);
			reg &= ~0x10;
			MV_REG_WRITE(0xD0400, reg);
			/* verify pin is output */
			reg = MV_REG_READ(0xD0404);
			reg &= ~0x10;
			MV_REG_WRITE(0xD0404, reg);
			goto done;
		}
		str = "on";
		if(!strncmp(buffer+len, str,strlen(str))) {
			len += strlen(str);
			printk("Setting WLAN SC out of low power.\n");
			/* Set level */
			reg = MV_REG_READ(0xD0400);
			reg |= 0x10;
			MV_REG_WRITE(0xD0400, reg);
			/* verify pin is output */
			reg = MV_REG_READ(0xD0404);
			reg &= ~0x10;
			MV_REG_WRITE(0xD0404, reg);
			goto done;
		}
		goto done;
	}

	str = "freqs";
	if(!strncmp(buffer+len, str,strlen(str)))
	{
		MV_PMU_FREQ_INFO freqs;

		printk("Frequencies: ");
		if(mvPmuGetCurrentFreq(&freqs) != MV_OK)
			printk(">>>>>> FAILED!\n");
		else
			printk("CPU %dMhz, AXI %dMHz, L2 %dMhz, DDR %dMhz\n", 
				freqs.cpuFreq, freqs.axiFreq, freqs.l2Freq, freqs.ddrFreq);
	}

	str = "deepcnt";
	if(!strncmp(buffer+len, str,strlen(str)))
	{
		printk("Deep Idle Entered for %d times.\n", deepIdleCtr);
	}
done:
	return count;
}

int pmu_proc_read(char* page, char** start, off_t off, int count,int* eof,
		    void* data)
{
	int len = 0;

	len += sprintf(page+len,"PM Proc debug shell:\n");
	len += sprintf(page+len,"   dvs <+10|+7.5|+5|+2.5|0|-2.5|-5|-7.5|-10>\n");
	len += sprintf(page+len,"   cpudfs <turbo|ddr>\n");
	len += sprintf(page+len,"   sysdfs <cpu> <l2> <ddr>\n");
	len += sprintf(page+len,"   freqs\n");
	len += sprintf(page+len,"   deepidle\n");
	len += sprintf(page+len,"   standby\n");
	len += sprintf(page+len,"   deepidle_block\n");
	len += sprintf(page+len,"   wfi_block\n");
	len += sprintf(page+len,"   idle <wfi|wfiwll|none>\n");
	len += sprintf(page+len,"   dvfs <hi|lo>\n");
	len += sprintf(page+len,"   gpu <on|off>\n");
	len += sprintf(page+len,"   vpu <on|off>\n");
	len += sprintf(page+len,"   wlan <on|off>\n");
	len += sprintf(page+len,"   deepcnt\n");
	return len;
}

static struct proc_dir_entry *pmu_proc_entry;
extern struct proc_dir_entry proc_root;
static int pm_proc_init(void)
{
	pmu_proc_entry = create_proc_entry("pm", 0666, &proc_root);
	pmu_proc_entry->read_proc = pmu_proc_read;
	pmu_proc_entry->write_proc = pmu_proc_write;
	pmu_proc_entry->owner = THIS_MODULE;
	return 0;
}
#endif /* CONFIG_PMU_PROC */

MV_BOOL enable_ebook = MV_TRUE;

static ssize_t ebook_show(struct kobject *kobj, struct kobj_attribute *attr,
			 char *buf)
{
	return sprintf(buf, "%lu\n", (unsigned long)enable_ebook);
}

static ssize_t ebook_store(struct kobject *kobj, struct kobj_attribute *attr,
			  const char * buf, size_t n)
{
	unsigned short value;
	if (sscanf(buf, "%hu", &value) != 1 ||
	    (value != 0 && value != 1)) {
		printk(KERN_ERR "idle_sleep_store: Invalid value\n");
		return -EINVAL;
	}
	if (value)
		enable_ebook = MV_TRUE;
	else
		enable_ebook = MV_FALSE;
		
	return n;
}

static struct kobj_attribute ebook_attr =
	__ATTR(ebook, 0644, ebook_show, ebook_store);

/*
 * Idle routine in normal operation
 */
void cpu_do_idle_enabled(void) {
	cpu_do_idle();
}

/*
 * This routine replaces the default_idle() routine during the PM state changes,
 * to avoid calling the WFI instruction.
 */
void cpu_do_idle_disabled(void) {
}

/*
 * Enter the Dove DEEP IDLE mode (power off CPU only)
 */
void dove_deepidle(void)
{
	MV_U32 reg;

	pr_debug("dove_deepidle: Entering Dove DEEP IDLE mode.\n");

	/* Put on the Led on MPP7 */
	reg = MV_REG_READ(PMU_SIG_SLCT_CTRL_0_REG);
	reg &= ~PMU_SIG_7_SLCT_CTRL_MASK;
	reg |= (PMU_SIGNAL_0 << PMU_SIG_7_SLCT_CTRL_OFFS);
	MV_REG_WRITE(PMU_SIG_SLCT_CTRL_0_REG, reg);


#if defined(CONFIG_CACHE_TAUROS2)
	tauros2_halt();
#endif
#if defined(CONFIG_VFP)
	vfp_save();
#endif

	/* Suspend the CPU only */
	mvPmuDeepIdle(enable_ebook);
	cpu_init();

#if defined(CONFIG_VFP)
	vfp_restore();
#endif

	/* Reinit L2 Cache */
#if defined(CONFIG_CACHE_TAUROS2)	
	tauros2_resume();
#endif
	/* Put off the Led on MPP7 */
	reg = MV_REG_READ(PMU_SIG_SLCT_CTRL_0_REG);
	reg &= ~PMU_SIG_7_SLCT_CTRL_MASK;
	reg |= (PMU_SIGNAL_1 << PMU_SIG_7_SLCT_CTRL_OFFS);
	MV_REG_WRITE(PMU_SIG_SLCT_CTRL_0_REG, reg);

	pr_debug("dove_deepidle: Exiting Dove DEEP IDLE mode.\n");
}

/*
 * Enter the Dove STANDBY mode (Power off all SoC)
 */
void dove_standby(void)
{
	MV_U32 reg;

	pr_debug("dove_standby: Entering Dove STANDBY mode.\n");

	/* Put on the Led on MPP7 */
	reg = MV_REG_READ(PMU_SIG_SLCT_CTRL_0_REG);
	reg &= ~PMU_SIG_7_SLCT_CTRL_MASK;
	reg |= (PMU_SIGNAL_BLINK/*PMU_SIGNAL_0*/ << PMU_SIG_7_SLCT_CTRL_OFFS);
	MV_REG_WRITE(PMU_SIG_SLCT_CTRL_0_REG, reg);

	/* Save CPU Peripherals state */
	dove_save_cpu_wins();
	dove_save_cpu_conf_regs();
	dove_save_timer_regs();	
	dove_save_int_regs();

#if defined(CONFIG_CACHE_TAUROS2)
	tauros2_halt();
#endif

#if defined(CONFIG_VFP)
	vfp_save();
#endif

	/* Suspend the CPU only */
	mvPmuStandby();
	cpu_init();

#if defined(CONFIG_VFP)
	vfp_restore();
#endif

	/* Reinit L2 Cache */
#if defined(CONFIG_CACHE_TAUROS2)	
	tauros2_resume();
#endif

	/* Restore CPU Peripherals state */
	dove_restore_int_regs();
	dove_restore_timer_regs();	
	dove_restore_cpu_conf_regs();
	dove_restore_cpu_wins();
	//dove_restore_pcie_regs(); /* Should be done after restoring cpu configuration registers */

	/* Put off the Led on MPP7 */
	reg = MV_REG_READ(PMU_SIG_SLCT_CTRL_0_REG);
	reg &= ~PMU_SIG_7_SLCT_CTRL_MASK;
	reg |= (PMU_SIGNAL_1 << PMU_SIG_7_SLCT_CTRL_OFFS);
	MV_REG_WRITE(PMU_SIG_SLCT_CTRL_0_REG, reg);

	pr_debug("dove_standby: Exiting Dove STANDBY mode.\n");
}

/*
 * Logical check for Dove valid PM states
 */
static int dove_pm_valid(suspend_state_t state)
{
	return ((state == PM_SUSPEND_MEM) ||
		(state == PM_SUSPEND_STANDBY));
}

/*
 * Initialise a transition to given system sleep state.
 * This is needed by all devices to decide whether to save device context
 * (for "ram") or not (for "standby")
 */
static int dove_pm_begin(suspend_state_t state)
{
	/* TODO: write target mode to the PMU flag register */
	dove_target_pm_state = state;
	return 0;
}

/*
 * Check if the CORE devices will loose (or have lost) power during suspend
 */
int dove_io_core_lost_power(void)
{
	if (dove_target_pm_state >= PM_SUSPEND_MEM)
		return 1;

	return 0;
}
EXPORT_SYMBOL(dove_io_core_lost_power);

/*
 * Preparation to enter a new PM state. This includes disabling the call
 * for the WFI instruction from the context of the do_idle() routine
 */
static int dove_pm_prepare(void)
{
	pr_debug("PM DEBUG: Preparing to enter PM state.\n");
	pm_idle = cpu_do_idle_disabled;
	return 0;
}

/*
 * Enter the requested PM state
 */
static int dove_pm_enter(suspend_state_t state)
{
	pr_debug("PM DEBUG: Entering PM state (%d).\n", state);
	switch (state)	{
	case PM_SUSPEND_STANDBY:
		dove_deepidle();
		break;
	case PM_SUSPEND_MEM:
		dove_standby();
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/*
 * This is called when the system has just left a sleep state, right after
 * the nonboot CPUs have been enabled pm_idle = dove_pand before devices are resumed (it is
 * executed with IRQs enabled
 */
static void dove_pm_finish(void)
{
	pm_idle = cpu_do_idle_enabled;
}

/*
 * This is called by the PM core right after resuming devices, to indicate to
 * the platform that the system has returned to the working state or
 * the transition to the sleep state has been aborted.
 */
static void dove_pm_end(void)
{
	dove_target_pm_state = PM_SUSPEND_ON;
}

static struct platform_suspend_ops dove_pm_ops = {
	.valid		= dove_pm_valid,
	.begin     	= dove_pm_begin,
	.prepare	= dove_pm_prepare,
	.enter		= dove_pm_enter,
	.finish		= dove_pm_finish,
	.end		= dove_pm_end,
};

int dove_timekeeping_resume(void);
int dove_timekeeping_suspend(void);

void dove_pm_cpuidle_deepidle (void)
{
	unsigned long ints;

	//MV_U32 ier, lcr;
#ifdef CONFIG_PMU_PROC
	deepIdleCtr++;
#endif
	dove_pm_prepare();
	local_irq_save(ints);
	sysdev_suspend(PMSG_SUSPEND);
	dove_pm_enter(PM_SUSPEND_STANDBY);
	sysdev_resume();	
	local_irq_restore(ints);
	dove_pm_finish();
}

void dove_pm_register (void)
{
	MV_PMU_FREQ_INFO freqs;

	printk(KERN_NOTICE "Power Management for Marvell Dove.\n");

	pm_idle = cpu_do_idle_enabled;
	suspend_set_ops(&dove_pm_ops);

	/* Create EBook control file in sysfs */
	if (sysfs_create_file(power_kobj, &ebook_attr.attr))
		printk(KERN_ERR "dove_pm_register: ebook sysfs_create_file failed!");

#ifdef CONFIG_PMU_PROC
	pm_proc_init();
#endif

	if (mvPmuGetCurrentFreq(&freqs) == MV_OK)
		printk(KERN_NOTICE "PMU Detected Frequencies CPU %dMhz, AXI %dMhz, L2 %dMhz, DDR %dMhz\n", 
				freqs.cpuFreq, freqs.axiFreq, freqs.l2Freq, freqs.ddrFreq);
	else
		printk(KERN_ERR "PMU Failed to detect current system frequencies!\n");
}
