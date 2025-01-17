/* Copyright (c) 2009-2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#include <linux/kernel.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/bootmem.h>
#include <linux/io.h>
#ifdef CONFIG_SPI_QSD
#include <linux/spi/spi.h>
#endif
#include <linux/mfd/pmic8058.h>
//[SIMT-lilening-20110804] enable vibrator{
#ifdef CONFIG_PMIC8058_VIBRATOR
#include <linux/pmic8058-vibrator.h>
#endif
//[SIMT-lilening-20110804] enable vibrator}
#include <linux/mfd/marimba.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/ofn_atlab.h>
#include <linux/power_supply.h>
#include <linux/input/pmic8058-keypad.h>
#include <linux/i2c/isa1200.h>
#include <linux/pwm.h>
#include <linux/pmic8058-pwm.h>
#include <linux/i2c/tsc2007.h>
#include <linux/leds-pmic8058.h>
#include <linux/input/cy8c_ts.h>
#include <linux/msm_adc.h>
#include <linux/dma-mapping.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/setup.h>

#include <mach/mpp.h>
#include <mach/board.h>
#include <mach/camera.h>
#include <mach/memory.h>
#include <mach/msm_iomap.h>
#include <mach/msm_hsusb.h>
#include <mach/rpc_hsusb.h>
#include <mach/msm_spi.h>
#include <mach/qdsp5v2/msm_lpa.h>
#include <mach/dma.h>
#include <linux/android_pmem.h>
#include <linux/input/msm_ts.h>
#include <mach/pmic.h>
#include <mach/rpc_pmapp.h>
#include <mach/qdsp5v2/aux_pcm.h>
#include <mach/qdsp5v2/mi2s.h>
#include <mach/qdsp5v2/audio_dev_ctl.h>
#include <mach/msm_battery.h>
#include <mach/rpc_server_handset.h>
#include <mach/msm_tsif.h>
#include <mach/socinfo.h>

#include <linux/i2c-gpio.h>
#include <linux/spi/spi.h>
#include <linux/cyttsp.h>
#include <linux/atmel_ctp.h>

#define GPIO_CTP_IRQ 112 //for Ponyo Atmel's TP

#define GPIO_I2C_SDA 124
#define GPIO_I2C_SCL 123

#ifdef CONFIG_CM3623_GPIO
#define GPIO_I2C_SDA_CM3623 121
#define GPIO_I2C_SCL_CM3623 120
#endif

#include <asm/mach/mmc.h>
#include <asm/mach/flash.h>
#include <mach/vreg.h>
#include "devices.h"
#include "timer.h"
#ifdef CONFIG_USB_ANDROID
#include <linux/usb/android_composite.h>
#endif
#include "pm.h"
#include "spm.h"
#include <mach/msm_serial_hs.h>
#include <mach/msm_reqs.h>
#include <mach/qdsp5v2/mi2s.h>
#include <mach/qdsp5v2/audio_dev_ctl.h>
#include <mach/sdio_al.h>
#include "smd_private.h"
#ifdef CONFIG_CM3623
#include <linux/cm3623.h>
#endif
#include "msm-keypad-devices.h" 

//[SIMT-huangli-110718] common part for bma device
#include "../../../drivers/i2c/chips/bma_common.h"

#ifdef CONFIG_BOSCH_BMA222
#include "../../../drivers/i2c/chips/bma222.h"
#endif


#ifdef CONFIG_BOSCH_BMA250
#include "../../../drivers/i2c/chips/bma250.h"
#endif


#define MSM_PMEM_SF_SIZE	0x1700000
#ifdef CONFIG_FB_MSM_TRIPLE_BUFFER
#define MSM_FB_SIZE            0x465000 /* 480 * 800 * 4bpp * 3 Pages */ // 0x780000
#else
#define MSM_FB_SIZE            0x2ee000 /* 480 * 800 * 4bpp * 2 Pages */ // 0x500000
#endif
#define MSM_PMEM_ADSP_SIZE      0x2000000 		//[SIMT-wanglei-110921] 0x1E00000 // 0x4B00000
#define PMEM_KERNEL_EBI1_SIZE   0x600000
#define MSM_PMEM_AUDIO_SIZE     0x200000

#define PMIC_GPIO_INT		27
#define PMIC_GPIO_SD_DET	36
#define PMIC_GPIO_SDC4_PWR_EN_N	24  /* PMIC GPIO Number 25 */ //changed by Yuhaipeng 20110617
#define PMIC_GPIO_HDMI_5V_EN_V3 32  /* PMIC GPIO for V3 H/W */

#define WIFI_GPIO_ENABLE 126

#define ADV7520_I2C_ADDR	0x39

#define FPGA_OPTNAV_GPIO_ADDR	0x8E000026
#define OPTNAV_I2C_SLAVE_ADDR	(0xB0 >> 1)
#define OPTNAV_IRQ		20
#define OPTNAV_CHIP_SELECT	19

/* Macros assume PMIC GPIOs start at 0 */
#define PM8058_GPIO_PM_TO_SYS(pm_gpio)     (pm_gpio + NR_GPIO_IRQS)
#define PM8058_GPIO_SYS_TO_PM(sys_gpio)    (sys_gpio - NR_GPIO_IRQS)

#define PMIC_GPIO_WLAN_EXT_POR  22 /* PMIC GPIO NUMBER 23 */

#define PMIC_GPIO_QUICKVX_CLK 37 /* PMIC GPIO 38 */

//[SIMT-caoxiangteng-110715]{
/*
 *  add gpio i2c simulate for poyon
 */
#ifdef CONFIG_CM3623_GPIO_I2C
#define GPIO_I2C_SDA_CM3623 121
#define GPIO_I2C_SCL_CM3623 120
#define GPIO_IRQ_CM3623	142
#endif
//[SIMT-caoxiangteng-110715]}


#if defined(CONFIG_M6MO) || defined(CONFIG_MT9D112)
static struct vreg *vreg_gp2;
#endif
static struct vreg *vreg_gp7;
#if defined(CONFIG_M6MO) || defined(CONFIG_MT9D112)
static struct vreg *vreg_lvsw1;
#endif
//static struct vreg *vreg_gp6;
//static struct vreg *vreg_gp16;
static struct vreg *ljvreg_gp13 = NULL;
static struct vreg *ljvreg_gp5 = NULL;
static struct vreg *ljvreg_wlan2 = NULL;
#ifdef CONFIG_M6MO
static struct vreg *vreg_gp15;
static struct vreg *vreg_wlan2;
static struct vreg *vreg_gp5;
#endif

#define WL_REG_ON 1
#define WLAN_RESET 2

void bcm_wlan_power_off(int flag)
{
	if (flag == WL_REG_ON) {
		gpio_direction_output(WIFI_GPIO_ENABLE, 0);
	}
}
EXPORT_SYMBOL(bcm_wlan_power_off);

void bcm_wlan_power_on(int flag)
{
	if (flag == WL_REG_ON) {
		gpio_direction_output(WIFI_GPIO_ENABLE, 1);
	} else {
		gpio_direction_output(WIFI_GPIO_ENABLE, 1);
		msleep(100);
		gpio_direction_output(WIFI_GPIO_ENABLE, 0);
		msleep(100);
		gpio_direction_output(WIFI_GPIO_ENABLE, 1);
		msleep(100);
	}
}
EXPORT_SYMBOL(bcm_wlan_power_on);

//Add by TaoYuan 2011.3.25
void BCM4330_PowerOn(void)
{
  printk(KERN_ERR "BCM4330_DEBUG:starting to power init!!\n");
  gpio_set_value(127, 1);
  msleep(100);
  gpio_set_value(128, 1);
  msleep(100);
}

void BCM4330_PowerOff(void)
{
  printk(KERN_ERR "BCM4330_DEBUG:starting to power off!!\n");
  gpio_set_value(127, 0);
  gpio_set_value(128, 0);
}


static int pm8058_gpios_init(void)
{
	int rc;
	int pmic_gpio_hdmi_5v_en;

	struct pm8058_gpio sdc4_en = {
		.direction      = PM_GPIO_DIR_OUT,
		.pull           = PM_GPIO_PULL_NO,
		.vin_sel        = PM_GPIO_VIN_L5,
		.function       = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol    = 0,
		.out_strength   = PM_GPIO_STRENGTH_LOW,
		.output_value   = 0,
	};

	struct pm8058_gpio hdmi_5V_en = {
		.direction      = PM_GPIO_DIR_OUT,
		.pull           = PM_GPIO_PULL_NO,
		.vin_sel        = PM_GPIO_VIN_VPH,
		.function       = PM_GPIO_FUNC_NORMAL,
		.out_strength   = PM_GPIO_STRENGTH_LOW,
		.output_value   = 0,
	};

	struct pm8058_gpio gpio23 = {
			.direction      = PM_GPIO_DIR_OUT,
			.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
			.output_value   = 0,
			.pull           = PM_GPIO_PULL_NO,
			.vin_sel        = 2,
			.out_strength   = PM_GPIO_STRENGTH_LOW,
			.function       = PM_GPIO_FUNC_NORMAL,
	};

#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
	struct pm8058_gpio sdcc_det = {
		.direction      = PM_GPIO_DIR_IN,
		.pull           = PM_GPIO_PULL_UP_1P5,
		.vin_sel        = 2,
		.function       = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol    = 0,
	};

	rc = pm8058_gpio_config(PMIC_GPIO_SD_DET - 1, &sdcc_det);
	if (rc) {
		pr_err("%s PMIC_GPIO_SD_DET config failed\n", __func__);
		return rc;
	}
#endif

	pmic_gpio_hdmi_5v_en = PMIC_GPIO_HDMI_5V_EN_V3 ;

	rc = pm8058_gpio_config(pmic_gpio_hdmi_5v_en, &hdmi_5V_en);
	if (rc) {
		pr_err("%s PMIC_GPIO_HDMI_5V_EN config failed\n", __func__);
		return rc;
	}

	/* Deassert GPIO#23 (source for Ext_POR on WLAN-Volans) */
	rc = pm8058_gpio_config(PMIC_GPIO_WLAN_EXT_POR, &gpio23);
	if (rc) {
		pr_err("%s PMIC_GPIO_WLAN_EXT_POR config failed\n", __func__);
		return rc;
	}

	rc = pm8058_gpio_config(PMIC_GPIO_SDC4_PWR_EN_N, &sdc4_en);
	if (rc) {
		pr_err("%s PMIC_GPIO_SDC4_PWR_EN_N config failed\n",
							 __func__);
		return rc;
	}
	rc = gpio_request(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_SDC4_PWR_EN_N),
			  "sdc4_en");
	if (rc) {
		pr_err("%s PMIC_GPIO_SDC4_PWR_EN_N gpio_request failed\n",
			__func__);
		return rc;
	}
	gpio_set_value_cansleep(
		PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_SDC4_PWR_EN_N), 0);

	return 0;
}

static int pm8058_pwm_config(struct pwm_device *pwm, int ch, int on)
{
	struct pm8058_gpio pwm_gpio_config = {
		.direction      = PM_GPIO_DIR_OUT,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.output_value   = 0,
		.pull           = PM_GPIO_PULL_NO,
		.vin_sel        = PM_GPIO_VIN_S3,
		.out_strength   = PM_GPIO_STRENGTH_HIGH,
		.function       = PM_GPIO_FUNC_2,
	};
	int	rc = -EINVAL;
	int	id, mode, max_mA;

	id = mode = max_mA = 0;
	switch (ch) {
	case 0:
	case 1:
	case 2:
		if (on) {
			id = 24 + ch;
			rc = pm8058_gpio_config(id - 1, &pwm_gpio_config);
			if (rc)
				pr_err("%s: pm8058_gpio_config(%d): rc=%d\n",
				       __func__, id, rc);
		}
		break;

	case 3:
		id = PM_PWM_LED_KPD;
		mode = PM_PWM_CONF_DTEST3;
		max_mA = 200;
		break;

	case 4:
		id = PM_PWM_LED_0;
		mode = PM_PWM_CONF_PWM1;
		max_mA = 8;//led current is to 8 mA 16;// [SIMT-qiukejun-110812] led current is up to 15 mA //200;
		break;

	case 5:
		id = PM_PWM_LED_1;
		mode = PM_PWM_CONF_PWM2;
		max_mA = 8;//led  current is to 8 mA 16;// [SIMT-qiukejun-110812] led current is up to 15 mA //200;
		break;

	case 6:
		id = PM_PWM_LED_2;
		mode = PM_PWM_CONF_PWM3;
		max_mA = 40;
		break;

	default:
		break;
	}

	if (ch >= 3 && ch <= 6) {
		if (!on) {
			mode = PM_PWM_CONF_NONE;
			max_mA = 0;
		}
		rc = pm8058_pwm_config_led(pwm, id, mode, max_mA);
		if (rc)
			pr_err("%s: pm8058_pwm_config_led(ch=%d): rc=%d\n",
			       __func__, ch, rc);
	}

	return rc;
}

static int pm8058_pwm_enable(struct pwm_device *pwm, int ch, int on)
{
	int	rc;

	switch (ch) {
	case 7:
		rc = pm8058_pwm_set_dtest(pwm, on);
		if (rc)
			pr_err("%s: pwm_set_dtest(%d): rc=%d\n",
			       __func__, on, rc);
		break;
	default:
		rc = -EINVAL;
		break;
	}
	return rc;
}

static const unsigned int surf_keymap[] = {
	/* this keymap is fit to ponyo project*/
	KEY(0, 0, KEY_VOLUMEUP),//KEY_7/*SWH*/
	KEY(0, 1, KEY_BACK),//KEY_DOWN
	KEY(0, 2, KEY_MENU),//KEY_UP
        //[SIMT-huangli-20110903]{
        /*
         *  no global keyboard for ponyo
         */
	KEY(1, 0, KEY_VOLUMEDOWN),//KEY_LEFT/*SWH*/
	KEY(1,  1, KEY_HOME),//KEY_SEND
        //[SIMT-huangli-20110903]}
};

static struct resource resources_keypad[] = {
	{
		.start	= PM8058_KEYPAD_IRQ(PMIC8058_IRQ_BASE),
		.end	= PM8058_KEYPAD_IRQ(PMIC8058_IRQ_BASE),
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= PM8058_KEYSTUCK_IRQ(PMIC8058_IRQ_BASE),
		.end	= PM8058_KEYSTUCK_IRQ(PMIC8058_IRQ_BASE),
		.flags	= IORESOURCE_IRQ,
	},
};

static struct matrix_keymap_data surf_keymap_data = {
        .keymap_size    = ARRAY_SIZE(surf_keymap),
        .keymap         = surf_keymap,
};


static struct pmic8058_keypad_data surf_keypad_data = {
	.input_name		= "surf_keypad",
	.input_phys_device	= "surf_keypad/input0",
        //[SIMT-huangli-20110903]{
        /*
         *  no global keyboard for ponyo
         */
	.num_rows		= 5,
	.num_cols		= 5,
        //[SIMT-huangli-20110903]}
	.rows_gpio_start	= 8,
	.cols_gpio_start	= 0,
	.debounce_ms		= {8, 10},
	.scan_delay_ms		= 32,
	.row_hold_ns		= 91500,
	.wakeup			= 1,//0,//1,	//[SIMT-qiukejun-110812] disable key-wakeup 
	.keymap_data            = &surf_keymap_data,
};

static struct pm8058_pwm_pdata pm8058_pwm_data = {
	.config		= pm8058_pwm_config,
	.enable		= pm8058_pwm_enable,
};
//[SIMT-lilening-20110804] enable vibrator{
#ifdef CONFIG_PMIC8058_VIBRATOR
static struct pmic8058_vibrator_pdata pmic_vib_pdata = {
	.initial_vibrate_ms  = 500,
	.level_mV = 2700,
	.max_timeout_ms = 15000,
};
#endif
//[SIMT-lilening-20110804] enable vibrator}
#ifdef CONFIG_LEDS_GPIO
static struct gpio_led gpio_leds_config[] = {
	{
		.name = "torch",
		.gpio = 33,
		.active_low = 0,
		.retain_state_suspended = 0,
		.default_state = LEDS_GPIO_DEFSTATE_OFF,
	},
};
static struct gpio_led_platform_data gpio_leds_pdata = {
	.num_leds = ARRAY_SIZE(gpio_leds_config),
	.leds = gpio_leds_config,
};

static struct platform_device gpio_leds = {
	.name          = "leds-gpio",
	.id            = -1,
	.dev           = {
		.platform_data = &gpio_leds_pdata,
	},
};
#endif

/* Put sub devices with fixed location first in sub_devices array */
#define	PM8058_SUBDEV_KPD	0

#ifdef CONFIG_LEDS_PMIC8058 
#define	PM8058_SUBDEV_LED	1
static struct pmic8058_led pmic8058_ffa_leds[] = {
	[0] = {
		.name		= "flash",
		.max_brightness = 15,
		.id 	= PMIC8058_ID_FLASH_LED_1,
	},
};

static struct pmic8058_leds_platform_data pm8058_ffa_leds_data = {
	.num_leds = ARRAY_SIZE(pmic8058_ffa_leds),
	.leds	= pmic8058_ffa_leds,
};

#endif


static struct pm8058_gpio_platform_data pm8058_gpio_data = {
	.gpio_base	= PM8058_GPIO_PM_TO_SYS(0),
	.irq_base	= PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, 0),
	.init		= pm8058_gpios_init,
};

static struct pm8058_gpio_platform_data pm8058_mpp_data = {
	.gpio_base	= PM8058_GPIO_PM_TO_SYS(PM8058_GPIOS),
	.irq_base	= PM8058_MPP_IRQ(PMIC8058_IRQ_BASE, 0),
};




#ifdef CONFIG_LEDS_PWM
#include <linux/leds_pwm.h>
#define PWM_LED_PERIOD 2000
static struct led_pwm pmic8058_pwm_leds[] = {
	{
		.name		= "blue",
		.max_brightness = 40,
		.pwm_id		= 5,
		.pwm_period_ns = PWM_LED_PERIOD,
//		.default_trigger = "timer", // [SIMT-qiukejun-110719] used to accomplish blinking led
	},
	{
		.name		= "red",
		.max_brightness = 40,
		.pwm_id		= 2,
		.pwm_period_ns = PWM_LED_PERIOD,
//		.default_trigger = "timer", // [SIMT-qiukejun-110719] used to accomplish blinking led
	},
	{
		.name		= "green",
		.max_brightness = 40,
		.pwm_id		= 4,
		.pwm_period_ns = PWM_LED_PERIOD,
//		.default_trigger = "timer", // [SIMT-qiukejun-110719] used to accomplish blinking led
	},
};

static struct led_pwm_platform_data pm8058_pwm_leds_data = {
	.num_leds = ARRAY_SIZE(pmic8058_pwm_leds),
	.leds = pmic8058_pwm_leds,
};
#endif


static struct mfd_cell pm8058_subdevs[] = {
	{	.name = "pm8058-keypad",
		.id		= -1,
		.num_resources	= ARRAY_SIZE(resources_keypad),
		.resources	= resources_keypad,
	},
#ifdef CONFIG_LEDS_PMIC8058 
	{	.name = "pm8058-led",
		.id		= -1,
	},
#endif
	{	.name = "pm8058-gpio",
		.id		= -1,
		.platform_data	= &pm8058_gpio_data,
		.data_size	= sizeof(pm8058_gpio_data),
	},
	{	.name = "pm8058-mpp",
		.id		= -1,
		.platform_data	= &pm8058_mpp_data,
		.data_size	= sizeof(pm8058_mpp_data),
	},
	{	.name = "pm8058-pwm",
		.id		= -1,
		.platform_data	= &pm8058_pwm_data,
		.data_size	= sizeof(pm8058_pwm_data),
	},
//[SIMT-lilening-20110804] enable vibrator{
#ifdef CONFIG_PMIC8058_VIBRATOR
	{
		.name = "pm8058-vib",
		.id = -1,
		.platform_data = &pmic_vib_pdata,
		.data_size     = sizeof(pmic_vib_pdata),
	},
#endif
//[SIMT-lilening-20110804] enable vibrator}
#ifdef CONFIG_LEDS_PWM
	{	.name = "leds_pwm",
		.id		= -1,
		.platform_data	= &pm8058_pwm_leds_data,
		.data_size	= sizeof(pm8058_pwm_leds_data),
	},
#endif
	{	.name = "pm8058-nfc",
		.id		= -1,
	},
	{	.name = "pm8058-upl",
		.id		= -1,
	},
};


static struct pm8058_platform_data pm8058_7x30_data = {
	.irq_base = PMIC8058_IRQ_BASE,

	.num_subdevs = ARRAY_SIZE(pm8058_subdevs),
	.sub_devices = pm8058_subdevs,
	.irq_trigger_flags = IRQF_TRIGGER_LOW,
};

static struct i2c_board_info pm8058_boardinfo[] __initdata = {
	{
		I2C_BOARD_INFO("pm8058-core", 0x55),
		.irq = MSM_GPIO_TO_INT(PMIC_GPIO_INT),
		.platform_data = &pm8058_7x30_data,
	},
};

static struct i2c_board_info msm_camera_boardinfo[] __initdata = {
#ifdef CONFIG_MT9D112
	{
		I2C_BOARD_INFO("mt9d112", 0x7A >> 1),
	},
#endif
#ifdef CONFIG_M6MO
	{
		I2C_BOARD_INFO("m6mo", 0x1f),
	},
#endif
#ifdef CONFIG_MT9D111SY
	{
		I2C_BOARD_INFO("mt9d111SY", 0x78 >> 1),
	},
#endif
#ifdef CONFIG_OV7690
	{
		I2C_BOARD_INFO("ov7690", 0x42 >> 1),
	},
#endif
#ifdef CONFIG_WEBCAM_OV9726
	{
		I2C_BOARD_INFO("ov9726", 0x10),
	},
#endif
#ifdef CONFIG_S5K3E2FX
	{
		I2C_BOARD_INFO("s5k3e2fx", 0x20 >> 1),
	},
#endif
#ifdef CONFIG_MT9P012
	{
		I2C_BOARD_INFO("mt9p012", 0x6C >> 1),
	},
#endif
#ifdef CONFIG_VX6953
	{
		I2C_BOARD_INFO("vx6953", 0x20),
	},
#endif
#ifdef CONFIG_MT9E013
	{
		I2C_BOARD_INFO("mt9e013", 0x6C >> 2),
	},
#endif
#ifdef CONFIG_SN12M0PZ
	{
		//I2C_BOARD_INFO("sn12m0pz", 0x34 >> 1),
		I2C_BOARD_INFO("sn12m0pz", 0x1f),
	},
#endif
#if defined(CONFIG_MT9T013) || defined(CONFIG_SENSORS_MT9T013)
	{
		I2C_BOARD_INFO("mt9t013", 0x6C),
	},
#endif

};

#ifdef CONFIG_MSM_CAMERA
//static uint32_t camera_off_vcm_gpio_table[] = {
//GPIO_CFG(1, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* VCM */
//};

