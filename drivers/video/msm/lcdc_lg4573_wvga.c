/* Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
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
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"

#include <linux/delay.h>
#include <linux/pwm.h>
#ifdef CONFIG_PMIC8058_PWM
#include <linux/mfd/pmic8058.h>
#include <linux/pmic8058-pwm.h>
#endif
#include <linux/charge_pump.h>
#include <mach/gpio.h>
#include "msm_fb.h"
#include "msm_fb_panel.h"
//[SIMT-lilening-20110804] add lcd vcom valibration {
#ifdef CONFIG_FB_MSM_LCDC_LG4573_VCOM_CALIBRATION
#include <linux/syscalls.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#endif
//[SIMT-lilening-20110804] add lcd vcom valibration}
//[SIMT-lilening-20110804] add lcd auto detect {
#ifdef CONFIG_FB_MSM_TRY_MDDI_CATCH_LCDC_PRISM
#include "mddihosti.h"
#endif
//[SIMT-lilening-20110804] add lcd auto detect }
static int lcdc_lg4573_panel_off(struct platform_device *pdev);

//[SIMT-lilening-20110804] change BL_MAX(16 ->255)
#ifdef CONFIG_BIGBANG_BACKLIGHT
#define BL_MAX		16
#else
#define BL_MAX		255
#endif
#ifdef CONFIG_PMIC8058_PWM
static struct pwm_device *bl_pwm;

//[SIMT-lilening-20110830] backlight step 64->512 {
#define PWM_PERIOD	1024	/* us, period of 1Khz */
//[SIMT-lilening-20110830] backlight step 64->512 }
#define DUTY_LEVEL	PWM_PERIOD / BL_MAX
#endif


//[SIMT-lilening-20110804] add lcd vcom valibration {
#ifdef CONFIG_FB_MSM_LCDC_LG4573_VCOM_CALIBRATION

#define VCOM_PATH		"persist/lcdvcom"
#define VCOM_REG		0xC5
#define VCOM_DEFAULT	0x77

static int lcdc_lg4573_get_vcom(unsigned int *pvcom);
static int lcdc_lg4573_set_vcom(unsigned int *pvcom);
static int lcdc_lg4573_init_vcom(unsigned int *pvcom);
static unsigned char vcom8 = VCOM_DEFAULT;
#endif
//[SIMT-lilening-20110804] add lcd vcom valibration }
static int spi_cs;
static int spi_sclk;
static int spi_mosi;
static int spi_miso;

struct lg4573_state_type {
	boolean disp_initialized;
	boolean display_on;
	boolean disp_powered_up;
};

struct lg4573_spi_data {
	u16 data;
	u16 sleep;
};
//[SIMT-lilening-20110804] change lcd init data{
static struct lg4573_spi_data lg4573_init_sequence[] = {
    
	{0x00c0, 0},
	{0x0101, 0},
	{0x0118, 0},//18

	{0x0020, 0},//exit_invert_mode
	{0x0029, 0},//set_display_on
	{0x003A, 0},//set_pixel_format 
	{0x0160, 0},//70   0X60 26k

	{0x00B1, 0},//RGB Interface Setting
	//{0x0106, 0},
	{0x010E, 0},
	{0x011E, 0},
	{0x010C, 0},

	{0x00B2, 0},//Panel Characteristics Setting
	{0x0110, 0},//480 pixels
	{0x01C8, 0},//800 pixels

	{0x00B3, 0},//Panel Drive Setting    Set the inversion mode
	{0x0100, 0},//1-dot inversion 0x01

	{0x00B4, 0},//Display Mode Control
	{0x0104, 0},//Dither disable.

	{0x00B5, 0},//Display Mode and Frame Memory Write Mode Setting
	{0x0110, 0},
	{0x0130, 0},
	{0x0130, 0},
	{0x0100, 0},
	{0x0100, 0},

	{0x00B6, 0},//Display Control 2 ( GIP Specific )
	{0x0101, 0},
	{0x0118, 0},
	{0x0102, 0},
	{0x0140, 0},
	{0x0110, 0},
	{0x0140, 0},


	{0x00C3, 0}, 
	{0x0103, 0},
	{0x0103, 0},
	{0x0103, 0},
	{0x0105, 0},
	{0x0104, 0},

	//Sleep(40, 0},        // 20100413 deleted by LGE

	{0x00C4, 0},//VDD Regulator setting
	{0x0112, 0},
	{0x0123, 0},//GDC AP
	{0x0114, 0},//VRH1  Vreg1out=1.533xVCI(10)
	{0x0114, 0},//VRH2  Vreg2out=-1.533xVCI(10)

	{0x0100, 0},

	{0x017a, 0},// 20100413 changed from 3F to 6D by LGE

	//Sleep(200, 0},       // 20100413 deleted by LGE
	//20110713 flicker test (0xc5:0x6e->0x72)
	{0x00C5, 0},
	{0x0177, 0},

	//Sleep(100, 0},       // 20100413 deleted by LGE

	{0x00C6, 0},
	{0x0125, 0},//VCI 23
	{0x0150, 0},//RESET RCO 53
	{0x0100, 0},//SBC GBC

	//Sleep(100, 0},       // 20100413 deleted by LGE

	//GAMMA SETTING
	{0x00D0, 0},
	{0x0100, 0},
	{0x0144, 0},
	{0x0144, 0},
	{0x0116, 0},
	{0x0115, 0},
	{0x0103, 0},
	{0x0161, 0},
	{0x0116, 0},
	{0x0133, 0},

	{0x00D1, 0},
	{0x0100, 0},
	{0x0144, 0},
	{0x0144, 0},
	{0x0116, 0},
	{0x0115, 0},
	{0x0103, 0},
	{0x0161, 0},
	{0x0116, 0},
	{0x0133, 0},

	{0x00D2, 0},
	{0x0100, 0},
	{0x0144, 0},
	{0x0144, 0},
	{0x0116, 0},
	{0x0115, 0},
	{0x0103, 0},
	{0x0161, 0},
	{0x0116, 0},
	{0x0133, 0},

	{0x00D3, 0},
	{0x0100, 0},
	{0x0144, 0},
	{0x0144, 0},
	{0x0116, 0},
	{0x0115, 0},
	{0x0103, 0},
	{0x0161, 0},
	{0x0116, 0},
	{0x0133, 0},

	{0x00D4, 0},
	{0x0100, 0},
	{0x0144, 0},
	{0x0144, 0},
	{0x0116, 0},
	{0x0115, 0},
	{0x0103, 0},
	{0x0161, 0},
	{0x0116, 0},
	{0x0133, 0},

	{0x00D5, 0},
	{0x0100, 0},
	{0x0144, 0},
	{0x0144, 0},
	{0x0116, 0},
	{0x0115, 0},
	{0x0103, 0},
	{0x0161, 0},
	{0x0116, 0},
	{0x0133, 0},
	//DISPLAY ON
	{0x0011, 200},
};
//[SIMT-lilening-20110804] change BL_MAX(16 ->255)
static struct lg4573_state_type lg4573_state = { 0 };
static struct msm_panel_common_pdata *lcdc_lg4573_pdata;

struct pm8058_gpio backlight_enable = {
	.direction      = PM_GPIO_DIR_OUT,
	.pull           = PM_GPIO_PULL_NO,
	.out_strength   = PM_GPIO_STRENGTH_HIGH,
	.function       = PM_GPIO_FUNC_NORMAL,
	.inv_int_pol    = 0,
	.vin_sel        = 2,
	.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
	.output_value   = 0,
};