static uint32_t camera_off_gpio_table[] = {
	/* parallel CAMERA interfaces */
	GPIO_CFG(0,  0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /*PWDN-VAG  */
	GPIO_CFG(31, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /*RST  */
	GPIO_CFG(143,0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* PWDN */
	GPIO_CFG(33,  0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_16MA), /* LED */
	GPIO_CFG(2,  0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT2 */	
	//GPIO_CFG(3,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT3 */
	GPIO_CFG(4,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT4 */
	GPIO_CFG(5,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT5 */
	GPIO_CFG(6,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT6 */
	GPIO_CFG(7,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT7 */
	GPIO_CFG(8,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT8 */
	GPIO_CFG(9,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT9 */
	GPIO_CFG(10, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT10 */
	GPIO_CFG(11, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT11 */
	GPIO_CFG(12, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* PCLK */
	GPIO_CFG(13, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* HSYNC_IN */
	GPIO_CFG(14, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* VSYNC_IN */
	GPIO_CFG(15, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* MCLK */
};

static uint32_t camera_on_gpio_table[] = {
	/* parallel CAMERA interfaces */
	GPIO_CFG(2,  0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* AF_VDD_EN */
	GPIO_CFG(0,  0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* PWDN-VGA */
	GPIO_CFG(31, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* RST */
	//GPIO_CFG(143,0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* PWDN */
	GPIO_CFG(143,0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* PWDN */
	GPIO_CFG(33,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_16MA), /* LED */
	//GPIO_CFG(3,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT3 */
	GPIO_CFG(4,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT4 */
	GPIO_CFG(5,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT5 */
	GPIO_CFG(6,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT6 */
	GPIO_CFG(7,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT7 */
	GPIO_CFG(8,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT8 */
	GPIO_CFG(9,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT9 */
	GPIO_CFG(10, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT10 */
	GPIO_CFG(11, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* DAT11 */
	GPIO_CFG(12, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* PCLK */
	GPIO_CFG(13, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* HSYNC_IN */
	GPIO_CFG(14, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), /* VSYNC_IN */
	GPIO_CFG(15, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* MCLK */
};

static void config_gpio_table(uint32_t *table, int len)
{
	int n, rc;
	for (n = 0; n < len; n++) {
		rc = gpio_tlmm_config(table[n], GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, table[n], rc);
			break;
		}
	}
}
static int config_camera_on_gpios(void)
{
	config_gpio_table(camera_on_gpio_table,ARRAY_SIZE(camera_on_gpio_table));

	return 0;
}

static void config_camera_off_gpios(void)
{
	config_gpio_table(camera_off_gpio_table,ARRAY_SIZE(camera_off_gpio_table));
}

struct resource msm_camera_resources[] = {
	{
		.start	= 0xA6000000,
		.end	= 0xA6000000 + SZ_1M - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= INT_VFE,
		.end	= INT_VFE,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.flags  = IORESOURCE_DMA,
	}
};

struct msm_camera_device_platform_data msm_camera_device_data = {
	.camera_gpio_on  = config_camera_on_gpios,
	.camera_gpio_off = config_camera_off_gpios,
	.ioext.camifpadphy = 0xAB000000,
	.ioext.camifpadsz  = 0x00000400,
	.ioext.csiphy = 0xA6100000,
	.ioext.csisz  = 0x00000400,
	.ioext.csiirq = INT_CSI,
	.ioclk.mclk_clk_rate = 24000000,
	.ioclk.vfe_clk_rate  = 147456000,
};

static struct msm_camera_sensor_flash_src msm_flash_src_pwm = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_PWM,
	._fsrc.pwm_src.freq  = 1000,
	._fsrc.pwm_src.max_load = 300,
	._fsrc.pwm_src.low_load = 30,
	._fsrc.pwm_src.high_load = 100,
	._fsrc.pwm_src.channel = 7,
};

#ifdef CONFIG_M6MO

static void msm_camera_vreg_enable_m6mo(void)
{

	pr_info("msm_camera_vreg_enable_sm6mo \n");

    //GP15(L22,DVDD)
	vreg_gp15 = vreg_get(NULL, "gp15");
	if (IS_ERR(vreg_gp15)) {
		pr_err("%s: VREG GP15 get failed %ld\n", __func__,PTR_ERR(vreg_gp15));
		vreg_gp15 = NULL;
		return;
	}
	if (vreg_set_level(vreg_gp15, 1200)) {
		pr_err("%s: VREG GP15 set failed\n", __func__);
		//goto gp7_put;
        return;
	}
	if (vreg_enable(vreg_gp15)) {
		pr_err("%s: VREG GP15 enable failed\n", __func__);
		//goto gp7_put;
        return;
    }
    //GP5(L23,DVDD)
	vreg_gp5 = vreg_get(NULL, "gp5");
	if (IS_ERR(vreg_gp5)) {
		pr_err("%s: VREG GP5 get failed %ld\n", __func__,PTR_ERR(vreg_gp15));
		vreg_gp15 = NULL;
		return;
	}
	if (vreg_set_level(vreg_gp5, 1200)) {
		pr_err("%s: VREG GP5 set failed\n", __func__);
		
        return;
	}
	if (vreg_enable(vreg_gp5)) {
		pr_err("%s: VREG GP5 enable failed\n", __func__);
		
        return;
    }
    //wlan2(L19,DOVDD)    
	vreg_wlan2 = vreg_get(NULL, "wlan2");
	if (IS_ERR(vreg_wlan2)) {
		pr_err("%s: VREG vreg_wlan2 get failed %ld\n", __func__,
			PTR_ERR(vreg_wlan2));
		vreg_wlan2 = NULL;
        return;
	}
	if (vreg_set_level(vreg_wlan2, 1800)) {
		pr_err("%s: VREG vreg_wlan2 set failed\n", __func__);
		//goto lvsw1_put;
        return;
	}
	if (vreg_enable(vreg_wlan2)){
		pr_err("%s: VREG vreg_wlan2 enable failed\n", __func__);
        return;
    }
    //lvsw1(LVS1,DOVDD)
	vreg_lvsw1 = vreg_get(NULL, "lvsw1");
	if (IS_ERR(vreg_lvsw1)) {
		pr_err("%s: VREG LVSW1 get failed %ld\n", __func__,
			PTR_ERR(vreg_lvsw1));
		vreg_lvsw1 = NULL;
        return;
	}
	if (vreg_set_level(vreg_lvsw1, 1800)) {
		pr_err("%s: VREG LVSW1 set failed\n", __func__);
		//goto lvsw1_put;
        return;
	}
	if (vreg_enable(vreg_lvsw1)){
		pr_err("%s: VREG LVSW1 enable failed\n", __func__);
        return;
    }


    //GP7(L8,DVDD)
	vreg_gp7 = vreg_get(NULL, "gp7");
	if (IS_ERR(vreg_gp7)) {
		pr_err("%s: VREG GP7 get failed %ld\n", __func__,PTR_ERR(vreg_gp7));
		vreg_gp7 = NULL;
		return;
	}
	if (vreg_set_level(vreg_gp7,2800)) {
		pr_err("%s: VREG GP7 set failed\n", __func__);
		//goto gp7_put;
        return;
	}
	if (vreg_enable(vreg_gp7)) {
		pr_err("%s: VREG GP7 enable failed\n", __func__);
		//goto gp7_put;
        return;
    }
    
    //GP2(L11,AVDD)
	vreg_gp2 = vreg_get(NULL, "gp2");
	if (IS_ERR(vreg_gp2)) {
		pr_err("%s: VREG GP2 get failed %ld\n", __func__,PTR_ERR(vreg_gp2));
		vreg_gp2 = NULL;
		return;
	}
	if (vreg_set_level(vreg_gp2, 2800)) {
		pr_err("%s: VREG GP2 set failed\n", __func__);
		//goto gp2_put;
        return;
	}
	if (vreg_enable(vreg_gp2)) {
		pr_err("%s: VREG GP2 enable failed\n", __func__);
		//goto gp2_put;
        return;
    }


	msleep(10);
/*
	gpio_request(143, "m6mo");   // front PWDN
	gpio_direction_output(143, 1);
	gpio_free(143);
*/
	msleep(10);

	gpio_request(0, "m6mo");     // back ON
	gpio_direction_output(0, 1);
	gpio_free(0);

	msleep(10);

    
	return;    

}

static void msm_camera_vreg_disable_m6mo(void)
{
	pr_info("msm_camera_vreg_disable_m6mo \n");


	gpio_request(31, "m6mo");
	gpio_direction_output(31, 0);
	gpio_free(31);
	/*
	gpio_request(143, "m6mo");
	gpio_direction_output(143, 0);
	gpio_free(143);
	*/
	msleep(10);

	gpio_request(0, "m6mo");
	gpio_direction_output(0, 0);
	gpio_free(0);

	msleep(10);

	if (vreg_gp2) {
		vreg_disable(vreg_gp2);
		vreg_put(vreg_gp2);
	}

	if (vreg_gp7) {
		vreg_disable(vreg_gp7);
		vreg_put(vreg_gp7);
	}

	if (vreg_lvsw1) {
		vreg_disable(vreg_lvsw1);
		vreg_put(vreg_lvsw1);
	}
	
	if (vreg_wlan2) {
		vreg_disable(vreg_wlan2);
		vreg_put(vreg_wlan2);
	}
	
	if (vreg_gp5) {
		vreg_disable(vreg_gp5);
		vreg_put(vreg_gp5);
	}
	
	if (vreg_gp15) {
		vreg_disable(vreg_gp15);
		vreg_put(vreg_gp15);
	}

	msleep(10);

}


static int config_camera_on_gpios_m6mo(void)
{
	config_gpio_table(camera_on_gpio_table,ARRAY_SIZE(camera_on_gpio_table));
    msm_camera_vreg_enable_m6mo();
	return 0;
}

static void config_camera_off_gpios_m6mo(void)
{
	config_gpio_table(camera_off_gpio_table,ARRAY_SIZE(camera_off_gpio_table));
    msm_camera_vreg_disable_m6mo();

}

struct msm_camera_device_platform_data msm_camera_device_data_m6mo = {
	.camera_gpio_on  = config_camera_on_gpios_m6mo,
	.camera_gpio_off = config_camera_off_gpios_m6mo,
	.ioext.camifpadphy = 0xAB000000,
	.ioext.camifpadsz  = 0x00000400,
	.ioext.csiphy = 0xA6100000,
	.ioext.csisz  = 0x00000400,
	.ioext.csiirq = INT_CSI,
	.ioclk.mclk_clk_rate = 24000000,//24000000,
	.ioclk.vfe_clk_rate  = 122880000,
};

static struct msm_camera_sensor_flash_src msm_flash_src_current_driver_m6mo = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_CURRENT_DRIVER,
	._fsrc.current_driver_src.low_current = 210,
	._fsrc.current_driver_src.high_current = 700,
};

static struct msm_camera_sensor_flash_data flash_m6mo = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src_current_driver_m6mo
};
static struct msm_camera_sensor_info msm_camera_sensor_m6mo_data = {
	.sensor_name    = "m6mo",
	.sensor_reset   = 31,
	.sensor_pwd     = 143,
	.vcm_pwd        = 1,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data_m6mo,
	.flash_data     = &flash_m6mo,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources),
	.csi_if         = 0
};

static struct platform_device msm_camera_sensor_m6mo = {
	.name      = "msm_camera_m6mo",
	.dev       = {
		.platform_data = &msm_camera_sensor_m6mo_data,
	},
};
#endif
#ifdef CONFIG_SN12M0PZ

static void msm_camera_vreg_enable_sn12m0pz(void)
{

	pr_info("msm_camera_vreg_enable_sn12m0pz \n");

    //GP15(L22,DVDD)
	vreg_gp15 = vreg_get(NULL, "gp15");
	if (IS_ERR(vreg_gp15)) {
		pr_err("%s: VREG GP15 get failed %ld\n", __func__,PTR_ERR(vreg_gp15));
		vreg_gp15 = NULL;
		return;
	}
	if (vreg_set_level(vreg_gp15, 1200)) {
		pr_err("%s: VREG GP15 set failed\n", __func__);
		//goto gp7_put;
        return;
	}
	if (vreg_enable(vreg_gp15)) {
		pr_err("%s: VREG GP15 enable failed\n", __func__);
		//goto gp7_put;
        return;
    }
    
    //wlan2(L19,DOVDD)    
	vreg_wlan2 = vreg_get(NULL, "wlan2");
	if (IS_ERR(vreg_wlan2)) {
		pr_err("%s: VREG vreg_wlan2 get failed %ld\n", __func__,
			PTR_ERR(vreg_wlan2));
		vreg_wlan2 = NULL;
        return;
	}
	if (vreg_set_level(vreg_wlan2, 1800)) {
		pr_err("%s: VREG vreg_wlan2 set failed\n", __func__);
		//goto lvsw1_put;
        return;
	}
	if (vreg_enable(vreg_wlan2)){
		pr_err("%s: VREG vreg_wlan2 enable failed\n", __func__);
        return;
    }

    //GP7(L8,DVDD)
	vreg_gp7 = vreg_get(NULL, "gp7");
	if (IS_ERR(vreg_gp7)) {
		pr_err("%s: VREG GP7 get failed %ld\n", __func__,PTR_ERR(vreg_gp7));
		vreg_gp7 = NULL;
		return;
	}
	if (vreg_set_level(vreg_gp7,2800)) {
		pr_err("%s: VREG GP7 set failed\n", __func__);
		//goto gp7_put;
        return;
	}
	if (vreg_enable(vreg_gp7)) {
		pr_err("%s: VREG GP7 enable failed\n", __func__);
		//goto gp7_put;
        return;
    }
    
    //GP2(L11,AVDD)
	vreg_gp2 = vreg_get(NULL, "gp2");
	if (IS_ERR(vreg_gp2)) {
		pr_err("%s: VREG GP2 get failed %ld\n", __func__,PTR_ERR(vreg_gp2));
		vreg_gp2 = NULL;
		return;
	}
	if (vreg_set_level(vreg_gp2, 2800)) {
		pr_err("%s: VREG GP2 set failed\n", __func__);
		//goto gp2_put;
        return;
	}
	if (vreg_enable(vreg_gp2)) {
		pr_err("%s: VREG GP2 enable failed\n", __func__);
		//goto gp2_put;
        return;
    }


	msleep(10);

	gpio_request(143, "sn12m0pz");   // front PWDN
	gpio_direction_output(143, 1);
	gpio_free(143);

	msleep(10);

	gpio_request(0, "sn12m0pz");     // back ON
	gpio_direction_output(0, 1);
	gpio_free(0);

	msleep(10);

    
	return;    

}

static void msm_camera_vreg_disable_sn12m0pz(void)
{
	pr_info("msm_camera_vreg_disable_sn12m0pz \n");


	gpio_request(31, "sn12m0pz");
	gpio_direction_output(31, 0);
	gpio_free(31);
	
	gpio_request(143, "sn12m0pz");
	gpio_direction_output(143, 0);
	gpio_free(143);

	msleep(10);

	gpio_request(0, "sn12m0pz");
	gpio_direction_output(0, 0);
	gpio_free(0);

	msleep(10);

	if (vreg_gp2) {
		vreg_disable(vreg_gp2);
		vreg_put(vreg_gp2);
	}

	if (vreg_gp7) {
		vreg_disable(vreg_gp7);
		vreg_put(vreg_gp7);
	}

	if (vreg_wlan2) {
		vreg_disable(vreg_wlan2);
		vreg_put(vreg_wlan2);
	}
	
	if (vreg_gp15) {
		vreg_disable(vreg_gp15);
		vreg_put(vreg_gp15);
	}

	msleep(10);

}


static int config_camera_on_gpios_sn12m0pz(void)
{
	config_gpio_table(camera_on_gpio_table,ARRAY_SIZE(camera_on_gpio_table));
    msm_camera_vreg_enable_sn12m0pz();
	return 0;
}

static void config_camera_off_gpios_sn12m0pz(void)
{
	config_gpio_table(camera_off_gpio_table,ARRAY_SIZE(camera_off_gpio_table));
    msm_camera_vreg_disable_sn12m0pz();

}

struct msm_camera_device_platform_data msm_camera_device_data_sn12m0pz = {
	.camera_gpio_on  = config_camera_on_gpios_sn12m0pz,
	.camera_gpio_off = config_camera_off_gpios_sn12m0pz,
	.ioext.camifpadphy = 0xAB000000,
	.ioext.camifpadsz  = 0x00000400,
	.ioext.csiphy = 0xA6100000,
	.ioext.csisz  = 0x00000400,
	.ioext.csiirq = INT_CSI,
	.ioclk.mclk_clk_rate = 12000000,
	.ioclk.vfe_clk_rate  = 122880000,
};

static struct msm_camera_sensor_flash_src msm_flash_src_current_driver = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_CURRENT_DRIVER,
	._fsrc.current_driver_src.low_current = 210,
	._fsrc.current_driver_src.high_current = 700,
};

static struct msm_camera_sensor_flash_data flash_sn12m0pz = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src_current_driver
};
static struct msm_camera_sensor_info msm_camera_sensor_sn12m0pz_data = {
	.sensor_name    = "sn12m0pz",
	.sensor_reset   = 31,
	.sensor_pwd     = 143,
	.vcm_pwd        = 1,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data_sn12m0pz,
	.flash_data     = &flash_sn12m0pz,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources),
	.csi_if         = 0
};

static struct platform_device msm_camera_sensor_sn12m0pz = {
	.name      = "msm_camera_sn12m0pz",
	.dev       = {
		.platform_data = &msm_camera_sensor_sn12m0pz_data,
	},
};
#endif


#ifdef CONFIG_OV7690
static void msm_camera_vreg_enable_ov7690(void)
{

	pr_info("msm_camera_vreg_enable_ov7690 \n");

    //GP15(L22,DVDD)
	vreg_gp15 = vreg_get(NULL, "gp15");
	if (IS_ERR(vreg_gp15)) {
		pr_err("%s: VREG GP15 get failed %ld\n", __func__,PTR_ERR(vreg_gp15));
		vreg_gp15 = NULL;
		return;
	}
	if (vreg_set_level(vreg_gp15, 1200)) {
		pr_err("%s: VREG GP15 set failed\n", __func__);
		//goto gp7_put;
        return;
	}
	if (vreg_enable(vreg_gp15)) {
		pr_err("%s: VREG GP15 enable failed\n", __func__);
		//goto gp7_put;
        return;
    }
    
    //wlan2(L19,DOVDD)    
	vreg_wlan2 = vreg_get(NULL, "wlan2");
	if (IS_ERR(vreg_wlan2)) {
		pr_err("%s: VREG vreg_wlan2 get failed %ld\n", __func__,
			PTR_ERR(vreg_wlan2));
		vreg_wlan2 = NULL;
        return;
	}
	if (vreg_set_level(vreg_wlan2, 1800)) {
		pr_err("%s: VREG vreg_wlan2 set failed\n", __func__);
		//goto lvsw1_put;
        return;
	}
	if (vreg_enable(vreg_wlan2)){
		pr_err("%s: VREG vreg_wlan2 enable failed\n", __func__);
        return;
    }

    //GP7(L8,DVDD)
	vreg_gp7 = vreg_get(NULL, "gp7");
	if (IS_ERR(vreg_gp7)) {
		pr_err("%s: VREG GP7 get failed %ld\n", __func__,PTR_ERR(vreg_gp7));
		vreg_gp7 = NULL;
		return;
	}
	if (vreg_set_level(vreg_gp7,2800)) {
		pr_err("%s: VREG GP7 set failed\n", __func__);
		//goto gp7_put;
        return;
	}
	if (vreg_enable(vreg_gp7)) {
		pr_err("%s: VREG GP7 enable failed\n", __func__);
		//goto gp7_put;
        return;
    }
    
    //GP2(L11,AVDD)
	vreg_gp2 = vreg_get(NULL, "gp2");
	if (IS_ERR(vreg_gp2)) {
		pr_err("%s: VREG GP2 get failed %ld\n", __func__,PTR_ERR(vreg_gp2));
		vreg_gp2 = NULL;
		return;
	}
	if (vreg_set_level(vreg_gp2, 2800)) {
		pr_err("%s: VREG GP2 set failed\n", __func__);
		//goto gp2_put;
        return;
	}
	if (vreg_enable(vreg_gp2)) {
		pr_err("%s: VREG GP2 enable failed\n", __func__);
		//goto gp2_put;
        return;
    }


	msleep(10);

	gpio_request(0, "ov7690");     // back PWDN
	gpio_direction_output(0, 0);
	gpio_free(0);

	msleep(10);

	gpio_request(143, "ov7690");   // front ON
	gpio_direction_output(143, 0);
	gpio_free(143);

	msleep(10);
   
	return;    

}

static void msm_camera_vreg_disable_ov7690(void)
{
	pr_info("msm_camera_vreg_disable_ov7690 \n");

	gpio_request(143, "ov7690");
	gpio_direction_output(143, 0);
	gpio_free(143);

	msleep(10);

	gpio_request(0, "ov7690");
	gpio_direction_output(0, 0);
	gpio_free(0);

	msleep(10);

	if (vreg_gp2) {
		vreg_disable(vreg_gp2);
		vreg_put(vreg_gp2);
	}

	if (vreg_gp7) {
		vreg_disable(vreg_gp7);
		vreg_put(vreg_gp7);
	}

	if (vreg_wlan2) {
		vreg_disable(vreg_wlan2);
		vreg_put(vreg_wlan2);
	}
	
	if (vreg_gp15) {
		vreg_disable(vreg_gp15);
		vreg_put(vreg_gp15);
	}

	msleep(10);

}

static int config_camera_on_gpios_ov7690(void)
{
	config_gpio_table(camera_on_gpio_table,ARRAY_SIZE(camera_on_gpio_table));
    msm_camera_vreg_enable_ov7690();
	return 0;
}

static void config_camera_off_gpios_ov7690(void)
{
	config_gpio_table(camera_off_gpio_table,ARRAY_SIZE(camera_off_gpio_table));
    msm_camera_vreg_disable_ov7690();

}

struct msm_camera_device_platform_data msm_camera_device_data_ov7690 = {
	.camera_gpio_on  = config_camera_on_gpios_ov7690,
	.camera_gpio_off = config_camera_off_gpios_ov7690,
	.ioext.camifpadphy = 0xAB000000,
	.ioext.camifpadsz  = 0x00000400,
	.ioext.csiphy = 0xA6100000,
	.ioext.csisz  = 0x00000400,
	.ioext.csiirq = INT_CSI,
	.ioclk.mclk_clk_rate = 24000000,
	.ioclk.vfe_clk_rate  = 122880000,
};

static struct msm_camera_sensor_flash_data flash_ov7690 = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src_pwm
};

static struct msm_camera_sensor_info msm_camera_sensor_ov7690_data = {
	.sensor_name    = "ov7690",
	.sensor_reset   = 0,
	.sensor_pwd     = 0,
	.vcm_pwd        = 1,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data_ov7690,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources),
	.flash_data     = &flash_ov7690,
	.csi_if         = 0
};

static struct platform_device msm_camera_sensor_ov7690 = {
	.name      = "msm_camera_ov7690",
	.dev       = {
		.platform_data = &msm_camera_sensor_ov7690_data,
	},
};
#endif

#ifdef CONFIG_MT9D112

static void msm_camera_vreg_enable_mt9d112(void)
{

	pr_info("msm_camera_vreg_enable_mt9d112 \n");
	gpio_request(2, "mt9d112");
	gpio_request(143, "mt9d112");
	gpio_direction_output(2, 1);
	gpio_direction_output(143, 0);
	gpio_free(2);
	gpio_free(143);
    //lvsw1(LVS1,DOVDD)
	vreg_lvsw1 = vreg_get(NULL, "lvsw1");
	if (IS_ERR(vreg_lvsw1)) {
		pr_err("%s: VREG LVSW1 get failed %ld\n", __func__,
			PTR_ERR(vreg_lvsw1));
		vreg_lvsw1 = NULL;
        return;
	}
	if (vreg_set_level(vreg_lvsw1, 1800)) {
		pr_err("%s: VREG LVSW1 set failed\n", __func__);
		//goto lvsw1_put;
        return;
	}
	if (vreg_enable(vreg_lvsw1)){
		pr_err("%s: VREG LVSW1 enable failed\n", __func__);
        return;
    }

    //GP7(L8,DVDD)
	vreg_gp7 = vreg_get(NULL, "gp7");
	if (IS_ERR(vreg_gp7)) {
		pr_err("%s: VREG GP7 get failed %ld\n", __func__,PTR_ERR(vreg_gp7));
		vreg_gp7 = NULL;
		return;
	}
	if (vreg_set_level(vreg_gp7, 1800)) {
		pr_err("%s: VREG GP7 set failed\n", __func__);
		//goto gp7_put;
        return;
	}
	if (vreg_enable(vreg_gp7)) {
		pr_err("%s: VREG GP7 enable failed\n", __func__);
		//goto gp7_put;
        return;
    }

    //GP2(L11,AVDD)
	vreg_gp2 = vreg_get(NULL, "gp2");
	if (IS_ERR(vreg_gp2)) {
		pr_err("%s: VREG GP2 get failed %ld\n", __func__,PTR_ERR(vreg_gp2));
		vreg_gp2 = NULL;
		return;
	}
	if (vreg_set_level(vreg_gp2, 2800)) {
		pr_err("%s: VREG GP2 set failed\n", __func__);
		//goto gp2_put;
        return;
	}
	if (vreg_enable(vreg_gp2)) {
		pr_err("%s: VREG GP2 enable failed\n", __func__);
		//goto gp2_put;
        return;
    }

	return;

}

static void msm_camera_vreg_disable_mt9d112(void)
{
	pr_info("msm_camera_vreg_disable_mt9d112 \n");


	if (vreg_gp2) {
		vreg_disable(vreg_gp2);
		vreg_put(vreg_gp2);
	}

	if (vreg_gp7) {
		vreg_disable(vreg_gp7);
		vreg_put(vreg_gp7);
	}

	if (vreg_lvsw1) {
		vreg_disable(vreg_lvsw1);
		vreg_put(vreg_lvsw1);
	}
	gpio_request(0, "mt9d112");
	gpio_request(2, "mt9d112");
	gpio_request(143, "mt9d112");
	gpio_request(15, "mt9d112");

	gpio_direction_output(0, 0); //reset
	gpio_direction_output(2, 0);//af_vdd_en
	gpio_direction_output(143, 0);//pwd
	gpio_direction_output(15, 0); //mclk

	gpio_free(0);
	gpio_free(2);
	gpio_free(143);
	gpio_free(15);


}

static int config_camera_on_gpios_mt9d112(void)
{
	config_gpio_table(camera_on_gpio_table,ARRAY_SIZE(camera_on_gpio_table));
    msm_camera_vreg_enable_mt9d112();
	return 0;
}

static void config_camera_off_gpios_mt9d112(void)
{
	config_gpio_table(camera_off_gpio_table,ARRAY_SIZE(camera_off_gpio_table));
    msm_camera_vreg_disable_mt9d112();

}

struct msm_camera_device_platform_data msm_camera_device_data_mt9d112 = {
	.camera_gpio_on  = config_camera_on_gpios_mt9d112,
	.camera_gpio_off = config_camera_off_gpios_mt9d112,
	.ioext.camifpadphy = 0xAB000000,
	.ioext.camifpadsz  = 0x00000400,
	.ioext.csiphy = 0xA6100000,
	.ioext.csisz  = 0x00000400,
	.ioext.csiirq = INT_CSI,
	.ioclk.mclk_clk_rate = 24000000,
	.ioclk.vfe_clk_rate  = 122880000,
};

static struct msm_camera_sensor_flash_data flash_mt9d112 = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src_pwm
};

static struct msm_camera_sensor_info msm_camera_sensor_mt9d112_data = {
	.sensor_name    = "mt9d112",
	.sensor_reset   = 0,
	.sensor_pwd     = 143,
	.vcm_pwd        = 1,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data_mt9d112,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources),
	.flash_data     = &flash_mt9d112,
	.csi_if         = 0
};

static struct platform_device msm_camera_sensor_mt9d112 = {
	.name      = "msm_camera_mt9d112",
	.dev       = {
		.platform_data = &msm_camera_sensor_mt9d112_data,
	},
};
#endif

#ifdef CONFIG_MT9D111SY

static void msm_camera_vreg_enable_mt9d112sy(void)
{

	pr_info("msm_camera_vreg_enable_mt9d112sy \n");
	gpio_request(2, "mt9d112");
	gpio_request(143, "mt9d112");
	gpio_direction_output(2, 1);
	gpio_direction_output(143, 0);
	gpio_free(2);
	gpio_free(143);
    //lvsw1(LVS1,DOVDD)
	vreg_lvsw1 = vreg_get(NULL, "lvsw1");
	if (IS_ERR(vreg_lvsw1)) {
		pr_err("%s: VREG LVSW1 get failed %ld\n", __func__,
			PTR_ERR(vreg_lvsw1));
		vreg_lvsw1 = NULL;
        return;
	}
	if (vreg_set_level(vreg_lvsw1, 1800)) {
		pr_err("%s: VREG LVSW1 set failed\n", __func__);
		//goto lvsw1_put;
        return;
	}
	if (vreg_enable(vreg_lvsw1)){
		pr_err("%s: VREG LVSW1 enable failed\n", __func__);
        return;
    }

    //GP7(L8,DVDD)
	vreg_gp7 = vreg_get(NULL, "gp7");
	if (IS_ERR(vreg_gp7)) {
		pr_err("%s: VREG GP7 get failed %ld\n", __func__,PTR_ERR(vreg_gp7));
		vreg_gp7 = NULL;
		return;
	}
	if (vreg_set_level(vreg_gp7, 1800)) {
		pr_err("%s: VREG GP7 set failed\n", __func__);
		//goto gp7_put;
        return;
	}
	if (vreg_enable(vreg_gp7)) {
		pr_err("%s: VREG GP7 enable failed\n", __func__);
		//goto gp7_put;
        return;
    }

    //GP2(L11,AVDD)
	vreg_gp2 = vreg_get(NULL, "gp2");
	if (IS_ERR(vreg_gp2)) {
		pr_err("%s: VREG GP2 get failed %ld\n", __func__,PTR_ERR(vreg_gp2));
		vreg_gp2 = NULL;
		return;
	}
	if (vreg_set_level(vreg_gp2, 2800)) {
		pr_err("%s: VREG GP2 set failed\n", __func__);
		//goto gp2_put;
        return;
	}
	if (vreg_enable(vreg_gp2)) {
		pr_err("%s: VREG GP2 enable failed\n", __func__);
		//goto gp2_put;
        return;
    }

	return;

}

static void msm_camera_vreg_disable_mt9d112sy(void)
{
	pr_info("msm_camera_vreg_disable_mt9d112sy \n");


	if (vreg_gp2) {
		vreg_disable(vreg_gp2);
		vreg_put(vreg_gp2);
	}

	if (vreg_gp7) {
		vreg_disable(vreg_gp7);
		vreg_put(vreg_gp7);
	}

	if (vreg_lvsw1) {
		vreg_disable(vreg_lvsw1);
		vreg_put(vreg_lvsw1);
	}
	gpio_request(0, "mt9d112");
	gpio_request(2, "mt9d112");
	gpio_request(143, "mt9d112");
	gpio_request(15, "mt9d112");

	gpio_direction_output(0, 0); //reset
	gpio_direction_output(2, 0);//af_vdd_en
	gpio_direction_output(143, 0);//pwd
	gpio_direction_output(15, 0); //mclk

	gpio_free(0);
	gpio_free(2);
	gpio_free(143);
	gpio_free(15);


}

static int config_camera_on_gpios_mt9d112sy(void)
{
	config_gpio_table(camera_on_gpio_table,ARRAY_SIZE(camera_on_gpio_table));
    msm_camera_vreg_enable_mt9d112sy();
	return 0;
}

static void config_camera_off_gpios_mt9d112sy(void)
{
	config_gpio_table(camera_off_gpio_table,ARRAY_SIZE(camera_off_gpio_table));
    msm_camera_vreg_disable_mt9d112sy();

}

struct msm_camera_device_platform_data msm_camera_device_data_mt9d112sy = {
	.camera_gpio_on  = config_camera_on_gpios_mt9d112sy,
	.camera_gpio_off = config_camera_off_gpios_mt9d112sy,
	.ioext.camifpadphy = 0xAB000000,
	.ioext.camifpadsz  = 0x00000400,
	.ioext.csiphy = 0xA6100000,
	.ioext.csisz  = 0x00000400,
	.ioext.csiirq = INT_CSI,
	.ioclk.mclk_clk_rate = 24000000,
	.ioclk.vfe_clk_rate  = 122880000,
};

static struct msm_camera_sensor_flash_data flash_mt9d111SY = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src_pwm
};
static struct msm_camera_sensor_info msm_camera_sensor_mt9d111SY_data = {
	.sensor_name    = "mt9d111SY",
	.sensor_reset   = 0,
	.sensor_pwd     = 143,
	.vcm_pwd        = 1,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data_mt9d112sy,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources),
	.flash_data     = &flash_mt9d111SY,
	.csi_if         = 0
};
static struct platform_device msm_camera_sensor_mt9d111SY = {
	.name      = "msm_camera_mt9d111SY",
	.dev       = {
		.platform_data = &msm_camera_sensor_mt9d111SY_data,
	},
};
#endif
#ifdef CONFIG_WEBCAM_OV9726
static struct msm_camera_sensor_flash_data flash_ov9726 = {
	.flash_type	= MSM_CAMERA_FLASH_LED,
	.flash_src	= &msm_flash_src_pwm
};
static struct msm_camera_sensor_info msm_camera_sensor_ov9726_data = {
	.sensor_name	= "ov9726",
	.sensor_reset	= 0,
	.sensor_pwd	= 85,
	.vcm_pwd	= 1,
	.vcm_enable	= 0,
	.pdata		= &msm_camera_device_data,
	.resource	= msm_camera_resources,
	.num_resources	= ARRAY_SIZE(msm_camera_resources),
	.flash_data	= &flash_ov9726,
	.sensor_platform_info = &ov9726_sensor_7630_info,
	.csi_if		= 1
};
struct platform_device msm_camera_sensor_ov9726 = {
	.name	= "msm_camera_ov9726",
	.dev	= {
		.platform_data = &msm_camera_sensor_ov9726_data,
	},
};
#endif

#ifdef CONFIG_S5K3E2FX
static struct msm_camera_sensor_flash_data flash_s5k3e2fx = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src_pwm,
};

static struct msm_camera_sensor_info msm_camera_sensor_s5k3e2fx_data = {
	.sensor_name    = "s5k3e2fx",
	.sensor_reset   = 0,
	.sensor_pwd     = 85,
	.vcm_pwd        = 1,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources),
	.flash_data     = &flash_s5k3e2fx,
	.csi_if         = 0
};

static struct platform_device msm_camera_sensor_s5k3e2fx = {
	.name      = "msm_camera_s5k3e2fx",
	.dev       = {
		.platform_data = &msm_camera_sensor_s5k3e2fx_data,
	},
};
#endif

#ifdef CONFIG_MT9P012
static struct msm_camera_sensor_flash_data flash_mt9p012 = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src_pwm
};

static struct msm_camera_sensor_info msm_camera_sensor_mt9p012_data = {
	.sensor_name    = "mt9p012",
	.sensor_reset   = 0,
	.sensor_pwd     = 85,
	.vcm_pwd        = 1,
	.vcm_enable     = 1,
	.pdata          = &msm_camera_device_data,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources),
	.flash_data     = &flash_mt9p012,
	.csi_if         = 0
};

static struct platform_device msm_camera_sensor_mt9p012 = {
	.name      = "msm_camera_mt9p012",
	.dev       = {
		.platform_data = &msm_camera_sensor_mt9p012_data,
	},
};
#endif

#ifdef CONFIG_MT9E013
static struct msm_camera_sensor_flash_data flash_mt9e013 = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src_pwm
};

static struct msm_camera_sensor_info msm_camera_sensor_mt9e013_data = {
	.sensor_name    = "mt9e013",
	.sensor_reset   = 0,
	.sensor_pwd     = 85,
	.vcm_pwd        = 1,
	.vcm_enable     = 1,
	.pdata          = &msm_camera_device_data,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources),
	.flash_data     = &flash_mt9e013,
	.csi_if         = 1
};

static struct platform_device msm_camera_sensor_mt9e013 = {
	.name      = "msm_camera_mt9e013",
	.dev       = {
		.platform_data = &msm_camera_sensor_mt9e013_data,
	},
};
#endif

#ifdef CONFIG_VX6953
static struct msm_camera_sensor_flash_data flash_vx6953 = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src_pwm
};
static struct msm_camera_sensor_info msm_camera_sensor_vx6953_data = {
	.sensor_name    = "vx6953",
	.sensor_reset   = 0,
	.sensor_pwd     = 85,
	.vcm_pwd        = 1,
	.vcm_enable		= 0,
	.pdata          = &msm_camera_device_data,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources),
	.flash_data     = &flash_vx6953,
	.csi_if         = 1
};
static struct platform_device msm_camera_sensor_vx6953 = {
	.name  	= "msm_camera_vx6953",
	.dev   	= {
		.platform_data = &msm_camera_sensor_vx6953_data,
	},
};
#endif

#ifdef CONFIG_MT9T013
static struct msm_camera_sensor_flash_data flash_mt9t013 = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src_pwm
};

static struct msm_camera_sensor_info msm_camera_sensor_mt9t013_data = {
	.sensor_name    = "mt9t013",
	.sensor_reset   = 0,
	.sensor_pwd     = 85,
	.vcm_pwd        = 1,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources),
	.flash_data     = &flash_mt9t013,
	.csi_if         = 1
};

static struct platform_device msm_camera_sensor_mt9t013 = {
	.name      = "msm_camera_mt9t013",
	.dev       = {
		.platform_data = &msm_camera_sensor_mt9t013_data,
	},
};
#endif

#ifdef CONFIG_MSM_GEMINI
static struct resource msm_gemini_resources[] = {
	{
		.start  = 0xA3A00000,
		.end    = 0xA3A00000 + 0x0150 - 1,
		.flags  = IORESOURCE_MEM,
	},
	{
		.start  = INT_JPEG,
		.end    = INT_JPEG,
		.flags  = IORESOURCE_IRQ,
	},
};

static struct platform_device msm_gemini_device = {
	.name           = "msm_gemini",
	.resource       = msm_gemini_resources,
	.num_resources  = ARRAY_SIZE(msm_gemini_resources),
};
#endif

#ifdef CONFIG_MSM_VPE
static struct resource msm_vpe_resources[] = {
	{
		.start	= 0xAD200000,
		.end	= 0xAD200000 + SZ_1M - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= INT_VPE,
		.end	= INT_VPE,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device msm_vpe_device = {
       .name = "msm_vpe",
       .id   = 0,
       .num_resources = ARRAY_SIZE(msm_vpe_resources),
       .resource = msm_vpe_resources,
};
#endif

#endif /*CONFIG_MSM_CAMERA*/

#ifdef CONFIG_MSM7KV2_AUDIO
static uint32_t audio_pamp_gpio_config =
   GPIO_CFG(82, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA);

static int __init snddev_poweramp_gpio_init(void)
{
	int rc;

	pr_info("snddev_poweramp_gpio_init \n");
	rc = gpio_tlmm_config(audio_pamp_gpio_config, GPIO_CFG_ENABLE);
	if (rc) {
		printk(KERN_ERR
			"%s: gpio_tlmm_config(%#x)=%d\n",
			__func__, audio_pamp_gpio_config, rc);
	}
	return rc;
}

void msm_snddev_tx_route_config(void)
{
	pr_debug("%s()\n", __func__);
}

void msm_snddev_tx_route_deconfig(void)
{
	pr_debug("%s()\n", __func__);
}

void msm_snddev_poweramp_on(void)
{
	gpio_set_value(82, 1);	/* enable spkr poweramp */
	pr_info("%s: power on amplifier\n", __func__);
}

void msm_snddev_poweramp_off(void)
{
	gpio_set_value(82, 0);	/* disable spkr poweramp */
	pr_info("%s: power off amplifier\n", __func__);
}

static struct vreg *snddev_vreg_ncp, *snddev_vreg_gp4;

void msm_snddev_hsed_voltage_on(void)
{
	int rc;

	snddev_vreg_gp4 = vreg_get(NULL, "gp4");
	if (IS_ERR(snddev_vreg_gp4)) {
		pr_err("%s: vreg_get(%s) failed (%ld)\n",
		__func__, "gp4", PTR_ERR(snddev_vreg_gp4));
		return;
	}
	rc = vreg_enable(snddev_vreg_gp4);
	if (rc)
		pr_err("%s: vreg_enable(gp4) failed (%d)\n", __func__, rc);

	snddev_vreg_ncp = vreg_get(NULL, "ncp");
	if (IS_ERR(snddev_vreg_ncp)) {
		pr_err("%s: vreg_get(%s) failed (%ld)\n",
		__func__, "ncp", PTR_ERR(snddev_vreg_ncp));
		return;
	}
	rc = vreg_enable(snddev_vreg_ncp);
	if (rc)
		pr_err("%s: vreg_enable(ncp) failed (%d)\n", __func__, rc);
}

void msm_snddev_hsed_voltage_off(void)
{
	int rc;

	if (IS_ERR(snddev_vreg_ncp)) {
		pr_err("%s: vreg_get(%s) failed (%ld)\n",
		__func__, "ncp", PTR_ERR(snddev_vreg_ncp));
		return;
	}
	rc = vreg_disable(snddev_vreg_ncp);
	if (rc)
		pr_err("%s: vreg_disable(ncp) failed (%d)\n", __func__, rc);
	vreg_put(snddev_vreg_ncp);

	if (IS_ERR(snddev_vreg_gp4)) {
		pr_err("%s: vreg_get(%s) failed (%ld)\n",
		__func__, "gp4", PTR_ERR(snddev_vreg_gp4));
		return;
	}
	rc = vreg_disable(snddev_vreg_gp4);
	if (rc)
		pr_err("%s: vreg_disable(gp4) failed (%d)\n", __func__, rc);

	vreg_put(snddev_vreg_gp4);

}

static unsigned aux_pcm_gpio_on[] = {
	GPIO_CFG(138, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),   /* PCM_DOUT */
	GPIO_CFG(139, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),   /* PCM_DIN  */
	GPIO_CFG(140, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),   /* PCM_SYNC */
	GPIO_CFG(141, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),   /* PCM_CLK  */
};

static int __init aux_pcm_gpio_init(void)
{
	int pin, rc;

	pr_info("aux_pcm_gpio_init \n");
	for (pin = 0; pin < ARRAY_SIZE(aux_pcm_gpio_on); pin++) {
		rc = gpio_tlmm_config(aux_pcm_gpio_on[pin],
					GPIO_CFG_ENABLE);
		if (rc) {
			printk(KERN_ERR
				"%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, aux_pcm_gpio_on[pin], rc);
		}
	}
	return rc;
}

static struct msm_gpio mi2s_clk_gpios[] = {
	{ GPIO_CFG(145, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	    "MI2S_SCLK"},
	{ GPIO_CFG(144, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	    "MI2S_WS"},
	{ GPIO_CFG(120, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	    "MI2S_MCLK_A"},
};

static struct msm_gpio mi2s_rx_data_lines_gpios[] = {
	{ GPIO_CFG(121, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	    "MI2S_DATA_SD0_A"},
	{ GPIO_CFG(122, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	    "MI2S_DATA_SD1_A"},
	{ GPIO_CFG(146, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	    "MI2S_DATA_SD3"},
};

static struct msm_gpio mi2s_tx_data_lines_gpios[] = {
	{ GPIO_CFG(146, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	    "MI2S_DATA_SD3"},
};

int mi2s_config_clk_gpio(void)
{
	int rc = 0;

	rc = msm_gpios_request_enable(mi2s_clk_gpios,
			ARRAY_SIZE(mi2s_clk_gpios));
	if (rc) {
		pr_err("%s: enable mi2s clk gpios  failed\n",
					__func__);
		return rc;
	}
	return 0;
}

int  mi2s_unconfig_data_gpio(u32 direction, u8 sd_line_mask)
{
	int i, rc = 0;
	sd_line_mask &= MI2S_SD_LINE_MASK;

	switch (direction) {
	case DIR_TX:
		msm_gpios_disable_free(mi2s_tx_data_lines_gpios, 1);
		break;
	case DIR_RX:
		i = 0;
		while (sd_line_mask) {
			if (sd_line_mask & 0x1)
				msm_gpios_disable_free(
					mi2s_rx_data_lines_gpios + i , 1);
			sd_line_mask = sd_line_mask >> 1;
			i++;
		}
		break;
	default:
		pr_err("%s: Invaild direction  direction = %u\n",
						__func__, direction);
		rc = -EINVAL;
		break;
	}
	return rc;
}

int mi2s_config_data_gpio(u32 direction, u8 sd_line_mask)
{
	int i , rc = 0;
	u8 sd_config_done_mask = 0;

	sd_line_mask &= MI2S_SD_LINE_MASK;

	switch (direction) {
	case DIR_TX:
		if ((sd_line_mask & MI2S_SD_0) || (sd_line_mask & MI2S_SD_1) ||
		   (sd_line_mask & MI2S_SD_2) || !(sd_line_mask & MI2S_SD_3)) {
			pr_err("%s: can not use SD0 or SD1 or SD2 for TX"
				".only can use SD3. sd_line_mask = 0x%x\n",
				__func__ , sd_line_mask);
			rc = -EINVAL;
		} else {
			rc = msm_gpios_request_enable(mi2s_tx_data_lines_gpios,
							 1);
			if (rc)
				pr_err("%s: enable mi2s gpios for TX failed\n",
					   __func__);
		}
		break;
	case DIR_RX:
		i = 0;
		while (sd_line_mask && (rc == 0)) {
			if (sd_line_mask & 0x1) {
				rc = msm_gpios_request_enable(
					mi2s_rx_data_lines_gpios + i , 1);
				if (rc) {
					pr_err("%s: enable mi2s gpios for"
					 "RX failed.  SD line = %s\n",
					 __func__,
					 (mi2s_rx_data_lines_gpios + i)->label);
					mi2s_unconfig_data_gpio(DIR_RX,
						sd_config_done_mask);
				} else
					sd_config_done_mask |= (1 << i);
			}
			sd_line_mask = sd_line_mask >> 1;
			i++;
		}
		break;
	default:
		pr_err("%s: Invaild direction  direction = %u\n",
			__func__, direction);
		rc = -EINVAL;
		break;
	}
	return rc;
}

int mi2s_unconfig_clk_gpio(void)
{
	msm_gpios_disable_free(mi2s_clk_gpios, ARRAY_SIZE(mi2s_clk_gpios));
	return 0;
}

#endif /* CONFIG_MSM7KV2_AUDIO */

static int __init buses_init(void)
{
	if (gpio_tlmm_config(GPIO_CFG(PMIC_GPIO_INT, 1, GPIO_CFG_INPUT,
				  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE))
		pr_err("%s: gpio_tlmm_config (gpio=%d) failed\n",
		       __func__, PMIC_GPIO_INT);

	pm8058_7x30_data.sub_devices[PM8058_SUBDEV_KPD].platform_data
		= &surf_keypad_data;
	pm8058_7x30_data.sub_devices[PM8058_SUBDEV_KPD].data_size
		= sizeof(surf_keypad_data);

	i2c_register_board_info(6 /* I2C_SSBI ID */, pm8058_boardinfo,
				ARRAY_SIZE(pm8058_boardinfo));

	return 0;
}

#define TIMPANI_RESET_GPIO	1

struct bahama_config_register{
	u8 reg;
	u8 value;
	u8 mask;
};

enum version{
	VER_1_0,
	VER_2_0,
	VER_UNSUPPORTED = 0xFF
};


static struct vreg *vreg_marimba_1;
static struct vreg *vreg_marimba_2;
static struct vreg *vreg_marimba_3;

static struct msm_gpio timpani_reset_gpio_cfg[] = {
{ GPIO_CFG(TIMPANI_RESET_GPIO, 0, GPIO_CFG_OUTPUT,
	GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "timpani_reset"} };

static u8 read_bahama_ver(void)
{
	int rc;
	struct marimba config = { .mod_id = SLAVE_ID_BAHAMA };
	u8 bahama_version;

	rc = marimba_read_bit_mask(&config, 0x00,  &bahama_version, 1, 0x1F);
	if (rc < 0) {
		printk(KERN_ERR
			 "%s: version read failed: %d\n",
			__func__, rc);
			return rc;
	} else {
		printk(KERN_INFO
		"%s: version read got: 0x%x\n",
		__func__, bahama_version);
	}

	switch (bahama_version) {
	case 0x08: /* varient of bahama v1 */
	case 0x10:
	case 0x00:
		return VER_1_0;
	case 0x09: /* variant of bahama v2 */
		return VER_2_0;
	default:
		return VER_UNSUPPORTED;
	}
}

static int config_timpani_reset(void)
{
	int rc;

	rc = msm_gpios_request_enable(timpani_reset_gpio_cfg,
				ARRAY_SIZE(timpani_reset_gpio_cfg));
	if (rc < 0) {
		printk(KERN_ERR
			"%s: msm_gpios_request_enable failed (%d)\n",
				__func__, rc);
	}
	return rc;
}

static unsigned int msm_timpani_setup_power(void)
{
	int rc;

	rc = config_timpani_reset();
	if (rc < 0)
		goto out;

	rc = vreg_enable(vreg_marimba_1);
	if (rc) {
		printk(KERN_ERR "%s: vreg_enable() = %d\n",
					__func__, rc);
		goto out;
	}
	rc = vreg_enable(vreg_marimba_2);
	if (rc) {
		printk(KERN_ERR "%s: vreg_enable() = %d\n",
					__func__, rc);
		goto fail_disable_vreg_marimba_1;
	}

	rc = gpio_direction_output(TIMPANI_RESET_GPIO, 1);
	if (rc < 0) {
		printk(KERN_ERR
			"%s: gpio_direction_output failed (%d)\n",
				__func__, rc);
		msm_gpios_free(timpani_reset_gpio_cfg,
				ARRAY_SIZE(timpani_reset_gpio_cfg));
		vreg_disable(vreg_marimba_2);
	} else
		goto out;


fail_disable_vreg_marimba_1:
	vreg_disable(vreg_marimba_1);

out:
	return rc;
};

static void msm_timpani_shutdown_power(void)
{
	int rc;

	rc = vreg_disable(vreg_marimba_1);
	if (rc) {
		printk(KERN_ERR "%s: return val: %d\n",
					__func__, rc);
	}
	rc = vreg_disable(vreg_marimba_2);
	if (rc) {
		printk(KERN_ERR "%s: return val: %d\n",
					__func__, rc);
	}

	rc = gpio_direction_output(TIMPANI_RESET_GPIO, 0);
	if (rc < 0) {
		printk(KERN_ERR
			"%s: gpio_direction_output failed (%d)\n",
				__func__, rc);
	}

	msm_gpios_free(timpani_reset_gpio_cfg,
				   ARRAY_SIZE(timpani_reset_gpio_cfg));
};

static unsigned int msm_bahama_core_config(int type)
{
	int rc = 0;

	if (type == BAHAMA_ID) {

		int i;
		struct marimba config = { .mod_id = SLAVE_ID_BAHAMA };

		const struct bahama_config_register v20_init[] = {
			/* reg, value, mask */
			{ 0xF4, 0x84, 0xFF }, /* AREG */
			{ 0xF0, 0x04, 0xFF } /* DREG */
		};

		if (read_bahama_ver() == VER_2_0) {
			for (i = 0; i < ARRAY_SIZE(v20_init); i++) {
				u8 value = v20_init[i].value;
				rc = marimba_write_bit_mask(&config,
					v20_init[i].reg,
					&value,
					sizeof(v20_init[i].value),
					v20_init[i].mask);
				if (rc < 0) {
					printk(KERN_ERR
						"%s: reg %d write failed: %d\n",
						__func__, v20_init[i].reg, rc);
					return rc;
				}
				printk(KERN_INFO "%s: reg 0x%02x value 0x%02x"
					" mask 0x%02x\n",
					__func__, v20_init[i].reg,
					v20_init[i].value, v20_init[i].mask);
			}
		}
	}
	printk(KERN_INFO "core type: %d\n", type);

	return rc;
}

static unsigned int msm_bahama_setup_power(void)
{
	int rc;

	rc = vreg_enable(vreg_marimba_3);
	if (rc) {
		printk(KERN_ERR "%s: vreg_enable() = %d\n",
				__func__, rc);
	}

	return rc;
};

static unsigned int msm_bahama_shutdown_power(int value)
{
	int rc = 0;

	if (value != BAHAMA_ID) {
		rc = vreg_disable(vreg_marimba_3);
		if (rc) {
			printk(KERN_ERR "%s: return val: %d\n",
					__func__, rc);
		}
	}

	return rc;
};

static unsigned int msm_marimba_gpio_config_svlte(int gpio_cfg_marimba)
{
	return 0;
};

static unsigned int msm_marimba_setup_power(void)
{
	int rc;

	rc = vreg_enable(vreg_marimba_1);
	if (rc) {
		printk(KERN_ERR "%s: vreg_enable() = %d \n",
					__func__, rc);
		goto out;
	}
	rc = vreg_enable(vreg_marimba_2);
	if (rc) {
		printk(KERN_ERR "%s: vreg_enable() = %d \n",
					__func__, rc);
		goto out;
	}
out:
	return rc;
};

static void msm_marimba_shutdown_power(void)
{
	int rc;

	rc = vreg_disable(vreg_marimba_1);
	if (rc) {
		printk(KERN_ERR "%s: return val: %d\n",
					__func__, rc);
	}
	rc = vreg_disable(vreg_marimba_2);
	if (rc) {
		printk(KERN_ERR "%s: return val: %d \n",
					__func__, rc);
	}
};

static int bahama_present(void)
{
	int id;
	switch (id = adie_get_detected_connectivity_type()) {
	case BAHAMA_ID:
		return 1;

	case MARIMBA_ID:
		return 0;

	case TIMPANI_ID:
	default:
	printk(KERN_ERR "%s: unexpected adie connectivity type: %d\n",
			__func__, id);
	return -ENODEV;
	}
}

struct vreg *fm_regulator;
static int fm_radio_setup(struct marimba_fm_platform_data *pdata)
{
	int rc;
	uint32_t irqcfg;
	const char *id = "FMPW";

	int bahama_not_marimba = bahama_present();

	if (bahama_not_marimba == -1) {
		printk(KERN_WARNING "%s: bahama_present: %d\n",
				__func__, bahama_not_marimba);
		return -ENODEV;
	}
	if (bahama_not_marimba)
		fm_regulator = vreg_get(NULL, "s3");
	else
		fm_regulator = vreg_get(NULL, "s2");

	if (IS_ERR(fm_regulator)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
			__func__, PTR_ERR(fm_regulator));
		return -1;
	}
	if (!bahama_not_marimba) {

		rc = pmapp_vreg_level_vote(id, PMAPP_VREG_S2, 1300);

		if (rc < 0) {
			printk(KERN_ERR "%s: voltage level vote failed (%d)\n",
				__func__, rc);
			return rc;
		}
	}
	rc = vreg_enable(fm_regulator);
	if (rc) {
		printk(KERN_ERR "%s: vreg_enable() = %d\n",
					__func__, rc);
		return rc;
	}

	rc = pmapp_clock_vote(id, PMAPP_CLOCK_ID_DO,
					  PMAPP_CLOCK_VOTE_ON);
	if (rc < 0) {
		printk(KERN_ERR "%s: clock vote failed (%d)\n",
			__func__, rc);
		goto fm_clock_vote_fail;
	}

	irqcfg = GPIO_CFG(147, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,
					GPIO_CFG_2MA);
	rc = gpio_tlmm_config(irqcfg, GPIO_CFG_ENABLE);
	if (rc) {
		printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, irqcfg, rc);
		rc = -EIO;
		goto fm_gpio_config_fail;

	}
	return 0;
fm_gpio_config_fail:
	pmapp_clock_vote(id, PMAPP_CLOCK_ID_DO,
				  PMAPP_CLOCK_VOTE_OFF);
fm_clock_vote_fail:
	vreg_disable(fm_regulator);
	return rc;

};

static void fm_radio_shutdown(struct marimba_fm_platform_data *pdata)
{
	int rc;
	const char *id = "FMPW";
	uint32_t irqcfg = GPIO_CFG(147, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
					GPIO_CFG_2MA);

	int bahama_not_marimba = bahama_present();
	if (bahama_not_marimba == -1) {
		printk(KERN_WARNING "%s: bahama_present: %d\n",
			__func__, bahama_not_marimba);
		return;
	}

	rc = gpio_tlmm_config(irqcfg, GPIO_CFG_ENABLE);
	if (rc) {
		printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, irqcfg, rc);
	}
	if (fm_regulator != NULL) {
		rc = vreg_disable(fm_regulator);

		if (rc) {
			printk(KERN_ERR "%s: return val: %d\n",
					__func__, rc);
		}
		fm_regulator = NULL;
	}
	rc = pmapp_clock_vote(id, PMAPP_CLOCK_ID_DO,
					  PMAPP_CLOCK_VOTE_OFF);
	if (rc < 0)
		printk(KERN_ERR "%s: clock_vote return val: %d\n",
						__func__, rc);

	if (!bahama_not_marimba)	{
		rc = pmapp_vreg_level_vote(id, PMAPP_VREG_S2, 0);

		if (rc < 0)
			printk(KERN_ERR "%s: vreg level vote return val: %d\n",
						__func__, rc);
	}
}

static struct marimba_fm_platform_data marimba_fm_pdata = {
	.fm_setup =  fm_radio_setup,
	.fm_shutdown = fm_radio_shutdown,
	.irq = MSM_GPIO_TO_INT(147),
	.vreg_s2 = NULL,
	.vreg_xo_out = NULL,
	.is_fm_soc_i2s_master = false,
};


/* Slave id address for FM/CDC/QMEMBIST
 * Values can be programmed using Marimba slave id 0
 * should there be a conflict with other I2C devices
 * */
#define MARIMBA_SLAVE_ID_FM_ADDR	0x2A
#define MARIMBA_SLAVE_ID_CDC_ADDR	0x77
#define MARIMBA_SLAVE_ID_QMEMBIST_ADDR	0X66

#define BAHAMA_SLAVE_ID_FM_ADDR         0x2A
#define BAHAMA_SLAVE_ID_QMEMBIST_ADDR   0x7B

static const char *tsadc_id = "MADC";
static const char *vregs_tsadc_name[] = {
	"s2",
       "gp12"
};
static struct vreg *vregs_tsadc[ARRAY_SIZE(vregs_tsadc_name)];

static const char *vregs_timpani_tsadc_name[] = {
	"s3",
       "gp12",
	"gp16"
};
static struct vreg *vregs_timpani_tsadc[ARRAY_SIZE(vregs_timpani_tsadc_name)];

static int marimba_tsadc_power(int vreg_on)
{
	int i, rc = 0;
	int tsadc_adie_type = adie_get_detected_codec_type();

	if (tsadc_adie_type == TIMPANI_ID) {
		for (i = 0; i < ARRAY_SIZE(vregs_timpani_tsadc_name); i++) {
			if (!vregs_timpani_tsadc[i]) {
				pr_err("%s: vreg_get %s failed(%d)\n",
				__func__, vregs_timpani_tsadc_name[i], rc);
				goto vreg_fail;
			}

			rc = vreg_on ? vreg_enable(vregs_timpani_tsadc[i]) :
				  vreg_disable(vregs_timpani_tsadc[i]);
			if (rc < 0) {
				pr_err("%s: vreg %s %s failed(%d)\n",
					__func__, vregs_timpani_tsadc_name[i],
				       vreg_on ? "enable" : "disable", rc);
				goto vreg_fail;
			}
		}
		/* Vote for D0 and D1 buffer */
		rc = pmapp_clock_vote(tsadc_id, PMAPP_CLOCK_ID_D1,
			vreg_on ? PMAPP_CLOCK_VOTE_ON : PMAPP_CLOCK_VOTE_OFF);
		if (rc)	{
			pr_err("%s: unable to %svote for d1 clk\n",
				__func__, vreg_on ? "" : "de-");
			goto do_vote_fail;
		}
		rc = pmapp_clock_vote(tsadc_id, PMAPP_CLOCK_ID_DO,
			vreg_on ? PMAPP_CLOCK_VOTE_ON : PMAPP_CLOCK_VOTE_OFF);
		if (rc)	{
			pr_err("%s: unable to %svote for d1 clk\n",
				__func__, vreg_on ? "" : "de-");
			goto do_vote_fail;
		}
	} else if (tsadc_adie_type == MARIMBA_ID) {
		for (i = 0; i < ARRAY_SIZE(vregs_tsadc_name); i++) {
			if (!vregs_tsadc[i]) {
				pr_err("%s: vreg_get %s failed (%d)\n",
					__func__, vregs_tsadc_name[i], rc);
				goto vreg_fail;
			}

			rc = vreg_on ? vreg_enable(vregs_tsadc[i]) :
				  vreg_disable(vregs_tsadc[i]);
			if (rc < 0) {
				pr_err("%s: vreg %s %s failed (%d)\n",
					__func__, vregs_tsadc_name[i],
				       vreg_on ? "enable" : "disable", rc);
				goto vreg_fail;
			}
		}
		/* If marimba vote for DO buffer */
		rc = pmapp_clock_vote(tsadc_id, PMAPP_CLOCK_ID_DO,
			vreg_on ? PMAPP_CLOCK_VOTE_ON : PMAPP_CLOCK_VOTE_OFF);
		if (rc)	{
			pr_err("%s: unable to %svote for d0 clk\n",
				__func__, vreg_on ? "" : "de-");
			goto do_vote_fail;
		}
	} else {
		pr_err("%s:Adie %d not supported\n",
				__func__, tsadc_adie_type);
		return -ENODEV;
	}

	msleep(5); /* ensure power is stable */

	return 0;

do_vote_fail:
vreg_fail:
	while (i) {
		if (vreg_on) {
			if (tsadc_adie_type == TIMPANI_ID)
				vreg_disable(vregs_timpani_tsadc[--i]);
			else if (tsadc_adie_type == MARIMBA_ID)
				vreg_disable(vregs_tsadc[--i]);
		} else {
			if (tsadc_adie_type == TIMPANI_ID)
				vreg_enable(vregs_timpani_tsadc[--i]);
			else if (tsadc_adie_type == MARIMBA_ID)
				vreg_enable(vregs_tsadc[--i]);
		}
	}

	return rc;
}

static int marimba_tsadc_vote(int vote_on)
{
	int rc = 0;

	if (adie_get_detected_codec_type() == MARIMBA_ID) {
		int level = vote_on ? 1300 : 0;
		rc = pmapp_vreg_level_vote(tsadc_id, PMAPP_VREG_S2, level);
		if (rc < 0)
			pr_err("%s: vreg level %s failed (%d)\n",
			__func__, vote_on ? "on" : "off", rc);
	}

	return rc;
}

static int marimba_tsadc_init(void)
{
	int i, rc = 0;
	int tsadc_adie_type = adie_get_detected_codec_type();

	if (tsadc_adie_type == TIMPANI_ID) {
		for (i = 0; i < ARRAY_SIZE(vregs_timpani_tsadc_name); i++) {
			vregs_timpani_tsadc[i] = vreg_get(NULL,
						vregs_timpani_tsadc_name[i]);
			if (IS_ERR(vregs_timpani_tsadc[i])) {
				pr_err("%s: vreg get %s failed (%ld)\n",
				       __func__, vregs_timpani_tsadc_name[i],
				       PTR_ERR(vregs_timpani_tsadc[i]));
				rc = PTR_ERR(vregs_timpani_tsadc[i]);
				goto vreg_get_fail;
			}
		}
	} else if (tsadc_adie_type == MARIMBA_ID) {
		for (i = 0; i < ARRAY_SIZE(vregs_tsadc_name); i++) {
			vregs_tsadc[i] = vreg_get(NULL, vregs_tsadc_name[i]);
			if (IS_ERR(vregs_tsadc[i])) {
				pr_err("%s: vreg get %s failed (%ld)\n",
				       __func__, vregs_tsadc_name[i],
				       PTR_ERR(vregs_tsadc[i]));
				rc = PTR_ERR(vregs_tsadc[i]);
				goto vreg_get_fail;
			}
		}
	} else {
		pr_err("%s:Adie %d not supported\n",
				__func__, tsadc_adie_type);
		return -ENODEV;
	}

	return 0;

vreg_get_fail:
	while (i) {
		if (tsadc_adie_type == TIMPANI_ID)
			vreg_put(vregs_timpani_tsadc[--i]);
		else if (tsadc_adie_type == MARIMBA_ID)
			vreg_put(vregs_tsadc[--i]);
	}
	return rc;
}

static int marimba_tsadc_exit(void)
{
	int i, rc = 0;
	int tsadc_adie_type = adie_get_detected_codec_type();

	if (tsadc_adie_type == TIMPANI_ID) {
		for (i = 0; i < ARRAY_SIZE(vregs_timpani_tsadc_name); i++) {
			if (vregs_tsadc[i])
				vreg_put(vregs_timpani_tsadc[i]);
		}
	} else if (tsadc_adie_type == MARIMBA_ID) {
		for (i = 0; i < ARRAY_SIZE(vregs_tsadc_name); i++) {
			if (vregs_tsadc[i])
				vreg_put(vregs_tsadc[i]);
		}
		rc = pmapp_vreg_level_vote(tsadc_id, PMAPP_VREG_S2, 0);
		if (rc < 0)
			pr_err("%s: vreg level off failed (%d)\n",
						__func__, rc);
	} else {
		pr_err("%s:Adie %d not supported\n",
				__func__, tsadc_adie_type);
		rc = -ENODEV;
	}

	return rc;
}


static struct msm_ts_platform_data msm_ts_data = {
	.min_x          = 0,
	.max_x          = 4096,
	.min_y          = 0,
	.max_y          = 4096,
	.min_press      = 0,
	.max_press      = 255,
	.inv_x          = 4096,
	.inv_y          = 4096,
	.can_wakeup	= false,
};

static struct marimba_tsadc_platform_data marimba_tsadc_pdata = {
	.marimba_tsadc_power =  marimba_tsadc_power,
	.init		     =  marimba_tsadc_init,
	.exit		     =  marimba_tsadc_exit,
	.level_vote	     =  marimba_tsadc_vote,
	.tsadc_prechg_en = true,
	.can_wakeup	= false,
	.setup = {
		.pen_irq_en	=	true,
		.tsadc_en	=	true,
	},
	.params2 = {
		.input_clk_khz		=	2400,
		.sample_prd		=	TSADC_CLK_3,
	},
	.params3 = {
		.prechg_time_nsecs	=	6400,
		.stable_time_nsecs	=	6400,
		.tsadc_test_mode	=	0,
	},
	.tssc_data = &msm_ts_data,
};

static struct vreg *vreg_codec_s4;
static int msm_marimba_codec_power(int vreg_on)
{
	int rc = 0;

	if (!vreg_codec_s4) {

		vreg_codec_s4 = vreg_get(NULL, "s4");

		if (IS_ERR(vreg_codec_s4)) {
			printk(KERN_ERR "%s: vreg_get() failed (%ld)\n",
				__func__, PTR_ERR(vreg_codec_s4));
			rc = PTR_ERR(vreg_codec_s4);
			goto  vreg_codec_s4_fail;
		}
	}

	if (vreg_on) {
		rc = vreg_enable(vreg_codec_s4);
		if (rc)
			printk(KERN_ERR "%s: vreg_enable() = %d \n",
					__func__, rc);
		goto vreg_codec_s4_fail;
	} else {
		rc = vreg_disable(vreg_codec_s4);
		if (rc)
			printk(KERN_ERR "%s: vreg_disable() = %d \n",
					__func__, rc);
		goto vreg_codec_s4_fail;
	}

vreg_codec_s4_fail:
	return rc;
}

static struct marimba_codec_platform_data mariba_codec_pdata = {
	.marimba_codec_power =  msm_marimba_codec_power,
#ifdef CONFIG_MARIMBA_CODEC
	.snddev_profile_init = msm_snddev_init,
#endif
};

static struct marimba_platform_data marimba_pdata = {
	.slave_id[MARIMBA_SLAVE_ID_FM]       = MARIMBA_SLAVE_ID_FM_ADDR,
	.slave_id[MARIMBA_SLAVE_ID_CDC]	     = MARIMBA_SLAVE_ID_CDC_ADDR,
	.slave_id[MARIMBA_SLAVE_ID_QMEMBIST] = MARIMBA_SLAVE_ID_QMEMBIST_ADDR,
	.slave_id[SLAVE_ID_BAHAMA_FM]        = BAHAMA_SLAVE_ID_FM_ADDR,
	.slave_id[SLAVE_ID_BAHAMA_QMEMBIST]  = BAHAMA_SLAVE_ID_QMEMBIST_ADDR,
	.marimba_setup = msm_marimba_setup_power,
	.marimba_shutdown = msm_marimba_shutdown_power,
	.bahama_setup = msm_bahama_setup_power,
	.bahama_shutdown = msm_bahama_shutdown_power,
	.marimba_gpio_config = msm_marimba_gpio_config_svlte,
	.bahama_core_config = msm_bahama_core_config,
	.fm = &marimba_fm_pdata,
	.codec = &mariba_codec_pdata,
};

static void __init msm7x30_init_marimba(void)
{
	vreg_marimba_1 = vreg_get(NULL, "s3");
	if (IS_ERR(vreg_marimba_1)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
			__func__, PTR_ERR(vreg_marimba_1));
		return;
	}
	vreg_set_level(vreg_marimba_1, 1800);

	vreg_marimba_2 = vreg_get(NULL, "gp16");
	if (IS_ERR(vreg_marimba_1)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
			__func__, PTR_ERR(vreg_marimba_1));
		return;
	}
	vreg_set_level(vreg_marimba_2, 1200);

	vreg_marimba_3 = vreg_get(NULL, "usb2");
	if (IS_ERR(vreg_marimba_3)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
			__func__, PTR_ERR(vreg_marimba_3));
		return;
	}
	vreg_set_level(vreg_marimba_3, 1800);
}

static struct marimba_codec_platform_data timpani_codec_pdata = {
	.marimba_codec_power =  msm_marimba_codec_power,
#ifdef CONFIG_TIMPANI_CODEC
	.snddev_profile_init = msm_snddev_init_timpani,
#endif
};

static struct marimba_platform_data timpani_pdata = {
	.slave_id[MARIMBA_SLAVE_ID_CDC]	= MARIMBA_SLAVE_ID_CDC_ADDR,
	.slave_id[MARIMBA_SLAVE_ID_QMEMBIST] = MARIMBA_SLAVE_ID_QMEMBIST_ADDR,
	.marimba_setup = msm_timpani_setup_power,
	.marimba_shutdown = msm_timpani_shutdown_power,
	.codec = &timpani_codec_pdata,
	.tsadc = &marimba_tsadc_pdata,
};

#define TIMPANI_I2C_SLAVE_ADDR	0xD

static struct i2c_board_info msm_i2c_gsbi7_timpani_info[] = {
	{
		I2C_BOARD_INFO("timpani", TIMPANI_I2C_SLAVE_ADDR),
		.platform_data = &timpani_pdata,
	},
};

#ifdef CONFIG_MSM7KV2_AUDIO
static struct resource msm_aictl_resources[] = {
	{
		.name = "aictl",
		.start = 0xa5000100,
		.end = 0xa5000100,
		.flags = IORESOURCE_MEM,
	}
};

static struct resource msm_mi2s_resources[] = {
	{
		.name = "hdmi",
		.start = 0xac900000,
		.end = 0xac900038,
		.flags = IORESOURCE_MEM,
	},
	{
		.name = "codec_rx",
		.start = 0xac940040,
		.end = 0xac940078,
		.flags = IORESOURCE_MEM,
	},
	{
		.name = "codec_tx",
		.start = 0xac980080,
		.end = 0xac9800B8,
		.flags = IORESOURCE_MEM,
	}

};

static struct msm_lpa_platform_data lpa_pdata = {
	.obuf_hlb_size = 0x2BFF8,
	.dsp_proc_id = 0,
	.app_proc_id = 2,
	.nosb_config = {
		.llb_min_addr = 0,
		.llb_max_addr = 0x3ff8,
		.sb_min_addr = 0,
		.sb_max_addr = 0,
	},
	.sb_config = {
		.llb_min_addr = 0,
		.llb_max_addr = 0x37f8,
		.sb_min_addr = 0x3800,
		.sb_max_addr = 0x3ff8,
	}
};

static struct resource msm_lpa_resources[] = {
	{
		.name = "lpa",
		.start = 0xa5000000,
		.end = 0xa50000a0,
		.flags = IORESOURCE_MEM,
	}
};

static struct resource msm_aux_pcm_resources[] = {

	{
		.name = "aux_codec_reg_addr",
		.start = 0xac9c00c0,
		.end = 0xac9c00c8,
		.flags = IORESOURCE_MEM,
	},
	{
		.name   = "aux_pcm_dout",
		.start  = 138,
		.end    = 138,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "aux_pcm_din",
		.start  = 139,
		.end    = 139,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "aux_pcm_syncout",
		.start  = 140,
		.end    = 140,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "aux_pcm_clkin_a",
		.start  = 141,
		.end    = 141,
		.flags  = IORESOURCE_IO,
	},
};

static struct platform_device msm_aux_pcm_device = {
	.name   = "msm_aux_pcm",
	.id     = 0,
	.num_resources  = ARRAY_SIZE(msm_aux_pcm_resources),
	.resource       = msm_aux_pcm_resources,
};

struct platform_device msm_aictl_device = {
	.name = "audio_interct",
	.id   = 0,
	.num_resources = ARRAY_SIZE(msm_aictl_resources),
	.resource = msm_aictl_resources,
};

struct platform_device msm_mi2s_device = {
	.name = "mi2s",
	.id   = 0,
	.num_resources = ARRAY_SIZE(msm_mi2s_resources),
	.resource = msm_mi2s_resources,
};

struct platform_device msm_lpa_device = {
	.name = "lpa",
	.id   = 0,
	.num_resources = ARRAY_SIZE(msm_lpa_resources),
	.resource = msm_lpa_resources,
	.dev		= {
		.platform_data = &lpa_pdata,
	},
};
#endif /* CONFIG_MSM7KV2_AUDIO */

#define DEC0_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
#define DEC1_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
 #define DEC2_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
 #define DEC3_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
#define DEC4_FORMAT (1<<MSM_ADSP_CODEC_MIDI)

static unsigned int dec_concurrency_table[] = {
	/* Audio LP */
	0,
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_MODE_LP)|
	(1<<MSM_ADSP_OP_DM)),

	/* Concurrency 1 */
	(DEC4_FORMAT),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),

	 /* Concurrency 2 */
	(DEC4_FORMAT),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),

	/* Concurrency 3 */
	(DEC4_FORMAT),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),

	/* Concurrency 4 */
	(DEC4_FORMAT),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),

	/* Concurrency 5 */
	(DEC4_FORMAT),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),

	/* Concurrency 6 */
	(DEC4_FORMAT),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
};

#define DEC_INFO(name, queueid, decid, nr_codec) { .module_name = name, \
	.module_queueid = queueid, .module_decid = decid, \
	.nr_codec_support = nr_codec}

#define DEC_INSTANCE(max_instance_same, max_instance_diff) { \
	.max_instances_same_dec = max_instance_same, \
	.max_instances_diff_dec = max_instance_diff}

static struct msm_adspdec_info dec_info_list[] = {
	DEC_INFO("AUDPLAY4TASK", 17, 4, 1),  /* AudPlay4BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY3TASK", 16, 3, 11),  /* AudPlay3BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY2TASK", 15, 2, 11),  /* AudPlay2BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY1TASK", 14, 1, 11),  /* AudPlay1BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY0TASK", 13, 0, 11), /* AudPlay0BitStreamCtrlQueue */
};

static struct dec_instance_table dec_instance_list[][MSM_MAX_DEC_CNT] = {
	/* Non Turbo Mode */
	{
		DEC_INSTANCE(4, 3), /* WAV */
		DEC_INSTANCE(4, 3), /* ADPCM */
		DEC_INSTANCE(4, 2), /* MP3 */
		DEC_INSTANCE(0, 0), /* Real Audio */
		DEC_INSTANCE(4, 2), /* WMA */
		DEC_INSTANCE(3, 2), /* AAC */
		DEC_INSTANCE(0, 0), /* Reserved */
		DEC_INSTANCE(0, 0), /* MIDI */
		DEC_INSTANCE(4, 3), /* YADPCM */
		DEC_INSTANCE(4, 3), /* QCELP */
		DEC_INSTANCE(4, 3), /* AMRNB */
		DEC_INSTANCE(1, 1), /* AMRWB/WB+ */
		DEC_INSTANCE(4, 3), /* EVRC */
		DEC_INSTANCE(1, 1), /* WMAPRO */
	},
	/* Turbo Mode */
	{
		DEC_INSTANCE(4, 3), /* WAV */
		DEC_INSTANCE(4, 3), /* ADPCM */
		DEC_INSTANCE(4, 3), /* MP3 */
		DEC_INSTANCE(0, 0), /* Real Audio */
		DEC_INSTANCE(4, 3), /* WMA */
		DEC_INSTANCE(4, 3), /* AAC */
		DEC_INSTANCE(0, 0), /* Reserved */
		DEC_INSTANCE(0, 0), /* MIDI */
		DEC_INSTANCE(4, 3), /* YADPCM */
		DEC_INSTANCE(4, 3), /* QCELP */
		DEC_INSTANCE(4, 3), /* AMRNB */
		DEC_INSTANCE(2, 3), /* AMRWB/WB+ */
		DEC_INSTANCE(4, 3), /* EVRC */
		DEC_INSTANCE(1, 2), /* WMAPRO */
	},
};

static struct msm_adspdec_database msm_device_adspdec_database = {
	.num_dec = ARRAY_SIZE(dec_info_list),
	.num_concurrency_support = (ARRAY_SIZE(dec_concurrency_table) / \
					ARRAY_SIZE(dec_info_list)),
	.dec_concurrency_table = dec_concurrency_table,
	.dec_info_list = dec_info_list,
	.dec_instance_list = &dec_instance_list[0][0],
};

static struct platform_device msm_device_adspdec = {
	.name = "msm_adspdec",
	.id = -1,
	.dev    = {
		.platform_data = &msm_device_adspdec_database
	},
};

#ifdef CONFIG_USB_FUNCTION
static struct usb_mass_storage_platform_data usb_mass_storage_pdata = {
	.nluns          = 0x02,
	.buf_size       = 16384,
	.vendor         = "GOOGLE",
	.product        = "Mass storage",
	.release        = 0xffff,
};

static struct platform_device mass_storage_device = {
	.name           = "usb_mass_storage",
	.id             = -1,
	.dev            = {
		.platform_data          = &usb_mass_storage_pdata,
	},
};
#endif
#ifdef CONFIG_USB_ANDROID
//chenping add for ponyo pidvid 20110721
#ifdef CONFIG_PONYO_DOCOMO_USB_PID_VID
static char *usb_functions_mass_storage_adb[] = {
	"adb",
        "usb_mass_storage",
};

static char *usb_functions_only_mass_storage[] = {
	"usb_mass_storage",
};
#endif
static char *usb_functions_default[] = {
	"diag",
	"modem",
	"nmea",
	"rmnet",
	"usb_mass_storage",
};

static char *usb_functions_default_adb[] = {
	"diag",
	"adb",
	"modem",
	"nmea",
	"rmnet",
	"usb_mass_storage",
};

static char *usb_functions_rndis[] = {
	"rndis",
};

static char *usb_functions_rndis_adb[] = {
	"rndis",
	"adb",
};

static char *usb_functions_rndis_diag[] = {
	"rndis",
	"diag",
};

static char *usb_functions_rndis_adb_diag[] = {
	"rndis",
	"diag",
	"adb",
};

static char *usb_functions_all[] = {
#ifdef CONFIG_USB_ANDROID_RNDIS
	"rndis",
#endif
#ifdef CONFIG_USB_ANDROID_DIAG
	"diag",
#endif
	"adb",
#ifdef CONFIG_USB_F_SERIAL
	"modem",
	"nmea",
#endif
#ifdef CONFIG_USB_ANDROID_RMNET
	"rmnet",
#endif
	"usb_mass_storage",
#ifdef CONFIG_USB_ANDROID_ACM
	"acm",
#endif
};

static struct android_usb_product usb_products[] = {
//chenping add for ponyo pidvid 20110721
#ifdef CONFIG_PONYO_DOCOMO_USB_PID_VID
	{
		.product_id     = 0x21F8,
		.num_functions	= ARRAY_SIZE(usb_functions_only_mass_storage),
		.functions      = usb_functions_only_mass_storage,
	},
	{
		.product_id     = 0x21F6,
		.num_functions	= ARRAY_SIZE(usb_functions_mass_storage_adb),
		.functions      = usb_functions_mass_storage_adb,
	},
	{
		.product_id     = 0x21F7,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis),
		.functions      = usb_functions_rndis,
	},
	{
		.product_id     = 0x21F7,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis_adb),
		.functions      = usb_functions_rndis_adb,
	},
#endif
	{
		.product_id     = 0x9026,
		.num_functions	= ARRAY_SIZE(usb_functions_default),
		.functions      = usb_functions_default,
	},
	{
		.product_id	= 0x9025,
		.num_functions	= ARRAY_SIZE(usb_functions_default_adb),
		.functions	= usb_functions_default_adb,
	},
	{
		/* RNDIS + DIAG */
		.product_id	= 0x902C,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis_diag),
		.functions	= usb_functions_rndis_diag,
	},
	{
		/* RNDIS + ADB + DIAG */
		.product_id	= 0x902D,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis_adb_diag),
		.functions	= usb_functions_rndis_adb_diag,
	},
};