/*===========================================================================

FUNCTION      SPI_DELAY

DESCRIPTION
  Delay for 33 us

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
static void spi_delay(unsigned long delay) 
{
	//delay /= 5;	
	udelay(delay);
}

static __inline void LCD_CS(unsigned char Lv) 
{
	gpio_set_value(spi_cs, Lv);
}

static __inline void LCD_SCL(unsigned char Lv) 
{
	gpio_set_value(spi_sclk, Lv);
}

static __inline unsigned long LCD_SO(void)
{
	unsigned long level;
	level = gpio_get_value(spi_miso);

	return level;
}

static __inline void LCD_SI(unsigned char Lv) 
{
	gpio_set_value(spi_mosi, Lv);
}

static __inline void LCD_RST(unsigned char Lv) 
{
	//gpio_set_value(spi_rst, Lv);
}

static __inline void SendByte( unsigned char Byte)
{
    unsigned long   Shift;
    unsigned long   Cnt;

    Shift = 0x80;

    for (Cnt = 0; Cnt < 8; Cnt++)
    {
        LCD_SCL( 0);
        if (Byte&Shift)
        {
            LCD_SI( 1);
        }
        else
        {
            LCD_SI( 0);
        }
        Shift = Shift >> 1;
        spi_delay( 1);
        LCD_SCL( 1);
        spi_delay( 1);
    }
}

static void SendStartByte( unsigned char RW, unsigned char RS)
{
#define LCD_DEV_ID(v) (v<<2)
#define LCD_RS(v)     (v<<1)
#define LCD_RW(v)     (v<<0)

    unsigned char StartByte = 0x70;
    StartByte |= (LCD_DEV_ID(0) | LCD_RS(RS) | LCD_RW(RW)); 
    SendByte( StartByte);
    
}

static void SetIndex( unsigned char Index)
{
    LCD_CS( 0);
    spi_delay( 1 );
    SendStartByte( 0, 0);
    SendByte( Index);
    spi_delay( 1 );
    LCD_CS( 1);
}

static void SetData( unsigned char Data)
{
    LCD_CS( 0);
    spi_delay( 1);
    SendStartByte( 0, 1);
    SendByte( Data);
    spi_delay( 1);
    LCD_CS( 1);
}

static void LcdSend(unsigned short Value)
{
  	if (Value & 0xFF00) {
       SetData(Value&0xFF);
  	}
    else {
       SetIndex(Value&0xFF);
    }
}

static __inline void GetByte(unsigned char *pByte)
{
    unsigned long   Shift;
    unsigned long   Cnt;
    unsigned char    Byte = 0;
    
    Shift = 0x80;

    for (Cnt = 0; Cnt < 8; Cnt++)
    {
        LCD_SCL( 0);
        spi_delay( 1);
        if (LCD_SO())
        {
            Byte |= Shift;
        }
        LCD_SCL( 1 );
        spi_delay( 1);
        Shift = Shift >> 1;
    }
    
    *pByte = Byte;
}


static void GetData( unsigned char *pData)
{
    LCD_CS( 0);
    spi_delay( 1);

    SendStartByte(1, 1);
    GetByte(pData); //dumy byte
    GetByte( pData);

    spi_delay(1);
    LCD_CS(1);
}

static void lg4573_serigo(struct lg4573_spi_data *sequence, int num)
{
    int i;
    for (i = 0; i < num; i++) {
//[SIMT-lilening-20110804] add lcd vcom valibration {
#ifdef CONFIG_FB_MSM_LCDC_LG4573_VCOM_CALIBRATION
    if(sequence[i].data == VCOM_REG)
    {
	sequence[i+1].data =vcom8|0x0100;
    }		
#endif
//[SIMT-lilening-20110804] add lcd vcom valibration }
        LcdSend(sequence[i].data);
        if (sequence[i].sleep)
            mdelay(sequence[i].sleep);
    }
}

static void lg4573_spi_init(void)
{
	spi_sclk = *(lcdc_lg4573_pdata->gpio_num);
	spi_cs   = *(lcdc_lg4573_pdata->gpio_num + 1);
	spi_mosi = *(lcdc_lg4573_pdata->gpio_num + 2);
	spi_miso = *(lcdc_lg4573_pdata->gpio_num + 3);

	/* Set the Chip Select deasserted (active low) */
	gpio_set_value(spi_cs, 1);
	/* Set the output so that we don't disturb the slave device */
	gpio_set_value(spi_sclk, 0);
	gpio_set_value(spi_mosi, 1);

}

static void lg4573_disp_powerup(void)
{
	if (!lg4573_state.disp_powered_up && !lg4573_state.display_on)
		lg4573_state.disp_powered_up = TRUE;
}

static void lg4573_disp_on(void)
{
	unsigned char pid;
    
	printk("\n++++++lg4573_disp_on +++++++\n\n");

	if (lg4573_state.disp_powered_up && !lg4573_state.display_on) {
     
    	SetIndex(0xa1);//read id
    	GetData(&pid);
    	printk("lg4573_disp_on ID[%x]\n",pid);
        
		lg4573_serigo(lg4573_init_sequence, ARRAY_SIZE(lg4573_init_sequence));
    	printk("lg4573_serigo done\n");
		lg4573_state.display_on = TRUE;
	}
}

static int lcdc_lg4573_panel_on(struct platform_device *pdev)
{
	if (!lg4573_state.disp_initialized) {
        lcdc_lg4573_pdata->panel_config_gpio(1);
		lg4573_spi_init();
		lg4573_disp_powerup();
		lg4573_disp_on();
		lg4573_state.disp_initialized = TRUE;
	}
	return 0;
}
//[SIMT-lilening-20110812] change lcd sleep mode {
static int lcdc_lg4573_panel_off(struct platform_device *pdev)
{
	if (lg4573_state.disp_powered_up && lg4573_state.display_on) {
		lg4573_state.display_on = FALSE;
		SetIndex(0x10);
		mdelay(5);
		SetIndex(0xc1);
		SetData(0x01);
		mdelay(120);
		lg4573_state.disp_initialized = FALSE;
		lcdc_lg4573_pdata->panel_config_gpio(0);
	}
	return 0;
}
//[SIMT-lilening-20110812] change lcd sleep mode }
static void lcdc_lg4573_panel_set_backlight(struct msm_fb_data_type *mfd)
{
	static bool bl_on = false;
	bool panel_on = mfd->panel_power_on;
	int bl_level;

	bl_level = mfd->bl_level;

	//printk("bl_lv %d bl_on %d panel_on %d\n", bl_level, bl_on, panel_on);
#ifdef CONFIG_PMIC8058_PWM
	if (bl_pwm) {
		pwm_config(bl_pwm, bl_level * DUTY_LEVEL, PWM_PERIOD);
		pwm_enable(bl_pwm);
	}
#else

#endif
#ifdef CONFIG_BIGBANG_BACKLIGHT
	if(bl_level >=0 && bl_level <= BL_MAX){
		bl_level *= 6;
	}else if(bl_level < 0){
		bl_level = 1;
	}else if(bl_level > BL_MAX){
		bl_level = 100;
	}
#endif
	if (bl_level && panel_on) {
		if (!bl_on) {
			msleep(200);
#ifdef CONFIG_BACKLIGHT_CHARGE_PUMP
			ChargePumpPowerOn();
#endif
			bl_on = true;
		}
#ifdef CONFIG_BACKLIGHT_CHARGE_PUMP
		ChargePumpSetDispLightLv(bl_level);
		// ChargePumpTest();
#endif
	} else {
#ifdef CONFIG_BACKLIGHT_CHARGE_PUMP
		ChargePumpPowerOff();
#endif
		bl_on = false;
	}
}
//[SIMT-lilening-20110804] add lcd vcom valibration {
#ifdef CONFIG_FB_MSM_LCDC_LG4573_VCOM_CALIBRATION
static int lcdc_lg4573_init_vcom(unsigned int *pvcom)
{
	int fd;
	int rc = 0;
	unsigned int vcom;
	mm_segment_t old_fs;
	char *filename = VCOM_PATH;

	old_fs = get_fs();
	printk("lcdc_lg4573_init_vcom\n");

	set_fs (KERNEL_DS);
	
	fd = sys_open(filename, O_RDONLY , 0);
	if (fd < 0) {
		printk("Can not open %s fd = 0x%x\n", filename, fd);
		rc = -ENOENT;
		goto error1;
	}
	
	sys_read(fd, (unsigned char *)&vcom, sizeof(vcom));
	printk("lcdc_lg4573_init_vcom = 0x%x\n",vcom);
	vcom8 = vcom & 0xFF;
	
	if(vcom8 > 0x7F || vcom8< 0x30)
	{
		vcom8 = VCOM_DEFAULT;
		printk("lcdc_lg4573_init_vcom failed 0x%x\n", (unsigned int)sys_unlink(filename));
		rc = -1;
	}
	else
	{
		SetIndex(VCOM_REG);
		SetData(vcom8);
		*pvcom = vcom8;
	}

	sys_close(fd);
error1:
	set_fs (old_fs);
	return rc;
}

static int lcdc_lg4573_set_vcom(unsigned int *pvcom)
{
	printk("lcdc_lg4573_set_vcom %d\n",*pvcom);
   	vcom8 = *pvcom;
	if(vcom8 > 0x7F || vcom8< 0x30)
	{
		vcom8 = VCOM_DEFAULT;
		printk("lcdc_lg4573_set_vcom invalid parameter\n ");
		return -1;
	}
	else
	{
		SetIndex(VCOM_REG);
		SetData(vcom8);
	}
	return 0;
}