static struct usb_mass_storage_platform_data mass_storage_pdata = {
	.nluns		= 1,
#ifdef CONFIG_PONYO_DOCOMO_USB_PID_VID
	.vendor		= "DOCOMO",
	.product        = "FOMA P01D",
#else
	.vendor		= "Qualcomm Incorporated",
	.product        = "Mass storage",
#endif
	.release	= 0x0100,
	.can_stall	= 1,

};

static struct platform_device usb_mass_storage_device = {
	.name	= "usb_mass_storage",
	.id	= -1,
	.dev	= {
		.platform_data = &mass_storage_pdata,
	},
};

static struct usb_ether_platform_data rndis_pdata = {
	/* ethaddr is filled by board_serialno_setup */
	.vendorID	= 0x05C6,
	.vendorDescr	= "Qualcomm Incorporated",
};

static struct platform_device rndis_device = {
	.name	= "rndis",
	.id	= -1,
	.dev	= {
		.platform_data = &rndis_pdata,
	},
};

static struct android_usb_platform_data android_usb_pdata = {
//chenping add for ponyo pidvid 20110721
#ifdef CONFIG_PONYO_DOCOMO_USB_PID_VID
	.vendor_id	= 0x04DA,
	.product_id      = 0x21F8,
#else
	.vendor_id	= 0x05C6,
	.product_id	= 0x9026,
#endif
	.version	= 0x0100,
#ifdef CONFIG_PONYO_DOCOMO_USB_PID_VID
	.product_name		= "FOMA P01D",
	.manufacturer_name	= "Panasonic Mobile Communications Co Ltd",
#else
	.product_name		= "Qualcomm HSUSB Device",
	.manufacturer_name	= "Qualcomm Incorporated",
#endif
	.num_products = ARRAY_SIZE(usb_products),
	.products = usb_products,
	.num_functions = ARRAY_SIZE(usb_functions_all),
	.functions = usb_functions_all,
	.serial_number = "1234567890ABCDEF",
};
static struct platform_device android_usb_device = {
	.name	= "android_usb",
	.id		= -1,
	.dev		= {
		.platform_data = &android_usb_pdata,
	},
};