static int lcdc_lg4573_get_vcom(unsigned int *pvcom)
{
	SetIndex(VCOM_REG);
	GetData(&vcom8);
	*pvcom = vcom8;
	printk("lcdc_lg4573_get_vcom 0x%x\n",*pvcom);
	return 0;
}
#endif
//[SIMT-lilening-20110804] add lcd vcom valibration }
static int __devinit lg4573_probe(struct platform_device *pdev)
{     
    struct platform_device *msm_fb_dev;
	struct msm_fb_data_type *mfd;
	struct fb_info *fbi;

	printk("zys+++++++++++lg4573_probe++++++zys\n\n");
	
	if (pdev->id == 0) {
		lcdc_lg4573_pdata = pdev->dev.platform_data;
		return 0;
	}

#ifdef CONFIG_PMIC8058_PWM
	bl_pwm = pwm_request(lcdc_lg4573_pdata->gpio, "backlight");
	if (bl_pwm == NULL || IS_ERR(bl_pwm)) {
		pr_err("%s pwm_request() failed\n", __func__);
		bl_pwm = NULL;
	}

	printk(KERN_INFO "lg4573_probe: bl_pwm=%x LPG_chan=%d\n",
			(int) bl_pwm, (int)lcdc_lg4573_pdata->gpio);

    if (bl_pwm) {
    	pwm_config(bl_pwm, 128 * DUTY_LEVEL, PWM_PERIOD);
    	pwm_enable(bl_pwm);
    }    
#endif
#ifdef CONFIG_BACKLIGHT_CHARGE_PUMP
    ChargePumpPowerOn();
#endif
#ifdef CONFIG_BACKLIGHT_CHARGE_PUMP
    ChargePumpSetDispLightLv(50);
#endif
	msm_fb_dev = msm_fb_add_device(pdev);
    if (msm_fb_dev) {
        mfd = platform_get_drvdata(msm_fb_dev);
        fbi = mfd->fbi;
        fbi->fbops->fb_open(fbi, 0);
    }

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = lg4573_probe,
	.driver = {
		.name   = "lcdc_lg4573_wvga",
	},
};

static struct msm_fb_panel_data lg4573_panel_data = {
	.on = lcdc_lg4573_panel_on,
	.off = lcdc_lg4573_panel_off,
	.set_backlight = lcdc_lg4573_panel_set_backlight,
//[SIMT-lilening-20110804] add lcd vcom valibration {
#ifdef CONFIG_FB_MSM_LCDC_LG4573_VCOM_CALIBRATION
	.set_vcom = lcdc_lg4573_set_vcom,
	.get_vcom = lcdc_lg4573_get_vcom,
	.init_vcom = lcdc_lg4573_init_vcom,
#endif
//[SIMT-lilening-20110804] add lcd vcom valibration }
};

static struct platform_device this_device = {
	.name   = "lcdc_lg4573_wvga",
	.id	= 1,
	.dev	= {
		.platform_data = &lg4573_panel_data,
	}
};

static int __init lcdc_lg4573_panel_init(void)
{
	int ret;
	struct msm_panel_info *pinfo;
//[SIMT-lilening-20110804] add lcd auto detect {
#ifdef CONFIG_FB_MSM_TRY_MDDI_CATCH_LCDC_PRISM
	if (mddi_get_client_id() != 0)
		return 0;

	ret = msm_fb_detect_client("lcdc_lg4573_wvga");
	if (ret)
		return 0;

#endif
//[SIMT-lilening-20110804] add lcd auto detect}

	printk("\n______________lcdc_lg4573_panel_init__________\n\n");
	ret = platform_driver_register(&this_driver);
	if (ret)
		return ret;

	pinfo = &lg4573_panel_data.panel_info;
	pinfo->xres = 480;
	pinfo->yres = 800;
	MSM_FB_SINGLE_MODE_PANEL(pinfo);
	pinfo->type = LCDC_PANEL;
	pinfo->pdest = DISPLAY_1;
	pinfo->wait_cycle = 0;
	pinfo->bpp = 18;
	pinfo->fb_num = 2;
	pinfo->clk_rate = 24500000;
	pinfo->bl_max = BL_MAX;
	pinfo->bl_min = 1;

	pinfo->lcdc.h_back_porch = 27;
	pinfo->lcdc.h_front_porch = 8;
	pinfo->lcdc.h_pulse_width = 1;
	pinfo->lcdc.v_back_porch = 9;
	pinfo->lcdc.v_front_porch = 4;
	pinfo->lcdc.v_pulse_width = 1;
	pinfo->lcdc.border_clr = 0;     /* blk */
	pinfo->lcdc.underflow_clr = 0;  /* blk */
	pinfo->lcdc.hsync_skew = 0;

	ret = platform_device_register(&this_device);
	if (ret) {
		printk(KERN_ERR "%s not able to register the device\n",
			__func__);
		goto fail_driver;
	}
    
	return ret;
    
fail_driver:
		platform_driver_unregister(&this_driver);

	return ret;
}
module_init(lcdc_lg4573_panel_init);

static void __exit lcdc_lg4573_panel_exit(void)
{
}
module_exit(lcdc_lg4573_panel_exit);