static int __init board_serialno_setup(char *serialno)
{
	int i;
	char *src = serialno;

	/* create a fake MAC address from our serial number.
	 * first byte is 0x02 to signify locally administered.
	 */
	rndis_pdata.ethaddr[0] = 0x02;
	for (i = 0; *src; i++) {
		/* XOR the USB serial across the remaining bytes */
		rndis_pdata.ethaddr[i % (ETH_ALEN - 1) + 1] ^= *src++;
	}

	android_usb_pdata.serial_number = serialno;
	return 1;
}
__setup("androidboot.serialno=", board_serialno_setup);

#endif

static struct msm_gpio optnav_config_data[] = {
	{ GPIO_CFG(OPTNAV_CHIP_SELECT, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
	"optnav_chip_select" },
};

static void __iomem *virtual_optnav;

static int optnav_gpio_setup(void)
{
	int rc = -ENODEV;
	rc = msm_gpios_request_enable(optnav_config_data,
			ARRAY_SIZE(optnav_config_data));

	/* Configure the FPGA for GPIOs */
	virtual_optnav = ioremap(FPGA_OPTNAV_GPIO_ADDR, 0x4);
	if (!virtual_optnav) {
		pr_err("%s:Could not ioremap region\n", __func__);
		return -ENOMEM;
	}
	/*
	 * Configure the FPGA to set GPIO 19 as
	 * normal, active(enabled), output(MSM to SURF)
	 */
	writew(0x311E, virtual_optnav);
	return rc;
}

static void optnav_gpio_release(void)
{
	msm_gpios_disable_free(optnav_config_data,
		ARRAY_SIZE(optnav_config_data));
	iounmap(virtual_optnav);
}

static struct vreg *vreg_gp7;
static struct vreg *vreg_gp4;
static struct vreg *vreg_gp9;
static struct vreg *vreg_usb3_3;

static int optnav_enable(void)
{
	int rc;
	/*
	 * Enable the VREGs L8(gp7), L10(gp4), L12(gp9), L6(usb)
	 * for I2C communication with keyboard.
	 */
	vreg_gp7 = vreg_get(NULL, "gp7");
	rc = vreg_set_level(vreg_gp7, 1800);
	if (rc) {
		pr_err("%s: vreg_set_level failed \n", __func__);
		goto fail_vreg_gp7;
	}

	rc = vreg_enable(vreg_gp7);
	if (rc) {
		pr_err("%s: vreg_enable failed \n", __func__);
		goto fail_vreg_gp7;
	}

	vreg_gp4 = vreg_get(NULL, "gp4");
	rc = vreg_set_level(vreg_gp4, 2600);
	if (rc) {
		pr_err("%s: vreg_set_level failed \n", __func__);
		goto fail_vreg_gp4;
	}

	rc = vreg_enable(vreg_gp4);
	if (rc) {
		pr_err("%s: vreg_enable failed \n", __func__);
		goto fail_vreg_gp4;
	}

	vreg_gp9 = vreg_get(NULL, "gp9");
	rc = vreg_set_level(vreg_gp9, 1800);
	if (rc) {
		pr_err("%s: vreg_set_level failed \n", __func__);
		goto fail_vreg_gp9;
	}

	rc = vreg_enable(vreg_gp9);
	if (rc) {
		pr_err("%s: vreg_enable failed \n", __func__);
		goto fail_vreg_gp9;
	}

	vreg_usb3_3 = vreg_get(NULL, "usb");
	rc = vreg_set_level(vreg_usb3_3, 3300);
	if (rc) {
		pr_err("%s: vreg_set_level failed \n", __func__);
		goto fail_vreg_3_3;
	}

	rc = vreg_enable(vreg_usb3_3);
	if (rc) {
		pr_err("%s: vreg_enable failed \n", __func__);
		goto fail_vreg_3_3;
	}

	/* Enable the chip select GPIO */
	gpio_set_value(OPTNAV_CHIP_SELECT, 1);
	gpio_set_value(OPTNAV_CHIP_SELECT, 0);

	return 0;

fail_vreg_3_3:
	;//vreg_disable(vreg_gp9); //del [simt-zhanghui-110811]
fail_vreg_gp9:
	vreg_disable(vreg_gp4);
fail_vreg_gp4:
	vreg_disable(vreg_gp7);
fail_vreg_gp7:
	return rc;
}

static void optnav_disable(void)
{
	vreg_disable(vreg_usb3_3);
	//vreg_disable(vreg_gp9); //del [simt-zhanghui-110811]
	vreg_disable(vreg_gp4);
	vreg_disable(vreg_gp7);

	gpio_set_value(OPTNAV_CHIP_SELECT, 1);
}

static struct ofn_atlab_platform_data optnav_data = {
	.gpio_setup    = optnav_gpio_setup,
	.gpio_release  = optnav_gpio_release,
	.optnav_on     = optnav_enable,
	.optnav_off    = optnav_disable,
	.rotate_xy     = 0,
	.function1 = {
		.no_motion1_en		= true,
		.touch_sensor_en	= true,
		.ofn_en			= true,
		.clock_select_khz	= 1500,
		.cpi_selection		= 1200,
	},
	.function2 =  {
		.invert_y		= false,
		.invert_x		= true,
		.swap_x_y		= false,
		.hold_a_b_en		= true,
		.motion_filter_en       = true,
	},
};

static int hdmi_comm_power(int on, int show);
static int hdmi_init_irq(void);
static int hdmi_enable_5v(int on);
static int hdmi_core_power(int on, int show);
static int hdmi_cec_power(int on);
static bool hdmi_check_hdcp_hw_support(void);

static struct msm_hdmi_platform_data adv7520_hdmi_data = {
	.irq = MSM_GPIO_TO_INT(18),
	.comm_power = hdmi_comm_power,
	.init_irq = hdmi_init_irq,
	.enable_5v = hdmi_enable_5v,
	.core_power = hdmi_core_power,
	.cec_power = hdmi_cec_power,
	.check_hdcp_hw_support = hdmi_check_hdcp_hw_support,
};

#if defined(CONFIG_SENSORS_MMC31XX) || defined(CONFIG_SENSORS_MMC328X)
/* + qiukj add for ponyo project*/
static int magsensor_powerctrl(int on_off)
{
        static struct vreg *vreg = NULL;
        int                             retval = -EINVAL;

        pr_info("%s(): on_off = %d\n",__func__,on_off);

        vreg = vreg_get(NULL,"gp4"); // gp4 corresponds to VREG L10 of 8058
        if (!vreg)
        {
                pr_err("%s: VREG L10 get failed\n", __func__);
                return retval;
        }

        if(on_off)
        {
                retval = vreg_set_level(vreg,3000); // set the voltage to 3 v
                if(retval)
                {
                        pr_err("%s: VREG L10 set failed\n", __func__);
                        goto L10_put;
                }

                retval = vreg_enable(vreg);
                if(retval)
                {
                        pr_err("%s: VREG L10 enable failed\n", __func__);
                        goto L10_disable;
                } 
        }  else
        	{
               	 	retval = vreg_set_level(vreg,0); // set the voltage to 0 v
                	if(retval)
               		 {
                        	pr_err("%s: VREG L10 set failed\n", __func__);
                       		 goto L10_put;
                	}

                retval = vreg_disable(vreg);
                if(retval)
                {
                        pr_err("%s: VREG L10 enable failed\n", __func__);
                        goto L10_disable;
                }
                }
        return 0;
	
L10_disable:
        vreg_disable(vreg);
L10_put:
        vreg_put(vreg);

        return  retval;
}
#endif

#ifdef CONFIG_SENSORS_MMC31XX
#include <linux/mmc31xx.h>
static struct mag_platform_data mmc31xx_platform_data = {
        .power = magsensor_powerctrl,
};
#endif

#ifdef CONFIG_SENSORS_MMC328X
#include <linux/mmc328x.h>
struct mag_platform_data mmc328x_platform_data = {
      .power = magsensor_powerctrl,
};
#endif

/* - qiukj add for ponyo project*/


//[SIMT-caoxiangteng-110715]{
/*
 * add platform data for cm3623
 */
#ifdef CONFIG_CM3623
static int cm3623_power(int on) {
	static struct vreg *vreg_gp4 = NULL, *vreg_wlan = NULL;
	int 				retval = -EINVAL;

	printk("cm3623 power open\n");
	vreg_gp4 = vreg_get(NULL,"gp4"); //gp4 corresponds to VREG L10 of 8058
	if (!vreg_gp4) {
		pr_err("%s: VREG L10 get failed\n", __func__);
		return retval;
	}
	vreg_wlan= vreg_get(NULL,"wlan"); //wlan corresponds to VREG L13 of 8058
	if (!vreg_wlan) {
		pr_err("%s: VREG L13 get failed\n", __func__);
		return retval;
	}

	if(on) {
		printk("cm3623 power on\n");
		gpio_tlmm_config(GPIO_CFG(GPIO_IRQ_CM3623,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE); //keep gpio 142 to pull up state
		#ifdef CONFIG_CM3623_GPIO_I2C
		gpio_tlmm_config(GPIO_CFG(GPIO_I2C_SDA_CM3623, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(GPIO_I2C_SCL_CM3623, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
		#endif
		retval = vreg_set_level(vreg_gp4,3000); //set the voltage to 3 v
		if(retval) {
			pr_err("%s: VREG L10 set failed\n", __func__);
			goto fail_vreg_gp4;
		}
		retval = vreg_enable(vreg_gp4);
		if(retval) {
			pr_err("%s: VREG L10 enable failed\n", __func__);
			goto fail_vreg_gp4;
		}
		retval = vreg_set_level(vreg_wlan,3000); //set the voltage to 3 v
		if(retval) {
			pr_err("%s: VREG L13 set failed\n", __func__);
			goto fail_vreg_wlan;
		}
		retval = vreg_enable(vreg_wlan);
		if(retval) {
			pr_err("%s: VREG L13 enable failed\n", __func__);
			goto fail_vreg_wlan;
		}
	}	
	else {
		printk("cm3623 power off\n");
		retval = vreg_disable(vreg_gp4);
		if(retval) {
			pr_err("%s: VREG L10 enable failed\n", __func__);
			goto fail_vreg_gp4;
		}		
		retval = vreg_disable(vreg_wlan);
		if(retval) {
			pr_err("%s: VREG L13 enable failed\n", __func__);
			goto fail_vreg_wlan;
		}
		gpio_tlmm_config(GPIO_CFG(GPIO_IRQ_CM3623,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE); //keep gpio 142 to pull up state
		#ifdef CONFIG_CM3623_GPIO_I2C
		gpio_tlmm_config(GPIO_CFG(GPIO_I2C_SDA_CM3623, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(GPIO_I2C_SCL_CM3623, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
		#endif
	}
	return 0;

fail_vreg_wlan:
	vreg_disable(vreg_gp4);
fail_vreg_gp4:
	return	retval;
}

static struct cm3623_platform_data cm3623_platform_data = {
	.gpio_int = GPIO_IRQ_CM3623,
	.power = cm3623_power,
};
#endif
//[SIMT-caoxiangteng-110715]}

static struct i2c_board_info msm_i2c_board_info[] = {
	{
		I2C_BOARD_INFO("m33c01", OPTNAV_I2C_SLAVE_ADDR),
		.irq		= MSM_GPIO_TO_INT(OPTNAV_IRQ),
		.platform_data = &optnav_data,
	},
	{
		I2C_BOARD_INFO("adv7520", ADV7520_I2C_ADDR),
		.platform_data = &adv7520_hdmi_data,
	},
#ifdef CONFIG_BOSCH_BMA222
        {
                I2C_BOARD_INFO(BMA222_DEV_NAME,BMA222_I2C_ADDR),
        },
#endif
#ifdef CONFIG_BOSCH_BMA250
        {
                I2C_BOARD_INFO(BMA250_DEV_NAME,BMA250_I2C_ADDR),
        },
#endif
#ifdef CONFIG_BATTERY_MAX17040_P01D
        {
                I2C_BOARD_INFO("max17040-p01d", 0x36),
        },
#endif
#ifdef CONFIG_BATTERY_MAX17040
        {
                I2C_BOARD_INFO("max17040",0x36),
        },
#endif
#ifdef CONFIG_SENSORS_MMC31XX
#include <linux/mmc31xx.h>
        {
                I2C_BOARD_INFO(MMC31XX_I2C_NAME,MMC31XX_I2C_ADDR),
                /* + qiukj add for ponyo project*/
                //.platform_data = &mmc31xx_platform_data,
                /* - qiukj add for ponyo project*/
        },
#endif
#ifdef CONFIG_SENSORS_MMC328X
        {
                I2C_BOARD_INFO(MMC328X_I2C_NAME,MMC328X_I2C_ADDR),
                /* + qiukj add for ponyo project*/
                .platform_data = &mmc328x_platform_data,
                /* - qiukj add for ponyo project*/
        },
#endif
	//[SIMT-caoxiangteng-110715]{
	/*
	* add cm3623 platform data 
	*/
#if !defined(CONFIG_CM3623_GPIO_I2C) && defined(CONFIG_CM2623)
	{
		I2C_BOARD_INFO(CM3623_NAME, CM3623_I2C_ADDR),
		.platform_data = &cm3623_platform_data,
	},
#endif
	//[SIMT-caoxiangteng-110715]}
};

static struct i2c_board_info msm_marimba_board_info[] = {
	{
		I2C_BOARD_INFO("marimba", 0xc),
		.platform_data = &marimba_pdata,
	}
};

#ifdef CONFIG_USB_FUNCTION
static struct usb_function_map usb_functions_map[] = {
	{"diag", 0},
	{"adb", 1},
	{"modem", 2},
	{"nmea", 3},
	{"mass_storage", 4},
	{"ethernet", 5},
};

static struct usb_composition usb_func_composition[] = {
	{
		.product_id         = 0x9012,
		.functions	    = 0x5, /* 0101 */
	},

	{
		.product_id         = 0x9013,
		.functions	    = 0x15, /* 10101 */
	},

	{
		.product_id         = 0x9014,
		.functions	    = 0x30, /* 110000 */
	},

	{
		.product_id         = 0x9016,
		.functions	    = 0xD, /* 01101 */
	},

	{
		.product_id         = 0x9017,
		.functions	    = 0x1D, /* 11101 */
	},

	{
		.product_id         = 0xF000,
		.functions	    = 0x10, /* 10000 */
	},

	{
		.product_id         = 0xF009,
		.functions	    = 0x20, /* 100000 */
	},

	{
		.product_id         = 0x9018,
		.functions	    = 0x1F, /* 011111 */
	},

};
static struct msm_hsusb_platform_data msm_hsusb_pdata = {
	.version	= 0x0100,
	.phy_info	= USB_PHY_INTEGRATED | USB_PHY_MODEL_45NM,
	.vendor_id	= 0x5c6,
	.product_name	= "Qualcomm HSUSB Device",
	.serial_number	= "1234567890ABCDEF",
	.manufacturer_name
			= "Qualcomm Incorporated",
	.compositions	= usb_func_composition,
	.num_compositions
			= ARRAY_SIZE(usb_func_composition),
	.function_map	= usb_functions_map,
	.num_functions	= ARRAY_SIZE(usb_functions_map),
	.core_clk	= 1,
};
#endif

static struct msm_handset_platform_data hs_platform_data = {
	.hs_name = "7k_handset",
	.pwr_key_delay_ms = 500, /* 0 will disable end key */
};

static struct platform_device hs_device = {
	.name   = "msm-handset",
	.id     = -1,
	.dev    = {
		.platform_data = &hs_platform_data,
	},
};

static struct msm_pm_platform_data msm_pm_data[MSM_PM_SLEEP_MODE_NR] = {
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].supported = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].suspend_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].idle_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].latency = 8594,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].residency = 23740,

	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].supported = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].suspend_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].idle_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].latency = 4594,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].residency = 23740,

	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE].supported = 1,
#ifdef CONFIG_MSM_STANDALONE_POWER_COLLAPSE
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE].suspend_enabled = 0,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE].idle_enabled = 1,
#else /*CONFIG_MSM_STANDALONE_POWER_COLLAPSE*/
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE].suspend_enabled = 0,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE].idle_enabled = 0,
#endif /*CONFIG_MSM_STANDALONE_POWER_COLLAPSE*/
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE].latency = 500,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE].residency = 6000,

	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].supported = 1,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].suspend_enabled
		= 1,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].idle_enabled = 0,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency = 443,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].residency = 1098,

	[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT].supported = 1,
	[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT].suspend_enabled = 1,
	[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT].idle_enabled = 1,
	[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT].latency = 2,
	[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT].residency = 0,
};

static struct resource qsd_spi_resources[] = {
	{
		.name   = "spi_irq_in",
		.start	= INT_SPI_INPUT,
		.end	= INT_SPI_INPUT,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name   = "spi_irq_out",
		.start	= INT_SPI_OUTPUT,
		.end	= INT_SPI_OUTPUT,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name   = "spi_irq_err",
		.start	= INT_SPI_ERROR,
		.end	= INT_SPI_ERROR,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name   = "spi_base",
		.start	= 0xA8000000,
		.end	= 0xA8000000 + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name   = "spidm_channels",
		.flags  = IORESOURCE_DMA,
	},
	{
		.name   = "spidm_crci",
		.flags  = IORESOURCE_DMA,
	},
};

#define AMDH0_BASE_PHYS		0xAC200000
#define ADMH0_GP_CTL		(ct_adm_base + 0x3D8)
static int msm_qsd_spi_dma_config(void)
{
	void __iomem *ct_adm_base = 0;
	u32 spi_mux = 0;
	int ret = 0;

	ct_adm_base = ioremap(AMDH0_BASE_PHYS, PAGE_SIZE);
	if (!ct_adm_base) {
		pr_err("%s: Could not remap %x\n", __func__, AMDH0_BASE_PHYS);
		return -ENOMEM;
	}

// wanggang modified for ponyo irda 20110403 
#ifdef CONFIG_USE_IRDA
	spi_mux = ioread32(ADMH0_GP_CTL)&(0xffffcfff);      
	iowrite32(spi_mux,ADMH0_GP_CTL);
	spi_mux = 0;
#else
	spi_mux = (ioread32(ADMH0_GP_CTL) & (0x3 << 12)) >> 12;
#endif

	qsd_spi_resources[4].start  = DMOV_USB_CHAN;
	qsd_spi_resources[4].end    = DMOV_TSIF_CHAN;

	switch (spi_mux) {
	case (1):
		qsd_spi_resources[5].start  = DMOV_HSUART1_RX_CRCI;
		qsd_spi_resources[5].end    = DMOV_HSUART1_TX_CRCI;
		break;
	case (2):
		qsd_spi_resources[5].start  = DMOV_HSUART2_RX_CRCI;
		qsd_spi_resources[5].end    = DMOV_HSUART2_TX_CRCI;
		break;
	case (3):
		qsd_spi_resources[5].start  = DMOV_CE_OUT_CRCI;
		qsd_spi_resources[5].end    = DMOV_CE_IN_CRCI;
		break;
	default:
		ret = -ENOENT;
	}

	iounmap(ct_adm_base);

	return ret;
}

static struct platform_device qsd_device_spi = {
	.name		= "spi_qsd",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(qsd_spi_resources),
	.resource	= qsd_spi_resources,
};

#ifdef CONFIG_SPI_QSD
static struct spi_board_info lcdc_toshiba_spi_board_info[] __initdata = {
	{
		.modalias       = "lcdc_toshiba_ltm030dd40",
		.mode           = SPI_MODE_3|SPI_CS_HIGH,
		.bus_num        = 0,
		.chip_select    = 0,
		.max_speed_hz   = 9963243,
	}
};
#endif

static struct msm_gpio qsd_spi_gpio_config_data[] = {
	{ GPIO_CFG(45, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_clk" },
	{ GPIO_CFG(46, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_cs0" },
	{ GPIO_CFG(47, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_8MA), "spi_mosi" },
	{ GPIO_CFG(48, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_miso" },
};

static int msm_qsd_spi_gpio_config(void)
{
	return msm_gpios_request_enable(qsd_spi_gpio_config_data,
		ARRAY_SIZE(qsd_spi_gpio_config_data));
}

static void msm_qsd_spi_gpio_release(void)
{
	msm_gpios_disable_free(qsd_spi_gpio_config_data,
		ARRAY_SIZE(qsd_spi_gpio_config_data));
}

static struct msm_spi_platform_data qsd_spi_pdata = {
	.max_clock_speed = 26331429,
	.gpio_config  = msm_qsd_spi_gpio_config,
	.gpio_release = msm_qsd_spi_gpio_release,
	.dma_config = msm_qsd_spi_dma_config,
};

static void __init msm_qsd_spi_init(void)
{
	qsd_device_spi.dev.platform_data = &qsd_spi_pdata;
}

#ifdef CONFIG_USB_EHCI_MSM_72K
static void msm_hsusb_vbus_power(unsigned phy_info, int on)
{
        int rc;
        static int vbus_is_on;
        struct pm8058_gpio usb_vbus = {
                .direction      = PM_GPIO_DIR_OUT,
                .pull           = PM_GPIO_PULL_NO,
                .output_buffer  = PM_GPIO_OUT_BUF_CMOS,
                .output_value   = 1,
                .vin_sel        = 2,
                .out_strength   = PM_GPIO_STRENGTH_MED,
                .function       = PM_GPIO_FUNC_NORMAL,
                .inv_int_pol    = 0,
        };

        /* If VBUS is already on (or off), do nothing. */
        if (unlikely(on == vbus_is_on))
                return;

        if (on) {
                rc = pm8058_gpio_config(36, &usb_vbus);
                if (rc) {
                        pr_err("%s PMIC GPIO 36 write failed\n", __func__);
                        return;
                }
	} else {
		gpio_set_value_cansleep(PM8058_GPIO_PM_TO_SYS(36), 0);
	}

        vbus_is_on = on;
}

static struct msm_usb_host_platform_data msm_usb_host_pdata = {
        .phy_info   = (USB_PHY_INTEGRATED | USB_PHY_MODEL_45NM),
        .vbus_power = msm_hsusb_vbus_power,
        .power_budget   = 180,
};
#endif

#ifdef CONFIG_USB_MSM_OTG_72K
static int hsusb_rpc_connect(int connect)
{
	if (connect)
		return msm_hsusb_rpc_connect();
	else
		return msm_hsusb_rpc_close();
}
#endif

#ifdef CONFIG_USB_MSM_OTG_72K
static struct vreg *vreg_3p3;
static int msm_hsusb_ldo_init(int init)
{
	uint32_t version = 0;
	int def_vol = 3400;

	version = socinfo_get_version();

	if (SOCINFO_VERSION_MAJOR(version) >= 2 &&
			SOCINFO_VERSION_MINOR(version) >= 1) {
		def_vol = 3075;
		pr_debug("%s: default voltage:%d\n", __func__, def_vol);
	}

	if (init) {
		vreg_3p3 = vreg_get(NULL, "usb");
		if (IS_ERR(vreg_3p3))
			return PTR_ERR(vreg_3p3);
		vreg_set_level(vreg_3p3, def_vol);
	} else
		vreg_put(vreg_3p3);

	return 0;
}

static int msm_hsusb_ldo_enable(int enable)
{
	static int ldo_status;

	if (!vreg_3p3 || IS_ERR(vreg_3p3))
		return -ENODEV;

	if (ldo_status == enable)
		return 0;

	ldo_status = enable;

	if (enable)
		return vreg_enable(vreg_3p3);

	return vreg_disable(vreg_3p3);
}

static int msm_hsusb_ldo_set_voltage(int mV)
{
	static int cur_voltage = 3400;

	if (!vreg_3p3 || IS_ERR(vreg_3p3))
		return -ENODEV;

	if (cur_voltage == mV)
		return 0;

	cur_voltage = mV;

	pr_debug("%s: (%d)\n", __func__, mV);

	return vreg_set_level(vreg_3p3, mV);
}
#endif

#ifndef CONFIG_USB_EHCI_MSM_72K
static int msm_hsusb_pmic_notif_init(void (*callback)(int online), int init);
#endif
static struct msm_otg_platform_data msm_otg_pdata = {
	.rpc_connect	= hsusb_rpc_connect,

#ifndef CONFIG_USB_EHCI_MSM_72K
	.pmic_vbus_notif_init         = msm_hsusb_pmic_notif_init,
#else
	.vbus_power = msm_hsusb_vbus_power,
#endif
	.core_clk		 = 1,
	.pemp_level		 = PRE_EMPHASIS_WITH_20_PERCENT,
	.cdr_autoreset		 = CDR_AUTO_RESET_DISABLE,
	.drv_ampl		 = HS_DRV_AMPLITUDE_DEFAULT,
	.se1_gating		 = SE1_GATING_DISABLE,
	.chg_vbus_draw		 = hsusb_chg_vbus_draw,
	.chg_connected		 = hsusb_chg_connected,
	.chg_init		 = hsusb_chg_init,
	.ldo_enable		 = msm_hsusb_ldo_enable,
	.ldo_init		 = msm_hsusb_ldo_init,
	.ldo_set_voltage	 = msm_hsusb_ldo_set_voltage,
};

#ifdef CONFIG_USB_GADGET
static struct msm_hsusb_gadget_platform_data msm_gadget_pdata = {
	.is_phy_status_timer_on = 1,
};
#endif
#ifndef CONFIG_USB_EHCI_MSM_72K
typedef void (*notify_vbus_state) (int);
notify_vbus_state notify_vbus_state_func_ptr;
int vbus_on_irq;
static irqreturn_t pmic_vbus_on_irq(int irq, void *data)
{
	pr_info("%s: vbus notification from pmic\n", __func__);

	(*notify_vbus_state_func_ptr) (1);

	return IRQ_HANDLED;
}
static int msm_hsusb_pmic_notif_init(void (*callback)(int online), int init)
{
	int ret;

	if (init) {
		if (!callback)
			return -ENODEV;

		notify_vbus_state_func_ptr = callback;
		vbus_on_irq = platform_get_irq_byname(&msm_device_otg,
			"vbus_on");
		if (vbus_on_irq <= 0) {
			pr_err("%s: unable to get vbus on irq\n", __func__);
			return -ENODEV;
		}

		ret = request_any_context_irq(vbus_on_irq, pmic_vbus_on_irq,
			IRQF_TRIGGER_RISING, "msm_otg_vbus_on", NULL);
		if (ret < 0) {
			pr_info("%s: request_irq for vbus_on"
				"interrupt failed\n", __func__);
			return ret;
		}
		msm_otg_pdata.pmic_vbus_irq = vbus_on_irq;
		return 0;
	} else {
		free_irq(vbus_on_irq, 0);
		notify_vbus_state_func_ptr = NULL;
		return 0;
	}
}
#endif

static struct android_pmem_platform_data android_pmem_pdata = {
	.name = "pmem",
	.allocator_type = PMEM_ALLOCATORTYPE_ALLORNOTHING,
	.cached = 1,
};

static struct platform_device android_pmem_device = {
	.name = "android_pmem",
	.id = 0,
	.dev = { .platform_data = &android_pmem_pdata },
};

#ifndef CONFIG_SPI_QSD
static int lcdc_gpio_array_num[] = {
				45, /* spi_clk */
				46, /* spi_cs  */
				47, /* spi_mosi */
				48, /* spi_miso */
				};

static struct msm_gpio lcdc_gpio_config_data[] = {
	{ GPIO_CFG(45, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_clk" },
	{ GPIO_CFG(46, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_cs0" },
	{ GPIO_CFG(47, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_mosi" },
	{ GPIO_CFG(48, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_miso" },
};

static void lcdc_config_gpios(int enable)
{
	if (enable) {
		msm_gpios_request_enable(lcdc_gpio_config_data,
					      ARRAY_SIZE(
						      lcdc_gpio_config_data));
	} else
		msm_gpios_disable_free(lcdc_gpio_config_data,
					    ARRAY_SIZE(
						    lcdc_gpio_config_data));
}
#endif

static struct msm_panel_common_pdata lcdc_sharp_panel_data = {
#ifndef CONFIG_SPI_QSD
	.panel_config_gpio = lcdc_config_gpios,
	.gpio_num          = lcdc_gpio_array_num,
#endif
	.gpio = 2, 	/* LPG PMIC_GPIO26 channel number */
};

static struct platform_device lcdc_sharp_panel_device = {
	.name   = "lcdc_sharp_wvga",
	.id     = 0,
	.dev    = {
		.platform_data = &lcdc_sharp_panel_data,
	}
};

static struct msm_gpio dtv_panel_irq_gpios[] = {
	{ GPIO_CFG(18, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA),
		"hdmi_int" },
};

static struct msm_gpio dtv_panel_gpios[] = {
	{ GPIO_CFG(160, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_data5" },
	{ GPIO_CFG(161, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_data6" },
	{ GPIO_CFG(162, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_data7" },
	{ GPIO_CFG(163, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_data8" },
	{ GPIO_CFG(164, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_data9" },
	{ GPIO_CFG(165, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat10" },
	{ GPIO_CFG(166, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat11" },
	{ GPIO_CFG(167, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat12" },
	{ GPIO_CFG(168, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat13" },
	{ GPIO_CFG(169, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat14" },
	{ GPIO_CFG(170, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat15" },
	{ GPIO_CFG(171, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat16" },
	{ GPIO_CFG(172, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat17" },
	{ GPIO_CFG(173, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat18" },
	{ GPIO_CFG(174, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat19" },
	{ GPIO_CFG(175, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat20" },
	{ GPIO_CFG(176, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat21" },
	{ GPIO_CFG(177, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat22" },
	{ GPIO_CFG(178, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_4MA), "dtv_dat23" },
};


static int gpio_set(const char *label, const char *name, int level, int on)
{
	struct vreg *vreg = vreg_get(NULL, label);
	int rc;

	if (IS_ERR(vreg)) {
		rc = PTR_ERR(vreg);
		pr_err("%s: vreg %s get failed (%d)\n",
			__func__, name, rc);
		return rc;
	}

	rc = vreg_set_level(vreg, level);
	if (rc) {
		pr_err("%s: vreg %s set level failed (%d)\n",
			__func__, name, rc);
		return rc;
	}

	if (on)
		rc = vreg_enable(vreg);
	else
		rc = vreg_disable(vreg);
	if (rc)
		pr_err("%s: vreg %s enable failed (%d)\n",
			__func__, name, rc);
	return rc;
}

static int hdmi_comm_power(int on, int show)
{
	int rc = gpio_set("gp7", "LDO8", 1800, on);
	if (rc) {
		pr_warning("hdmi_comm_power: LDO8 gpio failed: rc=%d\n", rc);
		return rc;
	}
	rc = gpio_set("gp4", "LDO10", 2600, on);
	if (rc)
		pr_warning("hdmi_comm_power: LDO10 gpio failed: rc=%d\n", rc);
	if (show)
		pr_info("%s: i2c comm: %d <LDO8+LDO10>\n", __func__, on);
	return rc;
}

static int hdmi_init_irq(void)
{
	int rc = msm_gpios_enable(dtv_panel_irq_gpios,
			ARRAY_SIZE(dtv_panel_irq_gpios));
	if (rc < 0) {
		pr_err("%s: gpio enable failed: %d\n", __func__, rc);
		return rc;
	}
	pr_info("%s\n", __func__);

	return 0;
}

static int hdmi_enable_5v(int on)
{
	int pmic_gpio_hdmi_5v_en ;

	pmic_gpio_hdmi_5v_en = PMIC_GPIO_HDMI_5V_EN_V3 ;

	pr_info("%s: %d\n", __func__, on);
	if (on) {
		int rc;
		rc = gpio_request(PM8058_GPIO_PM_TO_SYS(pmic_gpio_hdmi_5v_en),
			"hdmi_5V_en");
		if (rc) {
			pr_err("%s PMIC_GPIO_HDMI_5V_EN gpio_request failed\n",
				__func__);
			return rc;
		}
		gpio_set_value_cansleep(
			PM8058_GPIO_PM_TO_SYS(pmic_gpio_hdmi_5v_en), 1);
	} else {
		gpio_set_value_cansleep(
			PM8058_GPIO_PM_TO_SYS(pmic_gpio_hdmi_5v_en), 0);
		gpio_free(PM8058_GPIO_PM_TO_SYS(pmic_gpio_hdmi_5v_en));
	}
	return 0;
}

static int hdmi_core_power(int on, int show)
{
	if (show)
		pr_info("%s: %d <LDO8>\n", __func__, on);
	return gpio_set("gp7", "LDO8", 1800, on);
}

static int hdmi_cec_power(int on)
{
	pr_info("%s: %d <LDO17>\n", __func__, on);
	return gpio_set("gp11", "LDO17", 2600, on);
}

static bool hdmi_check_hdcp_hw_support(void)
{
	return true;
}

static int dtv_panel_power(int on)
{
	int flag_on = !!on;
	static int dtv_power_save_on;
	int rc;

	if (dtv_power_save_on == flag_on)
		return 0;

	dtv_power_save_on = flag_on;
	pr_info("%s: %d\n", __func__, on);

	if (on) {
		rc = msm_gpios_enable(dtv_panel_gpios,
				ARRAY_SIZE(dtv_panel_gpios));
		if (rc < 0) {
			printk(KERN_ERR "%s: gpio enable failed: %d\n",
				__func__, rc);
			return rc;
		}
	} else {
		rc = msm_gpios_disable(dtv_panel_gpios,
				ARRAY_SIZE(dtv_panel_gpios));
		if (rc < 0) {
			printk(KERN_ERR "%s: gpio disable failed: %d\n",
				__func__, rc);
			return rc;
		}
	}

	mdelay(5);		/* ensure power is stable */

	return rc;
}

static struct lcdc_platform_data dtv_pdata = {
	.lcdc_power_save   = dtv_panel_power,
};

static struct msm_serial_hs_platform_data msm_uart_dm1_pdata = {
       .inject_rx_on_wakeup = 1,
       .rx_to_inject = 0xFD,
};
// wanggang add for ponyo irda 20110403 start
#ifdef CONFIG_USE_IRDA
static struct msm_serial_hs_platform_data msm_uart_dm2_pdata = {
       .inject_rx_on_wakeup = 1,
       .rx_to_inject = 0xFD,
};
#endif
// wanggang add for ponyo irda 20110403 end

static struct resource msm_fb_resources[] = {
	{
		.flags  = IORESOURCE_DMA,
	}
};
//[SIMT-lilening-20110804] add lcd auto detect{
static int msm_fb_detect_panel(const char *name)
{
	if (!strcmp(name, "lcdc_lg4573_wvga"))
		return 0;
	else if (!strncmp(name, "mddi_toshiba_wvga_pt", 20))
		return -EPERM;
	else if (!strncmp(name, "lcdc_toshiba_wvga_pt", 20))
		return -EPERM;
	else if (!strcmp(name, "mddi_orise"))
		return -EPERM;
	else if (!strcmp(name, "mddi_quickvx"))
		return -EPERM;
	return -ENODEV;
}
//[SIMT-lilening-20110804] add lcd auto detect}
static struct msm_fb_platform_data msm_fb_pdata = {
	.detect_client = msm_fb_detect_panel,
	.mddi_prescan = 1,
};

static struct platform_device msm_fb_device = {
	.name   = "msm_fb",
	.id     = 0,
	.num_resources  = ARRAY_SIZE(msm_fb_resources),
	.resource       = msm_fb_resources,
	.dev    = {
		.platform_data = &msm_fb_pdata,
	}
};

static struct platform_device msm_migrate_pages_device = {
	.name   = "msm_migrate_pages",
	.id     = -1,
};

static struct android_pmem_platform_data android_pmem_kernel_ebi1_pdata = {
       .name = PMEM_KERNEL_EBI1_DATA_NAME,
	/* if no allocator_type, defaults to PMEM_ALLOCATORTYPE_BITMAP,
	* the only valid choice at this time. The board structure is
	* set to all zeros by the C runtime initialization and that is now
	* the enum value of PMEM_ALLOCATORTYPE_BITMAP, now forced to 0 in
	* include/linux/android_pmem.h.
	*/
       .cached = 0,
};

static struct android_pmem_platform_data android_pmem_adsp_pdata = {
       .name = "pmem_adsp",
       .allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
       .cached = 0,
};

static struct android_pmem_platform_data android_pmem_audio_pdata = {
       .name = "pmem_audio",
       .allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
       .cached = 0,
};

static struct platform_device android_pmem_kernel_ebi1_device = {
       .name = "android_pmem",
       .id = 1,
       .dev = { .platform_data = &android_pmem_kernel_ebi1_pdata },
};

static struct platform_device android_pmem_adsp_device = {
       .name = "android_pmem",
       .id = 2,
       .dev = { .platform_data = &android_pmem_adsp_pdata },
};

static struct platform_device android_pmem_audio_device = {
       .name = "android_pmem",
       .id = 4,
       .dev = { .platform_data = &android_pmem_audio_pdata },
};

#if defined(CONFIG_CRYPTO_DEV_QCRYPTO) || \
		defined(CONFIG_CRYPTO_DEV_QCRYPTO_MODULE) || \
		defined(CONFIG_CRYPTO_DEV_QCEDEV) || \
		defined(CONFIG_CRYPTO_DEV_QCEDEV_MODULE)

#define QCE_SIZE		0x10000
#define QCE_0_BASE		0xA8400000

#define QCE_HW_KEY_SUPPORT	1
#define QCE_SHA_HMAC_SUPPORT	0
#define QCE_SHARE_CE_RESOURCE	0
#define QCE_CE_SHARED		0

static struct resource qcrypto_resources[] = {
	[0] = {
		.start = QCE_0_BASE,
		.end = QCE_0_BASE + QCE_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.name = "crypto_channels",
		.start = DMOV_CE_IN_CHAN,
		.end = DMOV_CE_OUT_CHAN,
		.flags = IORESOURCE_DMA,
	},
	[2] = {
		.name = "crypto_crci_in",
		.start = DMOV_CE_IN_CRCI,
		.end = DMOV_CE_IN_CRCI,
		.flags = IORESOURCE_DMA,
	},
	[3] = {
		.name = "crypto_crci_out",
		.start = DMOV_CE_OUT_CRCI,
		.end = DMOV_CE_OUT_CRCI,
		.flags = IORESOURCE_DMA,
	},
	[4] = {
		.name = "crypto_crci_hash",
		.start = DMOV_CE_HASH_CRCI,
		.end = DMOV_CE_HASH_CRCI,
		.flags = IORESOURCE_DMA,
	},
};

static struct resource qcedev_resources[] = {
	[0] = {
		.start = QCE_0_BASE,
		.end = QCE_0_BASE + QCE_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.name = "crypto_channels",
		.start = DMOV_CE_IN_CHAN,
		.end = DMOV_CE_OUT_CHAN,
		.flags = IORESOURCE_DMA,
	},
	[2] = {
		.name = "crypto_crci_in",
		.start = DMOV_CE_IN_CRCI,
		.end = DMOV_CE_IN_CRCI,
		.flags = IORESOURCE_DMA,
	},
	[3] = {
		.name = "crypto_crci_out",
		.start = DMOV_CE_OUT_CRCI,
		.end = DMOV_CE_OUT_CRCI,
		.flags = IORESOURCE_DMA,
	},
	[4] = {
		.name = "crypto_crci_hash",
		.start = DMOV_CE_HASH_CRCI,
		.end = DMOV_CE_HASH_CRCI,
		.flags = IORESOURCE_DMA,
	},
};

#endif

#if defined(CONFIG_CRYPTO_DEV_QCRYPTO) || \
		defined(CONFIG_CRYPTO_DEV_QCRYPTO_MODULE)

static struct msm_ce_hw_support qcrypto_ce_hw_suppport = {
	.ce_shared = QCE_CE_SHARED,
	.shared_ce_resource = QCE_SHARE_CE_RESOURCE,
	.hw_key_support = QCE_HW_KEY_SUPPORT,
	.sha_hmac = QCE_SHA_HMAC_SUPPORT,
};

static struct platform_device qcrypto_device = {
	.name		= "qcrypto",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(qcrypto_resources),
	.resource	= qcrypto_resources,
	.dev		= {
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data = &qcrypto_ce_hw_suppport,
	},
};
#endif

#if defined(CONFIG_CRYPTO_DEV_QCEDEV) || \
		defined(CONFIG_CRYPTO_DEV_QCEDEV_MODULE)

static struct msm_ce_hw_support qcedev_ce_hw_suppport = {
	.ce_shared = QCE_CE_SHARED,
	.shared_ce_resource = QCE_SHARE_CE_RESOURCE,
	.hw_key_support = QCE_HW_KEY_SUPPORT,
	.sha_hmac = QCE_SHA_HMAC_SUPPORT,
};
static struct platform_device qcedev_device = {
	.name		= "qce",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(qcedev_resources),
	.resource	= qcedev_resources,
	.dev		= {
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data = &qcedev_ce_hw_suppport,
	},
};
#endif

static int mddi_toshiba_pmic_bl(int level)
{
	int ret = -EPERM;

	ret = pmic_set_led_intensity(LED_LCD, level);

	if (ret)
		printk(KERN_WARNING "%s: can't set lcd backlight!\n",
					__func__);
	return ret;
}

static struct msm_panel_common_pdata mddi_toshiba_pdata = {
	.pmic_backlight = mddi_toshiba_pmic_bl,
};

static struct platform_device mddi_toshiba_device = {
	.name   = "mddi_toshiba",
	.id     = 0,
	.dev    = {
		.platform_data = &mddi_toshiba_pdata,
	}
};

static unsigned wega_reset_gpio =
	GPIO_CFG(180, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA);

static unsigned char quickvx_mddi_client = 1, other_mddi_client = 1;
static unsigned char quickvx_ldo_enabled;

static unsigned quickvx_vlp_gpio =
	GPIO_CFG(97, 0, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL,	GPIO_CFG_2MA);

static struct pm8058_gpio pmic_quickvx_clk_gpio = {
	.direction      = PM_GPIO_DIR_OUT,
	.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
	.output_value   = 1,
	.pull           = PM_GPIO_PULL_NO,
	.vin_sel        = PM_GPIO_VIN_S3,
	.out_strength   = PM_GPIO_STRENGTH_HIGH,
	.function       = PM_GPIO_FUNC_2,
};

static struct msm_gpio bl_gpio_config_data[] = {
	{ GPIO_CFG(30, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "bl_ctrl" },
};

static void bl_config_gpios(void)
{
    int rc;
    rc = msm_gpios_enable(bl_gpio_config_data, ARRAY_SIZE(bl_gpio_config_data));
	if (rc < 0) {
		printk(KERN_ERR "%s: gpio enable failed: %d\n",
			__func__, rc);
	}
}

static void wlan_config_gpios(void)
{
	int rc;
	uint32_t wlan_reg_on = GPIO_CFG(WIFI_GPIO_ENABLE,
			0,
			GPIO_CFG_OUTPUT,
			GPIO_CFG_NO_PULL,
			GPIO_CFG_4MA);
	rc = gpio_tlmm_config(wlan_reg_on, GPIO_CFG_ENABLE);
	if (rc) {
		pr_err("%s: gpio_tlmm_config(%#x)=%d\n",__func__, wlan_reg_on, rc);
		return;
	}

	rc = gpio_request(WIFI_GPIO_ENABLE, "wlan_reg_on");
	if (rc) {
		pr_err("%s: unable to request gpio %d (%d)\n",
				__func__, WIFI_GPIO_ENABLE, rc);
	}
}

static int display_common_power(int on)
{
	int rc = 0, flag_on = !!on;
	static int display_common_power_save_on;
	struct vreg *vreg_ldo12, *vreg_ldo15 = NULL, *vreg_ldo6;
	struct vreg *vreg_ldo20, *vreg_ldo16;

	if (display_common_power_save_on == flag_on)
		return 0;

	display_common_power_save_on = flag_on;

	if (on) {
		/* reset Toshiba WeGA chip -- toggle reset pin -- gpio_180 */
		rc = gpio_tlmm_config(wega_reset_gpio, GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("%s: gpio_tlmm_config(%#x)=%d\n",
				       __func__, wega_reset_gpio, rc);
			return rc;
		}

		/* bring reset line low to hold reset*/
		gpio_set_value(180, 0);

		if (quickvx_mddi_client) {
			/* QuickVX chip -- VLP pin -- gpio 97 */
			rc = gpio_tlmm_config(quickvx_vlp_gpio,
				GPIO_CFG_ENABLE);
			if (rc) {
				pr_err("%s: gpio_tlmm_config(%#x)=%d\n",
					__func__, quickvx_vlp_gpio, rc);
				return rc;
			}

			/* bring QuickVX VLP line low */
			gpio_set_value(97, 0);

			rc = pm8058_gpio_config(PMIC_GPIO_QUICKVX_CLK,
				&pmic_quickvx_clk_gpio);
			if (rc) {
				pr_err("%s: pm8058_gpio_config(%#x)=%d\n",
					__func__, PMIC_GPIO_QUICKVX_CLK + 1,
					rc);
				return rc;
			}

			gpio_set_value_cansleep(PM8058_GPIO_PM_TO_SYS(
				PMIC_GPIO_QUICKVX_CLK), 0);
		}
	}

	/* Toshiba WeGA power -- has 3 power source */
	/* 1.5V -- LDO20*/
	vreg_ldo20 = vreg_get(NULL, "gp13");

	if (IS_ERR(vreg_ldo20)) {
		rc = PTR_ERR(vreg_ldo20);
		pr_err("%s: gp13 vreg get failed (%d)\n",
		       __func__, rc);
		return rc;
	}

	/* 1.8V -- LDO12 */
	vreg_ldo12 = vreg_get(NULL, "gp9");

	if (IS_ERR(vreg_ldo12)) {
		rc = PTR_ERR(vreg_ldo12);
		pr_err("%s: gp9 vreg get failed (%d)\n",
		       __func__, rc);
		return rc;
	}

	/* 2.6V -- LDO16 */
	vreg_ldo16 = vreg_get(NULL, "gp10");

	if (IS_ERR(vreg_ldo16)) {
		rc = PTR_ERR(vreg_ldo16);
		pr_err("%s: gp10 vreg get failed (%d)\n",
			   __func__, rc);
		return rc;
	}

	/* 3.075V -- LDO6 */
	vreg_ldo6 = vreg_get(NULL, "usb");

	if (IS_ERR(vreg_ldo6)) {
		rc = PTR_ERR(vreg_ldo6);
		pr_err("%s: usb vreg get failed (%d)\n",
			   __func__, rc);
		return rc;
	}

	/* lcd panel power */
	/* 3.1V -- LDO15 */
	vreg_ldo15 = vreg_get(NULL, "gp6");

	if (IS_ERR(vreg_ldo15)) {
		rc = PTR_ERR(vreg_ldo15);
		pr_err("%s: gp6 vreg get failed (%d)\n",
			__func__, rc);
		return rc;
	}

	/* For QuickLogic chip, LDO20 requires 1.8V */
	/* Toshiba chip requires 1.5V, but can tolerate 1.8V since max is 3V */
	if (quickvx_mddi_client)
		rc = vreg_set_level(vreg_ldo20, 1800);
	else
		rc = vreg_set_level(vreg_ldo20, 1500);
	if (rc) {
		pr_err("%s: vreg LDO20 set level failed (%d)\n",
		       __func__, rc);
		return rc;
	}

	rc = vreg_set_level(vreg_ldo12, 1800);
	if (rc) {
		pr_err("%s: vreg LDO12 set level failed (%d)\n",
		       __func__, rc);
		return rc;
	}

	if (other_mddi_client) {
		rc = vreg_set_level(vreg_ldo16, 2800);//modify value from 2.6 to 2.8 [simt-zhanghui-110810]
		if (rc) {
			pr_err("%s: vreg LDO16 set level failed (%d)\n",
				   __func__, rc);
			return rc;
		}
	}

	if (quickvx_mddi_client) {
		rc = vreg_set_level(vreg_ldo6, 3075);
		if (rc) {
			pr_err("%s: vreg LDO6 set level failed (%d)\n",
				   __func__, rc);
			return rc;
		}
	}

	// [SIMT-WY-20110718] adjust voltage for LCM lg4573
	rc = vreg_set_level(vreg_ldo15, 2800);
	if (rc) {
		pr_err("%s: vreg LDO15 set level failed (%d)\n",
			__func__, rc);
		return rc;
	}

	if (on) {
		rc = vreg_enable(vreg_ldo20);
		if (rc) {
			pr_err("%s: LDO20 vreg enable failed (%d)\n",
			       __func__, rc);
			return rc;
		}

		rc = vreg_enable(vreg_ldo12);
		if (rc) {
			pr_err("%s: LDO12 vreg enable failed (%d)\n",
			       __func__, rc);
			return rc;
		}

		if (other_mddi_client) {
			rc = vreg_enable(vreg_ldo16);
			if (rc) {
				pr_err("%s: LDO16 vreg enable failed (%d)\n",
					   __func__, rc);
				return rc;
			}
		}

		if (quickvx_mddi_client && quickvx_ldo_enabled) {
			/* Disable LDO6 during display ON */
			rc = vreg_disable(vreg_ldo6);
			if (rc) {
				pr_err("%s: LDO6 vreg disable failed (%d)\n",
					   __func__, rc);
				return rc;
			}
			quickvx_ldo_enabled = 0;
		}

		rc = vreg_enable(vreg_ldo15);
		if (rc) {
			pr_err("%s: LDO15 vreg enable failed (%d)\n",
				__func__, rc);
			return rc;
		}

		mdelay(5);		/* ensure power is stable */

		gpio_set_value(180, 1); /* bring reset line high */
		mdelay(10);	/* 10 msec before IO can be accessed */

		if (quickvx_mddi_client) {
			gpio_set_value(97, 1);
			msleep(2);
			gpio_set_value_cansleep(PM8058_GPIO_PM_TO_SYS(
				PMIC_GPIO_QUICKVX_CLK), 1);
			msleep(2);
		}

		rc = pmapp_display_clock_config(1);
		if (rc) {
			pr_err("%s pmapp_display_clock_config rc=%d\n",
					__func__, rc);
			return rc;
		}

	} else {
		rc = vreg_disable(vreg_ldo20);
		if (rc) {
			pr_err("%s: LDO20 vreg disable failed (%d)\n",
			       __func__, rc);
			return rc;
		}

		if (quickvx_mddi_client && !quickvx_ldo_enabled) {
			/* Enable LDO6 during display OFF for
			   Quicklogic chip to sleep with data retention */
			rc = vreg_enable(vreg_ldo6);
			if (rc) {
				pr_err("%s: LDO6 vreg enable failed (%d)\n",
					   __func__, rc);
				return rc;
			}
			quickvx_ldo_enabled = 1;
		}

		gpio_set_value(180, 0); /* bring reset line low */

		if (quickvx_mddi_client) {
			gpio_set_value(97, 0);
			gpio_set_value_cansleep(PM8058_GPIO_PM_TO_SYS(
				PMIC_GPIO_QUICKVX_CLK), 0);
		}

		rc = vreg_disable(vreg_ldo15);
		if (rc) {
			pr_err("%s: LDO15 vreg disable failed (%d)\n",
				__func__, rc);
			return rc;
		}

		mdelay(5);	/* ensure power is stable */
		
		rc = pmapp_display_clock_config(0);
		if (rc) {
			pr_err("%s pmapp_display_clock_config rc=%d\n",
					__func__, rc);
			return rc;
		}
	}

	return rc;
}

static int msm_fb_mddi_sel_clk(u32 *clk_rate)
{
	*clk_rate *= 2;
	return 0;
}

static int msm_fb_mddi_client_power(u32 client_id)
{
	struct vreg *vreg_ldo20, *vreg_ldo16;
	int rc;

	printk(KERN_NOTICE "\n client_id = 0x%x", client_id);
	/* Check if it is Quicklogic client */
	if (client_id == 0xc5835800) {
		printk(KERN_NOTICE "\n Quicklogic MDDI client");
		other_mddi_client = 0;
		vreg_ldo16 = vreg_get(NULL, "gp10");

		if (IS_ERR(vreg_ldo16)) {
			rc = PTR_ERR(vreg_ldo16);
			pr_err("%s: gp10 vreg get failed (%d)\n",
				   __func__, rc);
			return rc;
		}
	} else {
		printk(KERN_NOTICE "\n Non-Quicklogic MDDI client");
		quickvx_mddi_client = 0;
		gpio_set_value(97, 0);
		gpio_set_value_cansleep(PM8058_GPIO_PM_TO_SYS(
			PMIC_GPIO_QUICKVX_CLK), 0);

		vreg_ldo20 = vreg_get(NULL, "gp13");

		if (IS_ERR(vreg_ldo20)) {
			rc = PTR_ERR(vreg_ldo20);
			pr_err("%s: gp13 vreg get failed (%d)\n",
				   __func__, rc);
			return rc;
		}
		rc = vreg_set_level(vreg_ldo20, 1500);
		if (rc) {
			pr_err("%s: vreg LDO20 set level failed (%d)\n",
			       __func__, rc);
			return rc;
		}
	}
	return 0;
}

static struct mddi_platform_data mddi_pdata = {
	.mddi_power_save = display_common_power,
	.mddi_sel_clk = msm_fb_mddi_sel_clk,
	.mddi_client_power = msm_fb_mddi_client_power,
};

int mdp_core_clk_rate_table[] = {
	192000000,
	192000000,
	192000000,
	192000000,
};

static struct msm_panel_common_pdata mdp_pdata = {
	.hw_revision_addr = 0xac001270,
	.gpio = 30,
	.mdp_core_clk_rate = 192000000,
	.mdp_core_clk_table = mdp_core_clk_rate_table,
	.num_mdp_clk = ARRAY_SIZE(mdp_core_clk_rate_table),
};

static int lcd_panel_spi_gpio_num[] = {
			45, /* spi_clk */
			46, /* spi_cs  */
			47, /* spi_mosi */
			48, /* spi_miso */
		};

static struct msm_gpio lcd_panel_gpios[] = {
/* Workaround, since HDMI_INT is using the same GPIO line (18), and is used as
 * input.  if there is a hardware revision; we should reassign this GPIO to a
 * new open line; and removing it will just ensure that this will be missed in
 * the future.
	{ GPIO_CFG(18, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_grn0" },
 */
	{ GPIO_CFG(19, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_grn1" },
	{ GPIO_CFG(20, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_blu0" },
	{ GPIO_CFG(21, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_blu1" },
	{ GPIO_CFG(22, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_blu2" },
	{ GPIO_CFG(23, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_red0" },
	{ GPIO_CFG(24, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_red1" },
	{ GPIO_CFG(25, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_red2" },
#ifndef CONFIG_SPI_QSD
	{ GPIO_CFG(45, 0, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_clk" },
	{ GPIO_CFG(46, 0, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_cs0" },
	{ GPIO_CFG(47, 0, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_mosi" },
	{ GPIO_CFG(48, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_miso" },
#endif
	{ GPIO_CFG(90, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_pclk" },
	{ GPIO_CFG(91, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_en" },
	{ GPIO_CFG(92, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_vsync" },
	{ GPIO_CFG(93, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_hsync" },
	{ GPIO_CFG(94, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_grn2" },
	{ GPIO_CFG(95, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_grn3" },
	{ GPIO_CFG(96, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_grn4" },
	{ GPIO_CFG(97, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_grn5" },
	{ GPIO_CFG(98, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_grn6" },
	{ GPIO_CFG(99, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_grn7" },
	{ GPIO_CFG(100, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_blu3" },
	{ GPIO_CFG(101, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_blu4" },
	{ GPIO_CFG(102, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_blu5" },
	{ GPIO_CFG(103, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_blu6" },
	{ GPIO_CFG(104, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_blu7" },
	{ GPIO_CFG(105, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_red3" },
	{ GPIO_CFG(106, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_red4" },
	{ GPIO_CFG(107, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_red5" },
	{ GPIO_CFG(108, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_red6" },
	{ GPIO_CFG(109, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_red7" },
};

// [SIMT-WY-20110715] LCM lg4573 RGB interface GPIO config {
static struct msm_gpio lcd_lg4573_panel_gpios[] = {
	{ GPIO_CFG(22, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_blu2" },
	{ GPIO_CFG(25, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_red2" },
	{ GPIO_CFG(90, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_pclk" },
	{ GPIO_CFG(91, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_en" },
	{ GPIO_CFG(92, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_vsync" },
	{ GPIO_CFG(93, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_hsync" },
	{ GPIO_CFG(94, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_grn2" },
	{ GPIO_CFG(95, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_grn3" },
	{ GPIO_CFG(96, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_grn4" },
	{ GPIO_CFG(97, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_grn5" },
	{ GPIO_CFG(98, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_grn6" },
	{ GPIO_CFG(99, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_grn7" },
	{ GPIO_CFG(100, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_blu3" },
	{ GPIO_CFG(101, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_blu4" },
	{ GPIO_CFG(102, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_blu5" },
	{ GPIO_CFG(103, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_blu6" },
	{ GPIO_CFG(104, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_blu7" },
	{ GPIO_CFG(105, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_red3" },
	{ GPIO_CFG(106, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_red4" },
	{ GPIO_CFG(107, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_red5" },
	{ GPIO_CFG(108, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_red6" },
	{ GPIO_CFG(109, 1, GPIO_CFG_OUTPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "lcdc_red7" },
};
// [SIMT-WY-20110715] LCM lg4573 RGB interface gpio config }

static int lcdc_toshiba_panel_power(int on)
{
	int rc, i;
	struct msm_gpio *gp;

	rc = display_common_power(on);
	if (rc < 0) {
		printk(KERN_ERR "%s display_common_power failed: %d\n",
				__func__, rc);
		return rc;
	}

	if (on) {
		rc = msm_gpios_enable(lcd_panel_gpios,
				ARRAY_SIZE(lcd_panel_gpios));
		if (rc < 0) {
			printk(KERN_ERR "%s: gpio enable failed: %d\n",
					__func__, rc);
		}
	} else {	/* off */
		gp = lcd_panel_gpios;
		for (i = 0; i < ARRAY_SIZE(lcd_panel_gpios); i++) {
			/* ouput low */
			gpio_set_value(GPIO_PIN(gp->gpio_cfg), 0);
			gp++;
		}
	}

	return rc;
}

// [SIMT-lilening-20110812] LCM lg4573 power function {
static int lcdc_lg4573_panel_power(int on)
{
	int rc = 0, i;
	struct msm_gpio *gp;
	if (on){
		rc = display_common_power(on);
		if (rc < 0) {
			printk(KERN_ERR "%s display_common_power failed: %d\n",
					__func__, rc);
			return rc;
		}
	}
	if (on) {

		gpio_set_value(180, 1); /* bring reset line high */
		mdelay(10);	/* 10 msec before IO can be accessed */

		rc = pmapp_display_clock_config(1);
		if (rc) {
			pr_err("%s pmapp_display_clock_config rc=%d\n",
					__func__, rc);
			return rc;
		}

		rc = msm_gpios_enable(lcd_lg4573_panel_gpios,
				ARRAY_SIZE(lcd_lg4573_panel_gpios));
		if (rc < 0) {
			printk(KERN_ERR "%s: gpio enable failed: %d\n",
				__func__, rc);
		}
	} else {	/* off */

		rc = pmapp_display_clock_config(0);
		if (rc) {
			pr_err("%s pmapp_display_clock_config rc=%d\n",
					__func__, rc);
			return rc;
		}

		gp = lcd_lg4573_panel_gpios;
		for (i = 0; i < ARRAY_SIZE(lcd_lg4573_panel_gpios); i++) {
			/* ouput low */
			gpio_set_value(GPIO_PIN(gp->gpio_cfg), 0);
			gp++;
		}

		gpio_set_value(180, 0); /* bring reset line low */

	}

	return rc;
}
// [SIMT-lilening-20110812] LCM lg4573 power function }
static int lcdc_lg4573_gpio_array_num[] = {
	129, /* spi_clk */
	130, /* spi_cs  */
	132, /* spi_mosi */
	133, /* spi_miso */
};

static struct msm_gpio lcdc_lg4573_gpio_config_data[] = {
	{ GPIO_CFG(129, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_clk" },
	{ GPIO_CFG(130, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_cs0" },
	{ GPIO_CFG(132, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_mosi" },
	{ GPIO_CFG(133, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "spi_miso" },
};

static void lcdc_lg4573_config_gpios(int enable)
{
	if (enable) {
		msm_gpios_request_enable(lcdc_lg4573_gpio_config_data,
				ARRAY_SIZE(
					lcdc_lg4573_gpio_config_data));
	} else
		msm_gpios_disable_free(lcdc_lg4573_gpio_config_data,
				ARRAY_SIZE(
					lcdc_lg4573_gpio_config_data));
}


static struct msm_panel_common_pdata lcdc_lg4573_panel_data = {
	.panel_config_gpio = lcdc_lg4573_config_gpios,
	.gpio_num          = lcdc_lg4573_gpio_array_num,
	.gpio = 0,      /* LPG PMIC_GPIO24 channel number */
};


static struct platform_device lcdc_lg4573_panel_device = {
	.name   = "lcdc_lg4573_wvga",
	.id     = 0,
	.dev    = {
		.platform_data = &lcdc_lg4573_panel_data,
	}
};
//[SIMT-lilening-20110804] add lcd auto detect{
static int lcdc_panel_power(int on)
{
	int flag_on = !!on;
	static int lcdc_power_save_on;

	if (lcdc_power_save_on == flag_on)
		return 0;

	lcdc_power_save_on = flag_on;
	quickvx_mddi_client = 0;
    
	if(0 == msm_fb_detect_panel("lcdc_lg4573_wvga"))
		return lcdc_lg4573_panel_power(on);
	else if(0 == msm_fb_detect_panel("lcdc_toshiba_wvga_pt"))
		return lcdc_toshiba_panel_power(on);
	else
		return lcdc_lg4573_panel_power(on);
}
//[SIMT-lilening-20110804] add lcd auto detect}
static struct lcdc_platform_data lcdc_pdata = {
	.lcdc_power_save   = lcdc_panel_power,
};

static int atv_dac_power(int on)
{
	int rc = 0;
	struct vreg *vreg_s4, *vreg_ldo9;

	vreg_s4 = vreg_get(NULL, "s4");
	if (IS_ERR(vreg_s4)) {
		rc = PTR_ERR(vreg_s4);
		pr_err("%s: s4 vreg get failed (%d)\n",
			__func__, rc);
		return -1;
	}
	vreg_ldo9 = vreg_get(NULL, "gp1");
	if (IS_ERR(vreg_ldo9)) {
		rc = PTR_ERR(vreg_ldo9);
		pr_err("%s: ldo9 vreg get failed (%d)\n",
			__func__, rc);
		return rc;
	}

	if (on) {
		rc = vreg_enable(vreg_s4);
		if (rc) {
			pr_err("%s: s4 vreg enable failed (%d)\n",
				__func__, rc);
			return rc;
		}
		rc = vreg_enable(vreg_ldo9);
		if (rc) {
			pr_err("%s: ldo9 vreg enable failed (%d)\n",
				__func__, rc);
			return rc;
		}
	} else {
		rc = vreg_disable(vreg_ldo9);
		if (rc) {
			pr_err("%s: ldo9 vreg disable failed (%d)\n",
				   __func__, rc);
			return rc;
		}
		rc = vreg_disable(vreg_s4);
		if (rc) {
			pr_err("%s: s4 vreg disable failed (%d)\n",
				   __func__, rc);
			return rc;
		}
	}
	return rc;
}

static struct tvenc_platform_data atv_pdata = {
	.poll		 = 1,
	.pm_vid_en	 = atv_dac_power,
};

static void __init msm_fb_add_devices(void)
{
	msm_fb_register_device("mdp", &mdp_pdata);
	msm_fb_register_device("pmdh", &mddi_pdata);
	msm_fb_register_device("lcdc", &lcdc_pdata);
	msm_fb_register_device("dtv", &dtv_pdata);
	msm_fb_register_device("tvenc", &atv_pdata);
#ifdef CONFIG_FB_MSM_TVOUT
	msm_fb_register_device("tvout_device", NULL);
#endif
}

static struct msm_panel_common_pdata lcdc_toshiba_panel_data = {
	.gpio_num          = lcd_panel_spi_gpio_num,
};

static struct platform_device lcdc_toshiba_panel_device = {
	.name   = "lcdc_toshiba_wvga",
	.id     = 0,
	.dev    = {
		.platform_data = &lcdc_toshiba_panel_data,
	}
};

//Add by TaoYuan for bluetooth suspend 2011.3.28
static struct resource bluesleep_resources[] = {
	{
		.name	= "gpio_host_wake",
		.start	= 44,
		.end	= 44,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "gpio_ext_wake",
		.start	= 125,
		.end	= 125,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "host_wake",
		.start	= MSM_GPIO_TO_INT(44),
		.end	= MSM_GPIO_TO_INT(44),
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device msm_bluesleep_device = {
	.name = "bluesleep",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(bluesleep_resources),
	.resource	= bluesleep_resources,
};


#if defined(CONFIG_MARIMBA_CORE) && \
   (defined(CONFIG_MSM_BT_POWER) || defined(CONFIG_MSM_BT_POWER_MODULE))
static struct platform_device msm_bt_power_device = {
	.name = "bt_power",
	.id     = -1
};

enum {
	BT_RFR,
	BT_CTS,
	BT_RX,
	BT_TX,
};

static struct msm_gpio bt_config_power_on[] = {
	{ GPIO_CFG(44, 0, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL,   GPIO_CFG_2MA),
    "BT_WAKE_HOST" },
	/*Add by TaoYuan for bluetooth HOST_WAKE_BT & BT_WAKE_HOST
	GPIO125 & GPIO44 -- 2011.3.28*/
	{ GPIO_CFG(125, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,   GPIO_CFG_2MA),
		"HOST_WAKE_BT" },
	/*Add by TaoYuan for bluetooth REG_ON_GPIO127
	and REG_RESET_GPIO128 -- 2011.3.25*/
	{ GPIO_CFG(127, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,   GPIO_CFG_2MA),
		"BCM4330_REG_ON" },
	{ GPIO_CFG(128, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,   GPIO_CFG_2MA),
		"BCM4330_REG_RESET" },
	{ GPIO_CFG(134, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,   GPIO_CFG_2MA),
		"UART1DM_RFR" },
	{ GPIO_CFG(135, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL,   GPIO_CFG_2MA),
		"UART1DM_CTS" },
	{ GPIO_CFG(136, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL,   GPIO_CFG_2MA),
		"UART1DM_Rx" },
	{ GPIO_CFG(137, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,   GPIO_CFG_2MA),
		"UART1DM_Tx" },
	/*Add by TaoYuan for bluetooth PCM Configuration
	GPIO138-141-- 2011.3.28*/
	{ GPIO_CFG(138, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,   GPIO_CFG_2MA),
		"AUX_PCM_DOUT" },
	{ GPIO_CFG(139, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,   GPIO_CFG_2MA),
		"AUX_PCM_DIN" },
	{ GPIO_CFG(140, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,   GPIO_CFG_2MA),
		"AUX_PCM_SYNC" },
	{ GPIO_CFG(141, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,   GPIO_CFG_2MA),
		"AUX_PCM_CLK" },
};

static struct msm_gpio bt_config_power_off[] = {
	/*Add by TaoYuan for bluetooth HOST_WAKE_BT & BT_WAKE_HOST
	GPIO125 & GPIO44 -- 2011.3.28*/
	{ GPIO_CFG(44, 0, GPIO_CFG_INPUT,  GPIO_CFG_PULL_DOWN,   GPIO_CFG_2MA),
		"BT_WAKE_HOST" },
	{ GPIO_CFG(125, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN,   GPIO_CFG_2MA),
		"HOST_WAKE_BT" },
	/*Add by TaoYuan for bluetooth REG_ON_GPIO127
	and REG_RESET_GPIO128 -- 2011.3.25*/
	{ GPIO_CFG(127, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,   GPIO_CFG_2MA),
		"BCM4330_REG_ON" },
	{ GPIO_CFG(128, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,   GPIO_CFG_2MA),
		"BCM4330_REG_RESET" },
	{ GPIO_CFG(134, 0, GPIO_CFG_INPUT,  GPIO_CFG_PULL_DOWN,   GPIO_CFG_2MA),
		"UART1DM_RFR" },
	{ GPIO_CFG(135, 0, GPIO_CFG_INPUT,  GPIO_CFG_PULL_DOWN,   GPIO_CFG_2MA),
		"UART1DM_CTS" },
	{ GPIO_CFG(136, 0, GPIO_CFG_INPUT,  GPIO_CFG_PULL_DOWN,   GPIO_CFG_2MA),
		"UART1DM_Rx" },
	{ GPIO_CFG(137, 0, GPIO_CFG_INPUT,  GPIO_CFG_PULL_DOWN,   GPIO_CFG_2MA),
		"UART1DM_Tx" },
	/*Add by TaoYuan for bluetooth PCM Configuration
	GPIO138-141-- 2011.3.28*/
	{ GPIO_CFG(138, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN,   GPIO_CFG_2MA),
		"AUX_PCM_DOUT" },
	{ GPIO_CFG(139, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN,   GPIO_CFG_2MA),
		"AUX_PCM_DIN" },
	{ GPIO_CFG(140, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN,   GPIO_CFG_2MA),
		"AUX_PCM_SYNC" },
	{ GPIO_CFG(141, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN,   GPIO_CFG_2MA),
		"AUX_PCM_CLK" },
};

static const char *vregs_bt_marimba_name[] = {
	"s3",
	"s2",
	"gp16",
	"wlan"
};
static struct vreg *vregs_bt_marimba[ARRAY_SIZE(vregs_bt_marimba_name)];

static const char *vregs_bt_bahama_name[] = {
	"s3",
	"usb2",
	"s2",
	"wlan"
};
static struct vreg *vregs_bt_bahama[ARRAY_SIZE(vregs_bt_bahama_name)];

static int bluetooth_power(int on)
{
	int rc;

	if (on) {
		rc = msm_gpios_enable(bt_config_power_on,
			ARRAY_SIZE(bt_config_power_on));

		if (rc < 0)
			return rc;
		//Add by TaoYuan for BCM4330 PowerOn 2011.3.25
		BCM4330_PowerOn();

	} else {
		rc = msm_gpios_enable(bt_config_power_off,
					ARRAY_SIZE(bt_config_power_off));
		if (rc < 0)
			return rc;

		/* check for initial RFKILL block (power off) */
		if (platform_get_drvdata(&msm_bt_power_device) == NULL)
			goto out;
		//Modified by TaoYuan 2011.3.25
		BCM4330_PowerOff();
	}

out:
	printk(KERN_DEBUG "Bluetooth power switch: %d\n", on);

	return 0;
}

static void __init bt_power_init(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(vregs_bt_marimba_name); i++) {
		vregs_bt_marimba[i] = vreg_get(NULL, vregs_bt_marimba_name[i]);
		if (IS_ERR(vregs_bt_marimba[i])) {
			printk(KERN_ERR "%s: vreg get %s failed (%ld)\n",
			       __func__, vregs_bt_marimba_name[i],
			       PTR_ERR(vregs_bt_marimba[i]));
			return;
		}
	}

	for (i = 0; i < ARRAY_SIZE(vregs_bt_bahama_name); i++) {
		vregs_bt_bahama[i] = vreg_get(NULL, vregs_bt_bahama_name[i]);
		if (IS_ERR(vregs_bt_bahama[i])) {
			printk(KERN_ERR "%s: vreg get %s failed (%ld)\n",
			       __func__, vregs_bt_bahama_name[i],
			       PTR_ERR(vregs_bt_bahama[i]));
			return;
		}
	}

    msm_bt_power_device.dev.platform_data = &bluetooth_power;
}
#else
#define bt_power_init(x) do {} while (0)
#endif

extern struct wifi_platform_data bcm4330_wifi_control;
static struct platform_device bcm4330_wifi_device = {
	.name           = "bcmdhd_wlan",
	.id             = 1,
	.dev            = {
		.platform_data = &bcm4330_wifi_control,
	},
};


#ifdef CONFIG_BATTERY_MAX17040_P01D
extern int max17040_p01d_get_capacity(void);
static u32 msm_calculate_batt_capacity(u32 current_voltage)
{
	return max17040_p01d_get_capacity();
}
#endif

static struct msm_psy_batt_pdata msm_psy_batt_data = {
	.voltage_min_design 	= 3500,
	.voltage_max_design	= MSM_BATTERY_FULL,
	.avail_chg_sources   	= AC_CHG | USB_CHG ,
	.batt_technology        = POWER_SUPPLY_TECHNOLOGY_LION,
#ifdef CONFIG_BATTERY_MAX17040_P01D
	.calculate_capacity	= &msm_calculate_batt_capacity,
#endif
};

static struct platform_device msm_batt_device = {
	.name 		    = "msm-battery",
	.id		    = -1,
	.dev.platform_data  = &msm_psy_batt_data,
};

struct i2c_gpio_platform_data platform_data_gpio_i2c = {
	.sda_pin = GPIO_I2C_SDA, //124,
	.scl_pin = GPIO_I2C_SCL, //123,
};

struct platform_device msm_device_gpio_i2c = {
	.name   = "i2c-gpio",
	.id             = 10,
	.dev    = {
		.platform_data = &platform_data_gpio_i2c,
	}
};

//[SIMT-caoxiangteng-110715]{
/*
 *   gpio i2c simulate 
 */
#ifdef CONFIG_CM3623_GPIO_I2C
struct i2c_gpio_platform_data platform_data_gpio_i2c_cm3623 = {
	.sda_pin = GPIO_I2C_SDA_CM3623, //121,
	.scl_pin = GPIO_I2C_SCL_CM3623, //120,
};

struct platform_device msm_device_gpio_i2c_cm3623 = {
	.name   = "i2c-gpio",
	.id             = 11,
	.dev    = {
		.platform_data = &platform_data_gpio_i2c_cm3623,
	}
};
#endif
//[SIMT-caoxiangteng-110715]}


static char *msm_adc_surf_device_names[] = {
	"XO_ADC",
};

static struct msm_adc_platform_data msm_adc_pdata;

static struct platform_device msm_adc_device = {
	.name   = "msm_adc",
	.id = -1,
	.dev = {
		.platform_data = &msm_adc_pdata,
	},
};

#ifdef CONFIG_MSM_SDIO_AL
static struct msm_gpio mdm2ap_status = {
	GPIO_CFG(77, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	"mdm2ap_status"
};


static int configure_mdm2ap_status(int on)
{
	if (on)
		return msm_gpios_request_enable(&mdm2ap_status, 1);
	else {
		msm_gpios_disable_free(&mdm2ap_status, 1);
		return 0;
	}
}

static int get_mdm2ap_status(void)
{
	return gpio_get_value(GPIO_PIN(mdm2ap_status.gpio_cfg));
}

static struct sdio_al_platform_data sdio_al_pdata = {
	.config_mdm2ap_status = configure_mdm2ap_status,
	.get_mdm2ap_status = get_mdm2ap_status,
	.allow_sdioc_version_major_2 = 1,
	.peer_sdioc_version_minor = 0x0001,
	.peer_sdioc_version_major = 0x0003,
	.peer_sdioc_boot_version_minor = 0x0001,
	.peer_sdioc_boot_version_major = 0x0003,
};

struct platform_device msm_device_sdio_al = {
	.name = "msm_sdio_al",
	.id = -1,
	.dev		= {
		.platform_data	= &sdio_al_pdata,
	},
};

#endif /* CONFIG_MSM_SDIO_AL */

static struct platform_device *devices[] __initdata = {
#if defined(CONFIG_SERIAL_MSM) || defined(CONFIG_MSM_SERIAL_DEBUGGER)
	&msm_device_uart2,
#endif
	&msm_device_smd,
	&msm_device_dmov,
	&msm_device_nand,
#ifdef CONFIG_USB_FUNCTION
	&msm_device_hsusb_peripheral,
	&mass_storage_device,
#endif
#ifdef CONFIG_USB_MSM_OTG_72K
	&msm_device_otg,
#ifdef CONFIG_USB_GADGET
	&msm_device_gadget_peripheral,
#endif
#endif
#ifdef CONFIG_USB_ANDROID
	&usb_mass_storage_device,
	&rndis_device,
#ifdef CONFIG_USB_ANDROID_DIAG
	&usb_diag_device,
#endif
#ifdef CONFIG_USB_F_SERIAL
	&usb_gadget_fserial_device,
#endif
	&android_usb_device,
#endif
	&qsd_device_spi,
#ifdef CONFIG_I2C_SSBI
	&msm_device_ssbi6,
	&msm_device_ssbi7,
#endif
	&android_pmem_device,
	&msm_fb_device,
	&msm_migrate_pages_device,
	

	&msm_device_gpio_i2c,
	//[SIMT-caoxiangteng-110715]{
#ifdef CONFIG_CM3623_GPIO_I2C
	/*
	* add i2c gpio devices
	*/
	&msm_device_gpio_i2c_cm3623,
#endif 
	//[SIMT-caoxiangteng-110715]}
	&lcdc_lg4573_panel_device,


	&mddi_toshiba_device,
	&lcdc_toshiba_panel_device,
#ifdef CONFIG_MSM_ROTATOR
	&msm_rotator_device,
#endif
	&lcdc_sharp_panel_device,
	&android_pmem_kernel_ebi1_device,
	&android_pmem_adsp_device,
	&android_pmem_audio_device,
	&msm_device_i2c,
	&msm_device_i2c_2,
	&msm_device_uart_dm1,
// wanggang add for ponyo irda 20110403 add
#ifdef CONFIG_USE_IRDA
	&msm_device_uart_dm2,
#endif
// wanggang add for ponyo irda 20110403 end
	&hs_device,
#ifdef CONFIG_MSM7KV2_AUDIO
	&msm_aictl_device,
	&msm_mi2s_device,
	&msm_lpa_device,
	&msm_aux_pcm_device,
#endif
	&msm_device_adspdec,
	&qup_device_i2c,
#if defined(CONFIG_MARIMBA_CORE) && \
   (defined(CONFIG_MSM_BT_POWER) || defined(CONFIG_MSM_BT_POWER_MODULE))
	&msm_bt_power_device,
	&bcm4330_wifi_device,
#endif
	&msm_kgsl_3d0,
#ifdef CONFIG_MSM_KGSL_2D
	&msm_kgsl_2d0,
#endif
#ifdef CONFIG_MT9T013
	&msm_camera_sensor_mt9t013,
#endif
#ifdef CONFIG_MT9D112
	&msm_camera_sensor_mt9d112,
#endif
#ifdef CONFIG_OV7690
	&msm_camera_sensor_ov7690,
#endif
#ifdef CONFIG_MT9D111SY
	&msm_camera_sensor_mt9d111SY,
#endif
#ifdef CONFIG_WEBCAM_OV9726
	&msm_camera_sensor_ov9726,
#endif
#ifdef CONFIG_S5K3E2FX
	&msm_camera_sensor_s5k3e2fx,
#endif
#ifdef CONFIG_MT9P012
	&msm_camera_sensor_mt9p012,
#endif
#ifdef CONFIG_MT9E013
	&msm_camera_sensor_mt9e013,
#endif
#ifdef CONFIG_VX6953
	&msm_camera_sensor_vx6953,
#endif
#ifdef CONFIG_SN12M0PZ
	&msm_camera_sensor_sn12m0pz,
#endif
#ifdef CONFIG_M6MO
	&msm_camera_sensor_m6mo,
#endif
//Add by Taoyuan for bluetooth suspend 2011.3.28
         &msm_bluesleep_device,
	&msm_device_vidc_720p,
#ifdef CONFIG_MSM_GEMINI
	&msm_gemini_device,
#endif
#ifdef CONFIG_MSM_VPE
	&msm_vpe_device,
#endif
#if defined(CONFIG_TSIF) || defined(CONFIG_TSIF_MODULE)
	&msm_device_tsif,
#endif
#ifdef CONFIG_MSM_SDIO_AL
	&msm_device_sdio_al,
#endif

#if defined(CONFIG_CRYPTO_DEV_QCRYPTO) || \
		defined(CONFIG_CRYPTO_DEV_QCRYPTO_MODULE)
	&qcrypto_device,
#endif

#if defined(CONFIG_CRYPTO_DEV_QCEDEV) || \
		defined(CONFIG_CRYPTO_DEV_QCEDEV_MODULE)
	&qcedev_device,
#endif

	&msm_batt_device,
	&msm_adc_device,
	&msm_ebi0_thermal,
	&msm_ebi1_thermal,
	&keypad_device_headset,
};

static struct msm_gpio msm_i2c_gpios_hw[] = {
	{ GPIO_CFG(70, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "i2c_scl" },
	{ GPIO_CFG(71, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "i2c_sda" },
};

static struct msm_gpio msm_i2c_gpios_io[] = {
	{ GPIO_CFG(70, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "i2c_scl" },
	{ GPIO_CFG(71, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "i2c_sda" },
};

static struct msm_gpio qup_i2c_gpios_io[] = {
	{ GPIO_CFG(16, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "qup_scl" },
	{ GPIO_CFG(17, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "qup_sda" },
};
static struct msm_gpio qup_i2c_gpios_hw[] = {
	{ GPIO_CFG(16, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "qup_scl" },
	{ GPIO_CFG(17, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "qup_sda" },
};

static void
msm_i2c_gpio_config(int adap_id, int config_type)
{
	struct msm_gpio *msm_i2c_table;

	/* Each adapter gets 2 lines from the table */
	if (adap_id > 0)
		return;
	if (config_type)
		msm_i2c_table = &msm_i2c_gpios_hw[adap_id*2];
	else
		msm_i2c_table = &msm_i2c_gpios_io[adap_id*2];
	msm_gpios_enable(msm_i2c_table, 2);
}
/*This needs to be enabled only for OEMS*/
#ifndef CONFIG_QUP_EXCLUSIVE_TO_CAMERA
static struct vreg *qup_vreg;
#endif
static void
qup_i2c_gpio_config(int adap_id, int config_type)
{
	int rc = 0;
	struct msm_gpio *qup_i2c_table;
	/* Each adapter gets 2 lines from the table */
	if (adap_id != 4)
		return;
	if (config_type)
		qup_i2c_table = qup_i2c_gpios_hw;
	else
		qup_i2c_table = qup_i2c_gpios_io;
	rc = msm_gpios_enable(qup_i2c_table, 2);
	if (rc < 0)
		printk(KERN_ERR "QUP GPIO enable failed: %d\n", rc);
	/*This needs to be enabled only for OEMS*/
#ifndef CONFIG_QUP_EXCLUSIVE_TO_CAMERA
	if (qup_vreg) {
		int rc = vreg_set_level(qup_vreg, 1800);
		if (rc) {
			pr_err("%s: vreg LVS1 set level failed (%d)\n",
			__func__, rc);
		}
		rc = vreg_enable(qup_vreg);
		if (rc) {
			pr_err("%s: vreg_enable() = %d \n",
			__func__, rc);
		}
	}
#endif
}

static struct msm_i2c_platform_data msm_i2c_pdata = {
	.clk_freq = 100000,
	.pri_clk = 70,
	.pri_dat = 71,
	.rmutex  = 1,
	.rsl_id = "D:I2C02000021",
	.msm_i2c_config_gpio = msm_i2c_gpio_config,
};

static void __init msm_device_i2c_init(void)
{
	if (msm_gpios_request(msm_i2c_gpios_hw, ARRAY_SIZE(msm_i2c_gpios_hw)))
		pr_err("failed to request I2C gpios\n");

	msm_device_i2c.dev.platform_data = &msm_i2c_pdata;
}

static struct msm_i2c_platform_data msm_i2c_2_pdata = {
	.clk_freq = 100000,
	.rmutex  = 1,
	.rsl_id = "D:I2C02000022",
	.msm_i2c_config_gpio = msm_i2c_gpio_config,
};

static void __init msm_device_i2c_2_init(void)
{
	msm_device_i2c_2.dev.platform_data = &msm_i2c_2_pdata;
}
#ifdef CONFIG_SN12M0PZ
static struct msm_i2c_platform_data qup_i2c_pdata = {
	.clk_freq = 100000,//384000,
	.pclk = "camif_pad_pclk",
	.msm_i2c_config_gpio = qup_i2c_gpio_config,
};
#else
static struct msm_i2c_platform_data qup_i2c_pdata = {
	.clk_freq = 384000,
	.pclk = "camif_pad_pclk",
	.msm_i2c_config_gpio = qup_i2c_gpio_config,
};
#endif

static void __init qup_device_i2c_init(void)
{
	if (msm_gpios_request(qup_i2c_gpios_hw, ARRAY_SIZE(qup_i2c_gpios_hw)))
		pr_err("failed to request I2C gpios\n");

	qup_device_i2c.dev.platform_data = &qup_i2c_pdata;
	/*This needs to be enabled only for OEMS*/
#ifndef CONFIG_QUP_EXCLUSIVE_TO_CAMERA
	qup_vreg = vreg_get(NULL, "lvsw1");
	if (IS_ERR(qup_vreg)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
			__func__, PTR_ERR(qup_vreg));
	}
#endif
}

#ifdef CONFIG_I2C_SSBI
static struct msm_i2c_ssbi_platform_data msm_i2c_ssbi6_pdata = {
	.rsl_id = "D:PMIC_SSBI",
	.controller_type = MSM_SBI_CTRL_SSBI2,
};

static struct msm_i2c_ssbi_platform_data msm_i2c_ssbi7_pdata = {
	.rsl_id = "D:CODEC_SSBI",
	.controller_type = MSM_SBI_CTRL_SSBI,
};
#endif

static struct msm_acpu_clock_platform_data msm7x30_clock_data = {
	.acpu_switch_time_us = 50,
	.vdd_switch_time_us = 62,
};

//[SIMT-caoxiangteng-110715]{
/*
 * add gpio i2c devices
 */
#ifdef CONFIG_CM3623_GPIO_I2C
static struct i2c_board_info cm3623_i2c_devices[] = {
	{
		I2C_BOARD_INFO(CM3623_NAME, CM3623_I2C_ADDR),
		.platform_data = &cm3623_platform_data,
	},
};
#endif
//[SIMT-caoxiangteng-110715]}

static void __init msm7x30_init_irq(void)
{
	msm_init_irq();
}

static struct msm_gpio msm_nand_ebi2_cfg_data[] = {
	{GPIO_CFG(86, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "ebi2_cs1"},
	{GPIO_CFG(115, 2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "ebi2_busy1"},
};

struct vreg *vreg_s3;
struct vreg *vreg_mmc;

#if (defined(CONFIG_MMC_MSM_SDC1_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC2_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC3_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC4_SUPPORT))

struct sdcc_gpio {
	struct msm_gpio *cfg_data;
	uint32_t size;
	struct msm_gpio *sleep_cfg_data;
};
#if defined(CONFIG_MMC_MSM_SDC1_SUPPORT)
static struct msm_gpio sdc1_lvlshft_cfg_data[] = {
	{GPIO_CFG(35, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_16MA), "sdc1_lvlshft"},
};
#endif
static struct msm_gpio sdc1_cfg_data[] = {
	{GPIO_CFG(38, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "sdc1_clk"},
	{GPIO_CFG(39, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc1_cmd"},
	{GPIO_CFG(40, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc1_dat_3"},
	{GPIO_CFG(41, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "gpio_41_in"},// [SIMT-qiukejun-110727] add this configuration for detecting version for ponyo
	{GPIO_CFG(42, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc1_dat_1"},
	{GPIO_CFG(43, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "gpio_43_in"},// [SIMT-qiukejun-110727] add this configuration for detecting version for ponyo
};

static struct msm_gpio sdc2_cfg_data[] = {
};

static struct msm_gpio sdc3_cfg_data[] = {
	{GPIO_CFG(110, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "sdc3_clk"},
	{GPIO_CFG(111, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc3_cmd"},
	{GPIO_CFG(116, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc3_dat_3"},
	{GPIO_CFG(117, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc3_dat_2"},
	{GPIO_CFG(118, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc3_dat_1"},
	{GPIO_CFG(119, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), "sdc3_dat_0"},
};

static struct msm_gpio sdc3_sleep_cfg_data[] = {
	{GPIO_CFG(110, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			"sdc3_clk"},
	{GPIO_CFG(111, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			"sdc3_cmd"},
	{GPIO_CFG(116, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			"sdc3_dat_3"},
	{GPIO_CFG(117, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			"sdc3_dat_2"},
	{GPIO_CFG(118, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			"sdc3_dat_1"},
	{GPIO_CFG(119, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			"sdc3_dat_0"},
};

//[SIMT-yuhaipeng-20110930] change drive-current to 12mA for more stable data transfer
static struct msm_gpio sdc4_cfg_data[] = {
	{GPIO_CFG(58, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), "sdc4_clk"},
	{GPIO_CFG(59, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_12MA), "sdc4_cmd"},
	{GPIO_CFG(60, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_12MA), "sdc4_dat_3"},
	{GPIO_CFG(61, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_12MA), "sdc4_dat_2"},
	{GPIO_CFG(62, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_12MA), "sdc4_dat_1"},
	{GPIO_CFG(63, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_12MA), "sdc4_dat_0"},
};

//add by yuhaipeng-20110628, 
static struct msm_gpio sdc4_sleep_cfg_data[] = {
	{GPIO_CFG(58, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "sdc4_clk"},
	{GPIO_CFG(59, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "sdc4_cmd"},
	{GPIO_CFG(60, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "sdc4_dat_3"},
	{GPIO_CFG(61, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "sdc4_dat_2"},
	{GPIO_CFG(62, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "sdc4_dat_1"},
	{GPIO_CFG(63, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "sdc4_dat_0"},
};

static struct sdcc_gpio sdcc_cfg_data[] = {
	{
		.cfg_data = sdc1_cfg_data,
		.size = ARRAY_SIZE(sdc1_cfg_data),
		.sleep_cfg_data = NULL,
	},
	{
		.cfg_data = sdc2_cfg_data,
		.size = ARRAY_SIZE(sdc2_cfg_data),
		.sleep_cfg_data = NULL,
	},
	{
		.cfg_data = sdc3_cfg_data,
		.size = ARRAY_SIZE(sdc3_cfg_data),
		.sleep_cfg_data = sdc3_sleep_cfg_data,
	},
	{
		.cfg_data = sdc4_cfg_data,
		.size = ARRAY_SIZE(sdc4_cfg_data),
//		.sleep_cfg_data = NULL,
		.sleep_cfg_data = sdc4_sleep_cfg_data, //add by yuhaipeng
	},
};

struct sdcc_vreg {
	struct vreg *vreg_data;
	unsigned level;
};

static struct sdcc_vreg sdcc_vreg_data[4];

static unsigned long vreg_sts, gpio_sts;

static uint32_t msm_sdcc_setup_gpio(int dev_id, unsigned int enable)
{
	int rc = 0;
	struct sdcc_gpio *curr;

	curr = &sdcc_cfg_data[dev_id - 1];

	if (!(test_bit(dev_id, &gpio_sts)^enable))
		return rc;

	if (enable) {
		set_bit(dev_id, &gpio_sts);
		rc = msm_gpios_request_enable(curr->cfg_data, curr->size);
		if (rc)
			printk(KERN_ERR "%s: Failed to turn on GPIOs for slot %d\n",
				__func__,  dev_id);
	} else {
		clear_bit(dev_id, &gpio_sts);
		if (curr->sleep_cfg_data) {
			msm_gpios_enable(curr->sleep_cfg_data, curr->size);
			msm_gpios_free(curr->sleep_cfg_data, curr->size);
		} else {
			msm_gpios_disable_free(curr->cfg_data, curr->size);
		}
	}

	return rc;
}

static uint32_t msm_sdcc_setup_vreg(int dev_id, unsigned int enable)
{
	int rc = 0;
	struct sdcc_vreg *curr;
	static int enabled_once[] = {0, 0, 0, 0};

	curr = &sdcc_vreg_data[dev_id - 1];

	if (!(test_bit(dev_id, &vreg_sts)^enable))
		return rc;

	if (!enable || enabled_once[dev_id - 1])
		return 0;

	if (enable) {
		set_bit(dev_id, &vreg_sts);
		rc = vreg_set_level(curr->vreg_data, curr->level);
		if (rc) {
			printk(KERN_ERR "%s: vreg_set_level() = %d \n",
					__func__, rc);
		}
		rc = vreg_enable(curr->vreg_data);
		if (rc) {
			printk(KERN_ERR "%s: vreg_enable() = %d \n",
					__func__, rc);
		}
		enabled_once[dev_id - 1] = 1;
	} else {
		clear_bit(dev_id, &vreg_sts);
		rc = vreg_disable(curr->vreg_data);
		if (rc) {
			printk(KERN_ERR "%s: vreg_disable() = %d \n",
					__func__, rc);
		}
	}
	return rc;
}

static uint32_t msm_sdcc_setup_power(struct device *dv, unsigned int vdd)
{
	int rc = 0;
	struct platform_device *pdev;

	pdev = container_of(dv, struct platform_device, dev);
	rc = msm_sdcc_setup_gpio(pdev->id, (vdd ? 1 : 0));
	if (rc)
		goto out;

	if (pdev->id == 4) /* S3 is always ON and cannot be disabled */
		rc = msm_sdcc_setup_vreg(pdev->id, (vdd ? 1 : 0));
out:
	return rc;
}

#endif

#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
static unsigned int msm7x30_sdcc_slot_status(struct device *dev)
{
	return (unsigned int)
		gpio_get_value_cansleep(
			PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_SD_DET - 1));
}
#endif

static int msm_sdcc_get_wpswitch(struct device *dv)
{
	return -1;
}

//Add by Yuhaipeng 201100617,Set the sdcard_gpios when suspend/resume
//copied from msm_hsusb_vbus_power.
static int msm_cfg_sdcard_power(struct device *dev, bool on)
{
	if (on) {
	    printk("----%s----%d---PMIC_GPIO_SDC4_PWR_EN_N on--\n",__func__,__LINE__);
		gpio_set_value_cansleep(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_SDC4_PWR_EN_N), 0);
		//msm_sdcc_setup_gpio(4, 1);
	} else {
		//msm_sdcc_setup_gpio(4, 0);
	    printk("----%s----%d---PMIC_GPIO_SDC4_PWR_EN_N off--\n",__func__,__LINE__);
		gpio_set_value_cansleep(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_SDC4_PWR_EN_N), 1);
	}
	printk("----%s----%d-----\n",__func__,__LINE__);

	return 0;
}

#endif

#if defined(CONFIG_MMC_MSM_SDC1_SUPPORT)
static struct mmc_platform_data msm7x30_sdc1_data = {
	.ocr_mask	= MMC_VDD_165_195,
	.translate_vdd	= msm_sdcc_setup_power,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
#ifdef CONFIG_MMC_MSM_SDC1_DUMMY52_REQUIRED
	.dummy52_required = 1,
#endif
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 24576000,
	.msmsdcc_fmax	= 49152000,
	.nonremovable	= 0,
};
#endif

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
static struct mmc_platform_data msm7x30_sdc2_data = {
	.ocr_mask	= MMC_VDD_165_195 | MMC_VDD_27_28,
	.translate_vdd	= msm_sdcc_setup_power,
#ifdef CONFIG_MMC_MSM_SDC2_8_BIT_SUPPORT
	.mmc_bus_width  = MMC_CAP_8_BIT_DATA,
#else
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
#endif
#ifdef CONFIG_MMC_MSM_SDC2_DUMMY52_REQUIRED
	.dummy52_required = 1,
#endif
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 24576000,
	.msmsdcc_fmax	= 49152000,
	.nonremovable	= 1,
};
#endif

#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
static struct mmc_platform_data msm7x30_sdc3_data = {
	.ocr_mask	= MMC_VDD_27_28 | MMC_VDD_28_29,
	.translate_vdd	= msm_sdcc_setup_power,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
#ifdef CONFIG_MMC_MSM_SDIO_SUPPORT
//	.sdiowakeup_irq = MSM_GPIO_TO_INT(118), //Yuhaipeng 20110506
#endif
#ifdef CONFIG_MMC_MSM_SDC3_DUMMY52_REQUIRED
	.dummy52_required = 1,
#endif
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 24576000,
//	.msmsdcc_fmax	= 49152000,
	.msmsdcc_fmax	= 24576000,//Yuhaipeng 20110506
	.nonremovable	= 0,
};
#endif

#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
static struct mmc_platform_data msm7x30_sdc4_data = {
	.ocr_mask	= MMC_VDD_27_28 | MMC_VDD_28_29,
	.translate_vdd	= msm_sdcc_setup_power,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
	.status      = msm7x30_sdcc_slot_status,
	.status_irq  = PM8058_GPIO_IRQ(PMIC8058_IRQ_BASE, PMIC_GPIO_SD_DET - 1),
	.irq_flags   = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
#endif
	.wpswitch    = msm_sdcc_get_wpswitch,
	.cfg_sdcard_power = msm_cfg_sdcard_power, //Add by Yuhaipeng 20110617,Set the sdcard_gpios when suspend/resume
#ifdef CONFIG_MMC_MSM_SDC4_DUMMY52_REQUIRED
	.dummy52_required = 1,
#endif
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 24576000,
	.msmsdcc_fmax	= 49152000,
	.nonremovable	= 0,
};
#endif

#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
static void msm_sdc1_lvlshft_enable(void)
{
	int rc;

	/* Enable LDO5, an input to the FET that powers slot 1 */
	rc = vreg_set_level(vreg_mmc, 2850);
	if (rc)
		printk(KERN_ERR "%s: vreg_set_level() = %d \n",	__func__, rc);

	rc = vreg_enable(vreg_mmc);
	if (rc)
		printk(KERN_ERR "%s: vreg_enable() = %d \n", __func__, rc);

	/* Enable GPIO 35, to turn on the FET that powers slot 1 */
	rc = msm_gpios_request_enable(sdc1_lvlshft_cfg_data,
				ARRAY_SIZE(sdc1_lvlshft_cfg_data));
	if (rc)
		printk(KERN_ERR "%s: Failed to enable GPIO 35\n", __func__);

	rc = gpio_direction_output(GPIO_PIN(sdc1_lvlshft_cfg_data[0].gpio_cfg),
				1);
	if (rc)
		printk(KERN_ERR "%s: Failed to turn on GPIO 35\n", __func__);
}
#endif

static void __init msm7x30_init_mmc(void)
{
	vreg_s3 = vreg_get(NULL, "s3");
	if (IS_ERR(vreg_s3)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
		       __func__, PTR_ERR(vreg_s3));
		return;
	}

	vreg_mmc = vreg_get(NULL, "mmc");
	if (IS_ERR(vreg_mmc)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
		       __func__, PTR_ERR(vreg_mmc));
		return;
	}

#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
	sdcc_vreg_data[0].vreg_data = vreg_s3;
	sdcc_vreg_data[0].level = 1800;
	msm_add_sdcc(1, &msm7x30_sdc1_data);
#endif
#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
	sdcc_vreg_data[1].vreg_data = vreg_s3;
	sdcc_vreg_data[1].level = 1800;
	msm_add_sdcc(2, &msm7x30_sdc2_data);
#endif
#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
	sdcc_vreg_data[2].vreg_data = vreg_s3;
	sdcc_vreg_data[2].level = 1800;
	msm_sdcc_setup_gpio(3, 1);
	msm_add_sdcc(3, &msm7x30_sdc3_data);
#endif
#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
	sdcc_vreg_data[3].vreg_data = vreg_mmc;
	sdcc_vreg_data[3].level = 2850;
	msm_add_sdcc(4, &msm7x30_sdc4_data);
#endif

}

static void __init msm7x30_init_nand(void)
{
	char *build_id;
	struct flash_platform_data *plat_data;

	build_id = socinfo_get_build_id();
	if (build_id == NULL) {
		pr_err("%s: Build ID not available from socinfo\n", __func__);
		return;
	}

	if (build_id[8] == 'C' &&
			!msm_gpios_request_enable(msm_nand_ebi2_cfg_data,
			ARRAY_SIZE(msm_nand_ebi2_cfg_data))) {
		plat_data = msm_device_nand.dev.platform_data;
		plat_data->interleave = 1;
		printk(KERN_INFO "%s: Interleave mode Build ID found\n",
			__func__);
	}
}

#ifdef CONFIG_SERIAL_MSM_CONSOLE
static struct msm_gpio uart2_config_data[] = {
	{ GPIO_CFG(51, 2, GPIO_CFG_INPUT,   GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "UART2_Rx"},
	{ GPIO_CFG(52, 2, GPIO_CFG_OUTPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "UART2_Tx"},
};

static void msm7x30_init_uart2(void)
{
	msm_gpios_request_enable(uart2_config_data,
			ARRAY_SIZE(uart2_config_data));

}
#endif

#ifdef CONFIG_USE_IRDA
static struct msm_gpio uart2dm_b_config_data[] = {
       { GPIO_CFG(85, 3, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "UART2DM_RX_DATA_B"},
       { GPIO_CFG(87, 3, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "UART2DM_TX_DATA_B"},
};

#define UART2DM_BASE_PHYS         0xA3200000
#define UART_DM_IRDA		(ct_uart2dm_base + 0x0038)

static int msm7x30_init_uart2dm_b(void)
{
	void __iomem *ct_uart2dm_base = 0;
	int ret = 0;

	ret = msm_gpios_request_enable(uart2dm_b_config_data,
			ARRAY_SIZE(uart2dm_b_config_data));

	if(ret)
	{		
		printk(KERN_INFO "%s: msm_gpios_request_enable uart2dm_b_config_data failed!!!\n",
			__func__);
		return ret;
	}
	else
		printk(KERN_INFO "%s: uart2dm_b_config_data successfully!!!\n",
			__func__);

   	ct_uart2dm_base = ioremap(UART2DM_BASE_PHYS, PAGE_SIZE);
	if (!ct_uart2dm_base) {
		pr_err("%s: Could not remap %x\n", __func__, UART2DM_BASE_PHYS);
		return -ENOMEM;
	}

       // Enable the IRDA transceiver
       // IrDA-modulated signal polarity at UART_RX pin inverted
	iowrite32(0x03, UART_DM_IRDA);
	iounmap(ct_uart2dm_base);
	
	printk(KERN_INFO "%s: Enable uart2dm irda successfully!!!\n",
		__func__);

	return ret;
}

#define IRDA_VLED_EN 30
#define IRDA_PWDN      33
#define IRDA_TXD         87

static struct msm_gpio irda_config_data[] = {
	{ GPIO_CFG(30, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), "IRDA_VLED_EN"},
	{ GPIO_CFG(33, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "IRDA_PWDN"},
};

static struct msm_gpio irda_txd_config_data[] = {
       { GPIO_CFG(87, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "IRDA_TXD"},
};

static int msm7x30_init_irda(void)
{
	int rc;
	
	msm_gpios_request_enable(irda_config_data,
			ARRAY_SIZE(irda_config_data));

	rc = msm_gpios_request_enable(irda_txd_config_data,
			ARRAY_SIZE(irda_txd_config_data));

	if(rc)
		printk(KERN_INFO "%s: msm7x30_init_irda failed!!!\n",
			__func__);

	return rc;
}

static void msm7x30_irda_set_mode(void)
{
	gpio_set_value(IRDA_TXD, 0);
	
	gpio_set_value(IRDA_PWDN, 1);
	printk(KERN_INFO "%s: IRDA VLED 3V ON\n",
		__func__);
	
	udelay(20);
	gpio_set_value(IRDA_PWDN, 0);
	udelay(200);
	
	msm_gpios_disable_free(irda_txd_config_data, 
		ARRAY_SIZE(irda_txd_config_data));

	gpio_set_value(IRDA_VLED_EN, 0);
	gpio_set_value(IRDA_PWDN, 1);
}
#endif
/*  wanggang add for ponyo irda 20110403 end */

/* TSIF begin */
#if defined(CONFIG_TSIF) || defined(CONFIG_TSIF_MODULE)

#define TSIF_B_SYNC      GPIO_CFG(37, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA)
#define TSIF_B_DATA      GPIO_CFG(36, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA)
#define TSIF_B_EN        GPIO_CFG(35, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA)
#define TSIF_B_CLK       GPIO_CFG(34, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA)

static const struct msm_gpio tsif_gpios[] = {
	{ .gpio_cfg = TSIF_B_CLK,  .label =  "tsif_clk", },
	{ .gpio_cfg = TSIF_B_EN,   .label =  "tsif_en", },
	{ .gpio_cfg = TSIF_B_DATA, .label =  "tsif_data", },
	{ .gpio_cfg = TSIF_B_SYNC, .label =  "tsif_sync", },
};

static struct msm_tsif_platform_data tsif_platform_data = {
	.num_gpios = ARRAY_SIZE(tsif_gpios),
	.gpios = tsif_gpios,
        .tsif_clk = "tsif_clk",
	.tsif_pclk = "tsif_pclk",
	.tsif_ref_clk = "tsif_ref_clk",
};
#endif /* defined(CONFIG_TSIF) || defined(CONFIG_TSIF_MODULE) */
/* TSIF end   */

static void __init pmic8058_leds_init(void)
{
#ifdef CONFIG_LEDS_PMIC8058
	pm8058_7x30_data.sub_devices[PM8058_SUBDEV_LED].platform_data
		= &pm8058_ffa_leds_data;
	pm8058_7x30_data.sub_devices[PM8058_SUBDEV_LED].data_size
		= sizeof(pm8058_ffa_leds_data);
#endif
}

static struct msm_spm_platform_data msm_spm_data __initdata = {
	.reg_base_addr = MSM_SAW_BASE,

	.reg_init_values[MSM_SPM_REG_SAW_CFG] = 0x05,
	.reg_init_values[MSM_SPM_REG_SAW_SPM_CTL] = 0x18,
	.reg_init_values[MSM_SPM_REG_SAW_SPM_SLP_TMR_DLY] = 0x00006666,
	.reg_init_values[MSM_SPM_REG_SAW_SPM_WAKE_TMR_DLY] = 0xFF000666,

	.reg_init_values[MSM_SPM_REG_SAW_SLP_CLK_EN] = 0x01,
	.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_PRECLMP_EN] = 0x03,
	.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_POSTCLMP_EN] = 0x00,

	.reg_init_values[MSM_SPM_REG_SAW_SLP_CLMP_EN] = 0x01,
	.reg_init_values[MSM_SPM_REG_SAW_SLP_RST_EN] = 0x00,
	.reg_init_values[MSM_SPM_REG_SAW_SPM_MPM_CFG] = 0x00,

	.awake_vlevel = 0xF2,
	.retention_vlevel = 0xE0,
	.collapse_vlevel = 0x72,
	.retention_mid_vlevel = 0xE0,
	.collapse_mid_vlevel = 0xE0,

	.vctl_timeout_us = 50,
};

//[simt-zhanghui-110808]{
//CTP power manage function
static int ctp_power(int on)
{
	//static int num = 0;
	static struct vreg *vreg_12, *vreg_16, *vreg_lvs0;
 
	pr_info("ctp_power(): on = %d\n",on);	 
	
	vreg_12 = vreg_get(NULL, "gp9");
	vreg_16 = vreg_get(NULL, "gp10");
	vreg_lvs0 = vreg_get(NULL, "lvsw0");
    	
	if (vreg_12 && vreg_16 && vreg_lvs0) {
		if (on == CYTTSP_POWER_ON || on == ATMEL_POWER_ON) {
			pr_info("TP power on (TP=%d)\n",on);      
			if(on == CYTTSP_POWER_ON){     
				gpio_tlmm_config(GPIO_CFG(GPIO_CTP_IRQ,  0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
				gpio_set_value(GPIO_CTP_IRQ, 0);    
				msleep(3);
				gpio_tlmm_config(GPIO_CFG(GPIO_CTP_IRQ,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
			}
			gpio_tlmm_config(GPIO_CFG(GPIO_CTP_IRQ,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
			gpio_tlmm_config(GPIO_CFG(GPIO_I2C_SDA, 0, GPIO_CFG_INPUT,	GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
			gpio_tlmm_config(GPIO_CFG(GPIO_I2C_SCL, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
			
			vreg_set_level(vreg_16, 2800);
			vreg_enable(vreg_16);
			vreg_set_level(vreg_lvs0, 2800);
			vreg_enable(vreg_lvs0);
			msleep(1);

			vreg_set_level(vreg_12, 1800);
			vreg_enable(vreg_12);
			//num++;

		}
		else if(on == ATMEL_POWER_OFF /*&& num > 2*/){
			pr_info("TP power off (atmel)\n");    
		
			gpio_tlmm_config(GPIO_CFG(GPIO_I2C_SDA, 0, GPIO_CFG_INPUT,	GPIO_CFG_PULL_UP, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
			gpio_tlmm_config(GPIO_CFG(GPIO_I2C_SCL, 0, GPIO_CFG_INPUT,	GPIO_CFG_PULL_UP, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
			gpio_tlmm_config(GPIO_CFG(GPIO_CTP_IRQ,  0, GPIO_CFG_INPUT,     GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
			
		}
		else if(on == CYTTSP_POWER_OFF /*&& num > 2*/){
			pr_info("TP power off (cyttsp)\n");     

			gpio_tlmm_config(GPIO_CFG(GPIO_I2C_SDA, 0, GPIO_CFG_INPUT,	GPIO_CFG_PULL_UP, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
			gpio_tlmm_config(GPIO_CFG(GPIO_I2C_SCL, 0, GPIO_CFG_INPUT,	GPIO_CFG_PULL_UP, GPIO_CFG_16MA), GPIO_CFG_ENABLE);
			gpio_tlmm_config(GPIO_CFG(GPIO_CTP_IRQ,  0, GPIO_CFG_INPUT,     GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		}

	}
	else{
		printk("\n\n---------------[ %s ]-----------------\n\n ", __FUNCTION__);
	}
	return 0;
}
//[simt-zhanghui-110808]}

void msm_wakeup_mb86a29(void)
{
	printk("******%s\n",__func__);

	if (gpio_request(39, "39_ctrl") < 0)
		printk ("gpio 39 request failed,%s\n", __func__);
	if (gpio_request(88, "88_ctrl") < 0)
		printk ("gpio 88 request failed,%s\n", __func__);

	gpio_direction_output(88, 1);
	udelay(100);
	gpio_direction_output(39, 1);
	udelay(100);
	vreg_enable(ljvreg_wlan2);
	udelay(100);
}

EXPORT_SYMBOL(msm_wakeup_mb86a29);

void msm_waitmode_mb86a29(void)
{

	printk("******%s\n",__func__);
	vreg_disable(ljvreg_wlan2);
	udelay(100);

	gpio_direction_output(39, 0);
	udelay(100);

	gpio_direction_output(88, 0);
	udelay(100);

	gpio_free(39);	
	gpio_free(88);	
}

EXPORT_SYMBOL(msm_waitmode_mb86a29);

void msm_close_mb86a29(void)
{
	if((NULL == ljvreg_gp13) || (NULL == ljvreg_gp5) || (NULL == ljvreg_wlan2))
		return;

	vreg_disable(ljvreg_wlan2);
//      vreg_put(ljvreg_wlan2);
	udelay(100);

	vreg_disable(ljvreg_gp5);
//      vreg_put(ljvreg_gp5);
	udelay(100);

	vreg_disable(ljvreg_gp13);
//      vreg_put(ljvreg_gp13);
	udelay(100);
}

EXPORT_SYMBOL(msm_close_mb86a29);

int msm_config_mb86a29(void)
{

	int rc;
	printk("====================msm_config_mb86a29====================\n");

	//reset gpioi39
	gpio_tlmm_config(GPIO_CFG(39,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_2MA),GPIO_CFG_ENABLE);
        gpio_tlmm_config(GPIO_CFG(88,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
	                        GPIO_CFG_2MA),GPIO_CFG_ENABLE);
        //for interrupt
	gpio_tlmm_config(GPIO_CFG(113,  1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_2MA),GPIO_CFG_ENABLE);

	if (gpio_request(39, "39_ctrl") < 0)
		printk ("gpio 39 request failed,%s\n", __func__);
	if (gpio_request(88, "88_ctrl") < 0)
		printk ("gpio 88 request failed,%s\n", __func__);

	//VDDE AVD, VREG_L20_1P8
	ljvreg_gp13 = vreg_get(NULL, "gp13");
	if (IS_ERR(ljvreg_gp13)) {
		printk("%s: gp13 vreg get failed (%ld)",
				__func__, PTR_ERR(ljvreg_gp13));
		return 0;
	}

	rc = vreg_set_level(ljvreg_gp13, 1800);
	printk("gp13 rc: %d\n", rc);
	if (rc) {
		printk("%s: vreg_set_level failed (%d)",
				__func__, rc);
	}

	//(void) vreg_enable(vreg_gp13);

	//VDDI, VREG_L23_1V2

	ljvreg_gp5 = vreg_get(NULL, "gp5");
	if (IS_ERR(ljvreg_gp5)) {
		printk("%s: gp5 vreg get failed (%ld)",
				__func__, PTR_ERR(ljvreg_gp5));
		return 0;
	}

	rc = vreg_set_level(ljvreg_gp5, 1200);
	printk("gp5 rc: %d\n", rc);
	if (rc) {
		printk("%s: vreg_set_level failed (%d)",
				__func__, rc);
	}

	//AVDE, VREG_L19_2P8

	ljvreg_wlan2 = vreg_get(NULL, "wlan2");
	if (IS_ERR(ljvreg_wlan2)) {
		printk("%s: vreg_wlan2 vreg get failed (%ld)",
				__func__, PTR_ERR(ljvreg_wlan2));
		return 0;
	}

	rc = vreg_set_level(ljvreg_wlan2, 2800);
	printk("wlan2 rc: %d\n", rc);
	if (rc) {
		printk("%s: vreg_set_level failed (%d)",
				__func__, rc);
	}

	//now
	rc = vreg_disable(ljvreg_gp13);
	rc = vreg_disable(ljvreg_gp5);
	gpio_direction_output(88, 0);
	udelay(100);

	gpio_direction_output(39, 0);
	udelay(100);
	rc =  vreg_disable(ljvreg_wlan2);

	mdelay(100);

	rc = vreg_enable(ljvreg_gp13);
	udelay(100);
	rc = vreg_enable(ljvreg_gp5);
	udelay(100);
	gpio_direction_output(88, 1);
	udelay(100);
	gpio_direction_output(39, 1);
	udelay(100);
	rc =  vreg_enable(ljvreg_wlan2);
	udelay(100);

	return 0;

}

EXPORT_SYMBOL(msm_config_mb86a29);

//[simt-zhanghui-110830]{
struct atmel_i2c_platform_data supersonic_atmel_ts_data[] = {
      {
              .version = 0x21,
		.abs_x_min = 0,
		.abs_x_max = 480,
		.abs_y_min = 0,
		.abs_y_max = 800,
		.abs_pressure_min = 0,
		.abs_pressure_max = 255,
		.abs_width_min = 0,
		.abs_width_max = 15, 
		.gpio_irq = GPIO_CTP_IRQ,
		.power = ctp_power,
//		.config_T6 = {reset,	backupnv,	calibrate,	reportall,	reserved,	diagnostic},		//comms
		.config_T6 = {0,		0,			0,			0,			0,			0}, 

//		.config_T7 = {idleacqint,	actvacqint,		actv2idleto},		//power

		.config_T7 = {32,			14,				50},


//		.config_T8 = {chrgtime,		reserved,	tchdrift,	driftst,	tchautocal,		reserved,		atchcalst,	
		.config_T8 = {10,			0,			5,			10,			0,				0,				5, 
//					  atchcalsthr,				atchfrccalthr,			atchfrccalratio},	//acquire
					  35,						10,						192},

//		.config_T9 = {ctrl,		xorigin,	yorigin,	xsize,		ysize,		akscfg,		blen,	tchthr,		tchdi,
		.config_T9 = {131,		0,			2,			14,			9,			0,			16,		40,			3, 
//					  orientate,	mrgtimeout,		movhysti,	movhystn,	movfilter,	numtouch,	mrghyst,	mrgthr,
					  3,			0,				0,			0,			30,			2,			0,			0, 
//					  tchamphyst,	xres(L,H),			yres(L,H),			xloclip,	xhiclip,	yloclip,	yhiclip,	
					  0,			0x1f, 0x3,		0xdf, 0x1,				10,			10,			18,			18,			
//					  xedgectrl,	xedgedist,		yedgectrl,	yedgedist,	jumplimit,	tchhyst},  //touchscreen
				  218,			60,				212,			105,			0,			10}, 

//		.config_T15 = {ctrl,	xorigin,	xsize,		yorigin,	ysize,		akscfg,		blen,		tchthr,		tchdi,
		.config_T15 = {0,		0,			0,			0,			0,			0,			0,			0,			0, 
//					   reserved[0],		reserved[1]},	//keyarray
					   0,				0}, 
//		.config_T18 = {ctrl,	command}
		.config_T18 = {0,	0},
//		.config_T19 = {ctrl,	reportmask,		dir,	pullup,		out,	wake,	pwm,	per,	duty[0],	duty[1],
		.config_T19 = {0,		0,				0,		0,			0,		0,		0,		0,		0,			0,	
//					   duty[2],		duty[3],	trigger[0],		trigger[1],		trigger[2],		trigger[3]}, //gpiopwm
					   0,			0,			0,				0,				0,				0}, 

//		.config_T20 = {ctrl,	xlogrip,	xhigrip,	ylogrip,	yhigrip,	maxtchs,	RESERVED2,	szthr1,		szthr2,
		.config_T20 = {0,		0,			0,			0,			0,			0,			0,			0,			0, 
//					   shpthr1,		shpthr2,		supextto}, //gripfacesuppression
					   0,			0,				0}, 


//		.config_T22 = {ctrl,	reserved0,		reserved1,		gcaful0,	gcaful1,	gcafll0,	gcafll1,	actvgcafvalid,
		.config_T22 = {5,		0,				0,				0,			0,			0,			0,			0, 
//					   noisethr,	reserved2,	freqhopscale,	freq[0],	freq[1],	freq[2],	freq[3],	freq[4],	
					   30,			0,			0,				18,			21,			29,			255,			255, 

//					   idlegcafvalid},			//noise_suppression
					   0}, 
		
//		.config_T23 = {ctrl,	xorigin,	yorigin,	xsize,		ysize,		reserved,		blen,		fxddthr0,	fxddthr0,	
		.config_T23 = {0,		0,			0,			0,			0,			0,				0,			0,			0,
//					   fxddi,	average,	mvnullrate,		mvdthr},	//proximity
					   0,		0,			0,				0},

//		.config_T24 = {ctrl,	numgest,	gesten0,	gesten1,	pressproc,		tapto,		flickto,	dragto,		spressto,
		.config_T24 = {1,		0,			3,			0,			0,				0,			0,			0,			0, 
//					   lpressto,		rptpressto,		flickthr0,	flickthr1,		dragthr0,	dragthr1,	tapthr0,	tapthr1,
					   0,			0,				0,			0,				0,			0,			0,			0, 
//					   throwthr0,		throwthr1}, //onetouch_gesture
					   0,				0}, 


//		.config_T25 = {ctrl,	cmd,	siglim[0].upsiglim(L,H),		siglim[0].upsiglim,		siglim[1].upsiglim,		siglim[1].upsiglim,
		.config_T25 = {3,		0,		0x1c,0x25,						0x7c,0x15,				0,0,					0,0, 
//					   siglim[2].upsiglim,		siglim[2].upsiglim},	//selftest
					   0,0,						0,0},

//		.config_T27 = {ctrl,	numgest,	reserved,	gesten,		rotatethr,		zoomthr}, //twotouch
		.config_T27 = {0,		0,			0,			0,			0,				0,0},

//		.config_T28 = {ctrl,	cmd,	mode,	idlegcafdepth,		actvgcafdepth,	voltage},	//cteconfig
		.config_T28 = {0,		0,		2,		8,					8,				10},

//      crc
		.object_crc = {0xff, 0x83, 0xde},

//		
		.cal_tchthr = 50, 
		.tchthr = 40,

	},
};
//[simt-zhanghui-110830]}

//[simt-zhanghui-110830]{
static struct i2c_board_info CTP_i2c_devices[] = {
	{
		I2C_BOARD_INFO(ATMEL_CTP_NAME, 0x4A),
		.platform_data = supersonic_atmel_ts_data,
		.irq = MSM_GPIO_TO_INT(GPIO_CTP_IRQ),

	},
};
//[simt-zhanghui-110830]{

static struct msm_gpio headset_key_data[] = {
	{GPIO_CFG(67, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), "headset_key"},
};

// [SIMT-qiukejun-110727]{
/*
*    This function is used to read gpio status for detecting version for ponyo
*/
int detect_version(int *gpio_value_array)
{
	int *temp_p=NULL;

	temp_p = gpio_value_array;

	temp_p[0] = gpio_get_value(41);
	printk("[%s]gpio_level[0] = %d\n ",__func__,temp_p[0]);

	temp_p[1] = gpio_get_value(43);
	printk("[%s]gpio_level[1] = %d\n ",__func__,temp_p[1]);
	
	printk("[%s] end \n",__func__);

	return 0;
}
// [SIMT-qiukejun-110727]}

//[SIMT-liyueyi-20110808] keep PMIC L8 power supply{
int set_L8_to_2P2(void)
{
   struct vreg *vreg_adc = NULL;
   int rc;
   vreg_adc = vreg_get(NULL, "gp12");
    if (IS_ERR(vreg_adc)) {
    	printk("%s: gp12 vreg get failed (%ld)",
    			__func__, PTR_ERR(vreg_adc));
    	return 0;
    }

    rc = vreg_set_level(vreg_adc, 2200);
    printk("gp12 rc: %d\n", rc);
    if (rc) {
    	printk("%s: vreg_set_level failed (%d)",
    			__func__, rc);
    }
   vreg_enable(vreg_adc);
   return 1;
}
//[SIMT-liyueyi-20110808] keep PMIC L8 power supply}

static void __init msm7x30_init(void)
{
	int rc;
	unsigned smem_size;
	uint32_t soc_version = 0;
       //chenping add for ponyo usb bug 20110711 start
       #ifdef  CONFIG_PONYO_DOCOMO_OVP_CONTROL
        struct msm_gpio usb_ovp_enable[] = { 
	{GPIO_CFG(49, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), "usb_ovp_enable"},
        };
       #endif
	//chenping add for ponyo usb bug 20110711 end
	if (socinfo_init() < 0)
		printk(KERN_ERR "%s: socinfo_init() failed!\n",
		       __func__);

	soc_version = socinfo_get_version();

	msm_clock_init(msm_clocks_7x30, msm_num_clocks_7x30);

	msm_config_mb86a29();

    set_L8_to_2P2(); //[SIMT-liyueyi-20110808] keep PMIC L8 power supply

#ifdef CONFIG_SERIAL_MSM_CONSOLE
	msm7x30_init_uart2();
#endif
//chenping add for ponyo usb bug 20110711 start
#ifdef  CONFIG_PONYO_DOCOMO_OVP_CONTROL
    rc = msm_gpios_request_enable(
        usb_ovp_enable,
        ARRAY_SIZE(usb_ovp_enable));
    if(rc)
          pr_err("gpio_49_enable failed\n"); 
    else
    {
	rc = gpio_direction_output(49, 0);
        if(rc)
	     pr_err("gpio_49_pull_down failed\n"); 
	msm_gpios_free( usb_ovp_enable,
        ARRAY_SIZE(usb_ovp_enable));
    }
#endif
//chenping add for ponyo usb bug 20110711 end
// wanggang add for ponyo irda 20110403 start
#ifdef CONFIG_USE_IRDA
	msm7x30_init_irda();
	msm7x30_irda_set_mode();

    msm7x30_init_uart2dm_b();
#endif
// wanggang add for ponyo irda 20110403 end
	msm_spm_init(&msm_spm_data, 1);
	msm_acpu_clock_init(&msm7x30_clock_data);
#ifdef CONFIG_USB_FUNCTION
	msm_hsusb_pdata.swfi_latency =
		msm_pm_data
		[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency;
	msm_device_hsusb_peripheral.dev.platform_data = &msm_hsusb_pdata;
#endif

#ifdef CONFIG_USB_MSM_OTG_72K
	if (SOCINFO_VERSION_MAJOR(soc_version) >= 2 &&
			SOCINFO_VERSION_MINOR(soc_version) >= 1) {
		pr_debug("%s: SOC Version:2.(1 or more)\n", __func__);
		msm_otg_pdata.ldo_set_voltage = 0;
	}

	msm_device_otg.dev.platform_data = &msm_otg_pdata;
#ifdef CONFIG_USB_GADGET
	msm_otg_pdata.swfi_latency =
 	msm_pm_data
 	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency;
	msm_device_gadget_peripheral.dev.platform_data = &msm_gadget_pdata;
#endif
#endif
	msm_uart_dm1_pdata.wakeup_irq = gpio_to_irq(136);
	msm_device_uart_dm1.dev.platform_data = &msm_uart_dm1_pdata;
// wanggang add for ponyo irda 20110403 start
#ifdef CONFIG_USE_IRDA
    msm_uart_dm2_pdata.wakeup_irq = gpio_to_irq(85);
	msm_device_uart_dm2.dev.platform_data = &msm_uart_dm2_pdata;
#endif
// wanggang add for ponyo irda 20110403 end
#if defined(CONFIG_TSIF) || defined(CONFIG_TSIF_MODULE)
	msm_device_tsif.dev.platform_data = &tsif_platform_data;
#endif
	msm_adc_pdata.dev_names = msm_adc_surf_device_names;
	msm_adc_pdata.num_adc = ARRAY_SIZE(msm_adc_surf_device_names);

    bl_config_gpios();
	wlan_config_gpios();

    msm_gpios_request_enable(
        headset_key_data, 
        ARRAY_SIZE(headset_key_data));
    msm_gpios_free(
        headset_key_data, 
        ARRAY_SIZE(headset_key_data));
        
	platform_add_devices(devices, ARRAY_SIZE(devices));
#ifdef CONFIG_USB_EHCI_MSM_72K
	msm_add_host(0, &msm_usb_host_pdata);
#endif
	msm7x30_init_mmc();
	msm7x30_init_nand();
	msm_qsd_spi_init();

#ifdef CONFIG_SPI_QSD
	spi_register_board_info(lcdc_toshiba_spi_board_info,
		ARRAY_SIZE(lcdc_toshiba_spi_board_info));
#endif

	msm_fb_add_devices();
	msm_pm_set_platform_data(msm_pm_data, ARRAY_SIZE(msm_pm_data));
	msm_device_i2c_init();
	i2c_register_board_info(10, CTP_i2c_devices, ARRAY_SIZE(CTP_i2c_devices));
	//[SIMT-caoxiangteng-110715]{
#ifdef CONFIG_CM3623_GPIO_I2C
	i2c_register_board_info(11, cm3623_i2c_devices, ARRAY_SIZE(cm3623_i2c_devices));
#endif
	//[SIMT-caoxiangteng-110715]}

	msm_device_i2c_2_init();
	qup_device_i2c_init();
	buses_init();
	msm7x30_init_marimba();
#ifdef CONFIG_MSM7KV2_AUDIO
	snddev_poweramp_gpio_init();
	aux_pcm_gpio_init();
#endif

	i2c_register_board_info(0, msm_i2c_board_info,
			ARRAY_SIZE(msm_i2c_board_info));

	marimba_pdata.tsadc = &marimba_tsadc_pdata;

	i2c_register_board_info(2, msm_marimba_board_info,
			ARRAY_SIZE(msm_marimba_board_info));

	i2c_register_board_info(2, msm_i2c_gsbi7_timpani_info,
			ARRAY_SIZE(msm_i2c_gsbi7_timpani_info));

	i2c_register_board_info(4 /* QUP ID */, msm_camera_boardinfo,
				ARRAY_SIZE(msm_camera_boardinfo));

	bt_power_init();
#ifdef CONFIG_I2C_SSBI
	msm_device_ssbi6.dev.platform_data = &msm_i2c_ssbi6_pdata;
	msm_device_ssbi7.dev.platform_data = &msm_i2c_ssbi7_pdata;
#endif

	pmic8058_leds_init();
#ifdef CONFIG_LEDS_GPIO
      platform_device_register(&gpio_leds);
#endif
	boot_reason = *(unsigned int *)
		(smem_get_entry(SMEM_POWER_ON_STATUS_INFO, &smem_size));
	printk(KERN_NOTICE "Boot Reason = 0x%02x\n", boot_reason);
}

static unsigned pmem_sf_size = MSM_PMEM_SF_SIZE;
static int __init pmem_sf_size_setup(char *p)
{
	pmem_sf_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_sf_size", pmem_sf_size_setup);

static unsigned fb_size = MSM_FB_SIZE;
static int __init fb_size_setup(char *p)
{
	fb_size = memparse(p, NULL);
	return 0;
}
early_param("fb_size", fb_size_setup);

static unsigned pmem_adsp_size = MSM_PMEM_ADSP_SIZE;
static int __init pmem_adsp_size_setup(char *p)
{
	pmem_adsp_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_adsp_size", pmem_adsp_size_setup);

static unsigned pmem_audio_size = MSM_PMEM_AUDIO_SIZE;
static int __init pmem_audio_size_setup(char *p)
{
	pmem_audio_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_audio_size", pmem_audio_size_setup);

static unsigned pmem_kernel_ebi1_size = PMEM_KERNEL_EBI1_SIZE;
static int __init pmem_kernel_ebi1_size_setup(char *p)
{
	pmem_kernel_ebi1_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_kernel_ebi1_size", pmem_kernel_ebi1_size_setup);

static void __init msm7x30_allocate_memory_regions(void)
{
	void *addr;
	unsigned long size;
/*
   Request allocation of Hardware accessible PMEM regions
   at the beginning to make sure they are allocated in EBI-0.
   This will allow 7x30 with two mem banks enter the second
   mem bank into Self-Refresh State during Idle Power Collapse.

    The current HW accessible PMEM regions are
    1. Frame Buffer.
       LCDC HW can access msm_fb_resources during Idle-PC.

    2. Audio
       LPA HW can access android_pmem_audio_pdata during Idle-PC.
*/

	size = fb_size ? : MSM_FB_SIZE;
	addr = alloc_bootmem(size);
	msm_fb_resources[0].start = __pa(addr);
	msm_fb_resources[0].end = msm_fb_resources[0].start + size - 1;
	pr_info("allocating %lu bytes at %p (%lx physical) for fb\n",
		size, addr, __pa(addr));

	size = pmem_audio_size;
	if (size) {
		addr = alloc_bootmem(size);
		android_pmem_audio_pdata.start = __pa(addr);
		android_pmem_audio_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for audio "
			"pmem arena\n", size, addr, __pa(addr));
	}

	size = pmem_kernel_ebi1_size;
	if (size) {
		addr = alloc_bootmem_aligned(size, 0x100000);
		android_pmem_kernel_ebi1_pdata.start = __pa(addr);
		android_pmem_kernel_ebi1_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for kernel"
			" ebi1 pmem arena\n", size, addr, __pa(addr));
	}

	size = pmem_sf_size;
	if (size) {
		addr = alloc_bootmem(size);
		android_pmem_pdata.start = __pa(addr);
		android_pmem_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for sf "
			"pmem arena\n", size, addr, __pa(addr));
	}

	size = pmem_adsp_size;
	if (size) {
		addr = alloc_bootmem(size);
		android_pmem_adsp_pdata.start = __pa(addr);
		android_pmem_adsp_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for adsp "
			"pmem arena\n", size, addr, __pa(addr));
	}
}

static void __init msm7x30_map_io(void)
{
	msm_shared_ram_phys = 0x00100000;
	msm_map_msm7x30_io();
	msm7x30_allocate_memory_regions();
}

MACHINE_START(PANASONIC_P01D, "P-01D")
	.boot_params = PHYS_OFFSET + 0x100,
	.map_io = msm7x30_map_io,
	.init_irq = msm7x30_init_irq,
	.init_machine = msm7x30_init,
	.timer = &msm_timer,
MACHINE_END
