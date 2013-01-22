/*
 *  max17040_battery.c
 *  fuel-gauge systems for lithium-ion (Li+) batteries
 *
 *  Copyright (C) 2009 Samsung Electronics
 *  Minkyu Kang <mk7.kang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
 
#include <linux/fcntl.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/power_supply.h>
#include <linux/max17040_battery.h>
#include <linux/slab.h>
#ifndef SIMCUST_MAX17040
#define SIMCUST_MAX17040
#endif
#define MAX17040_VCELL_MSB	0x02
#define MAX17040_VCELL_LSB	0x03
#define MAX17040_SOC_MSB	0x04
#define MAX17040_SOC_LSB	0x05
#define MAX17040_MODE_MSB	0x06
#define MAX17040_MODE_LSB	0x07
#define MAX17040_VER_MSB	0x08
#define MAX17040_VER_LSB	0x09
#define MAX17040_RCOMP_MSB	0x0C
#define MAX17040_RCOMP_LSB	0x0D
#define MAX17040_CMD_MSB	0xFE
#define MAX17040_CMD_LSB	0xFF

#define MAX17040_DELAY		HZ*15
#define MAX17040_BATTERY_FULL	95


struct max17040_chip {
	struct i2c_client		*client;
	struct delayed_work		work;
	struct power_supply		battery;
	struct max17040_platform_data	*pdata;

	/* State Of Connect */
	int online;
	/* battery voltage */
	int vcell;
	/* battery capacity */
	int soc;
	/* State Of Charge */
	int status;
#ifdef SIMCUST_MAX17040
    u8 rcomp_m;
    u8 rcomp_l;
    u8 ocv_m;
    u8 ocv_l;
#endif       
};

static int max17040_write_reg(struct i2c_client *client, int reg, u8 value)
{
	int ret;

	ret = i2c_smbus_write_byte_data(client, reg, value);

	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return ret;
}

static int max17040_read_reg(struct i2c_client *client, int reg)
{
	int ret;

	ret = i2c_smbus_read_byte_data(client, reg);

	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return ret;
}

static void max17040_reset(struct i2c_client *client)
{
	max17040_write_reg(client, MAX17040_CMD_MSB, 0x54);
	max17040_write_reg(client, MAX17040_CMD_LSB, 0x00);
}

static int max17040_get_vcell(struct i2c_client *client)
{
	struct max17040_chip *chip = i2c_get_clientdata(client);
	u8 msb;
	u8 lsb;

	msb = max17040_read_reg(client, MAX17040_VCELL_MSB);
	lsb = max17040_read_reg(client, MAX17040_VCELL_LSB);

	chip->vcell = ((msb << 4) + (lsb >> 4))*125/100;
       return chip->vcell;
}

static int max17040_get_soc(struct i2c_client *client)
{
	struct max17040_chip *chip = i2c_get_clientdata(client);
	u8 msb;
	u8 lsb;

	msb = max17040_read_reg(client, MAX17040_SOC_MSB);
	lsb = max17040_read_reg(client, MAX17040_SOC_LSB);
    
	chip->soc = (int)((msb * 256) + lsb); //(int)(((msb * 256) + lsb) * 0.001953125
	printk("max17040_get_soc <%d> \n",chip->soc);
       return chip->soc;
}

static void max17040_get_version(struct i2c_client *client)
{
	u8 msb;
	u8 lsb;

	msb = max17040_read_reg(client, MAX17040_VER_MSB);
	lsb = max17040_read_reg(client, MAX17040_VER_LSB);

	dev_info(&client->dev, "MAX17040 Fuel-Gauge Ver %d%d\n", msb, lsb);
}

#ifdef SIMCUST_MAX17040
/*   SIMCUST_MAX17040   */
/*-------------------------------------------------------------------------
The custom model that is stored in the MAX17040 is written to RAM and so it must be 
written to the device any time that power is applied or restored to the device.

        Device=MAX17040
        Title = 592_1_052511
        EmptyAdjustment = 0
        FullAdjustment= 100
        RCOMP0=66
        TempCoUp =-0.925
        TempCoDown = -5.875
        OCVTest = 55776
        SOCCheckA = 223
        SOCCheckB = 225
        bits= 19
-------------------------------------------------------------------------*/
#define OCVTest 55776
#define SOCCheckA 223
#define SOCCheckB 225
#define MAX17040_LOCK_MSB     0x3E
#define MAX17040_LOCK_LSB      0x3F
#define MAX17040_OCV_MSB     0x0E
#define MAX17040_OCV_LSB      0x0F
#define MAX17040_RCOMP_MSB     0x0C
#define MAX17040_RCOMP_LSB      0x0D
#define MODEL_1 0x40
#define MODEL_2 0x50
#define MODEL_3 0x60
#define MODEL_4 0x70

static u8 cust_model_1[] = {
MODEL_1/*reg_addr*/,
0x9D,0x20,0xB7,0xA0,0xB8,0xF0,0xBA,0x40,0xBB,0x90,0xBB,0xF0,0xBC,0x60,0xBD,0x10,
};
static u8 cust_model_2[] = {
MODEL_2/*reg_addr*/,
0xBD,0xC0,0xBE,0x90,0xC1,0x80,0xC4,0x70,0xC7,0x80,0xC9,0xA0,0xCB,0xC0,0xCF,0xE0,
};
static u8 cust_model_3[] = {
MODEL_3/*reg_addr*/,
0x01,0xC0,0x2C,0x00,0x2C,0x00,0x00,0xE0,0x49,0x00,0x56,0x00,0x58,0x00,0x56,0x00,
};
static u8 cust_model_4[] = {
MODEL_4/*reg_addr*/,
0x47,0x80,0x1C,0x80,0x1A,0x00,0x1B,0x00,0x19,0xE0,0x19,0xE0,0x09,0xE0,0x09,0xE0,
};

static int max17040_i2c_rx_data(struct i2c_client *client,char *buf, int len)
{
	uint8_t i;
	struct i2c_msg msgs[] = {
		{
			.addr	= client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= buf,
		},
		{
			.addr	= client->addr,
			.flags	= I2C_M_RD,
			.len	= len,
			.buf	= buf,
		}
	};

	for (i = 0; i < 3; i++) {
		if (i2c_transfer(client->adapter, msgs, 2) >= 0) {
			break;
		}
		mdelay(10);
	}

	if (i >= 3) {
		pr_err("%s: retry over %d\n", __FUNCTION__, 3);
		return -EIO;
	}

	return 0;
}
static int max17040_i2c_tx_data(struct i2c_client *client,char *buf, int len)
{
	uint8_t i;
	struct i2c_msg msg[] = {
		{
			.addr	= client->addr,
			.flags	= 0,
			.len	= len,
			.buf	= buf,
		}
	};
	
	for (i = 0; i < 3; i++) {
		if (i2c_transfer(client->adapter, msg, 1) >= 0) {
			break;
		}
		mdelay(10);
	}

	if (i >= 3) {
		pr_err("%s: retry over %d\n", __FUNCTION__, 3);
		return -EIO;
	}
	return 0;
}
/*   M A X 1 7 0 4 0 _ U N L O C K _ M O D E L _ A C C E S S   */
/*-------------------------------------------------------------------------
    To unlock access to the model the host software must write 0x4Ah to memory location 
    0x3Eh and write 0x57h to memory location 0x3Fh.
-------------------------------------------------------------------------*/
static void max17040_unlock_model_access(struct i2c_client *client)
{
    u8 buf_reg[] = {MAX17040_LOCK_MSB,0x4A,0x57};
    
    max17040_i2c_tx_data(client, buf_reg, 3);
}
static void max17040_lock_model_access(struct i2c_client *client)
{
    u8 buf_reg[] = {MAX17040_LOCK_MSB,0x00,0x00};
    
    max17040_i2c_tx_data(client, buf_reg,3);
}

/*   M A X 1 7 0 4 0 _ R E A D _ R C O M P _ O C V   */
/*-------------------------------------------------------------------------
    The RCOMP and OCV Registers will be modified during the process of loading the custom
    model. Read and store these values so that they can be written back to the device after 
    the model has been loaded.
-------------------------------------------------------------------------*/
static void max17040_read_rcomp_ocv(struct i2c_client *client)
{
    u8 rcomp_ocv[4];
    struct max17040_chip *chip = i2c_get_clientdata(client);

    rcomp_ocv[0] = MAX17040_RCOMP_MSB;
    max17040_i2c_rx_data(client, rcomp_ocv, 4);
        
    chip->rcomp_m = rcomp_ocv[0];
    chip->rcomp_l = rcomp_ocv[1];
    chip->ocv_m = rcomp_ocv[2];
    chip->ocv_l = rcomp_ocv[3];
}


/*   M A X 1 7 0 4 0 _ W R I T E _ O C V   */
/*-------------------------------------------------------------------------
    Write OCV Register
-------------------------------------------------------------------------*/
static void max17040_write_ocv(struct i2c_client *client)
{
    u8 buf_reg[3];
    buf_reg[0] = MAX17040_OCV_MSB;
    buf_reg[1] = ((OCVTest>>8)&0x00FF);
    buf_reg[2] = ((OCVTest)&0x00FF);
    max17040_i2c_tx_data(client, buf_reg,3);
}


/*   M A X 1 7 0 4 0 _ W R I T E _ R C O M P _ T O _ M A X   */
/*-------------------------------------------------------------------------
    Write RCOMP to a Maximum value of 0xFF00h
-------------------------------------------------------------------------*/
static void max17040_write_rcomp_to_max(struct i2c_client *client)
{
    u8 buf_reg[] = {MAX17040_OCV_MSB,0xFF,0x00};

	max17040_i2c_tx_data(client, buf_reg,3);
}


/*   M A X 1 7 0 4 0 _ W R I T E _ M O D E L   */
/*-------------------------------------------------------------------------
    Once the model is unlocked, the host software must write the 64 byte model to the 
    MAX17040/1/3. The model is located between memory locations 0x40h and 0x7Fh.
-------------------------------------------------------------------------*/
static void max17040_write_model(struct i2c_client *client)
{
    max17040_i2c_tx_data(client, cust_model_1,17);
    max17040_i2c_tx_data(client, cust_model_2,17);
    max17040_i2c_tx_data(client, cust_model_3,17);
    max17040_i2c_tx_data(client, cust_model_4,17);
}
/*   M A X 1 7 0 4 0 _ S O C _ I S _ A V A L I A B L E   */
/*-------------------------------------------------------------------------
    There will be some variation in the SOC register due to the ongoing activity of the
    Model Gauge algorithm.Therefore, the upper byte of the RCOMP register is verified 
    to be within a specified range to verify that the model was loaded correctly.
-------------------------------------------------------------------------*/
static bool max17040_soc_is_avaliable(struct i2c_client *client)
{

    u8 soc[2];

    //Read SOC Register
    soc[0] = MAX17040_SOC_MSB;
    max17040_i2c_rx_data(client, soc, 2);

    printk(" max17040_soc_is_avaliable soc:%d\n",soc[0]);
    //Compare value
    if(( soc[0] >= SOCCheckA )&&( soc[0] <= SOCCheckB))
        return 1;
    else
        return 0;
}

/*   M A X 1 7 0 4 0 _ R E S T O R E _ R C O M P _ O C V   */
/*-------------------------------------------------------------------------
    
-------------------------------------------------------------------------*/
static void max17040_restore_rcomp_ocv(struct i2c_client *client)
{
     struct max17040_chip *chip = i2c_get_clientdata(client);
     u8 buf_reg[5];
     
     buf_reg[0] = MAX17040_RCOMP_MSB;
     buf_reg[1] = 0x66;//chip->rcomp_m;
     buf_reg[2] = 0x00;//chip->rcomp_l;
     buf_reg[3] = chip->ocv_m;
     buf_reg[4] = chip->ocv_l;
     max17040_i2c_tx_data(client, buf_reg,5);
}


/*   M A X 1 7 0 4 0 _ C U S T _ U P D A T A _ M O D E L   */
/*-------------------------------------------------------------------------
    
-------------------------------------------------------------------------*/
static void max17040_cust_updata_model(struct i2c_client *client)
{
    max17040_unlock_model_access(client);
    msleep(100);
    max17040_read_rcomp_ocv(client);
    
    max17040_write_ocv(client);
    max17040_write_rcomp_to_max(client);
    
    msleep(150);
    
    max17040_write_model(client);
    msleep(150);
    max17040_write_ocv(client);
    msleep(300);
    
    max17040_soc_is_avaliable(client);

    max17040_restore_rcomp_ocv(client);        
    
    max17040_lock_model_access(client);
}

#endif

#if 0
static void max17040_get_online(struct i2c_client *client)
{
	struct max17040_chip *chip = i2c_get_clientdata(client);

	if (chip->pdata->battery_online)
		chip->online = chip->pdata->battery_online();
	else
		chip->online = 1;
}

static void max17040_get_status(struct i2c_client *client)
{
	struct max17040_chip *chip = i2c_get_clientdata(client);

	if (!chip->pdata->charger_online || !chip->pdata->charger_enable) {
		chip->status = POWER_SUPPLY_STATUS_UNKNOWN;
		return;
	}

	if (chip->pdata->charger_online()) {
		if (chip->pdata->charger_enable())
			chip->status = POWER_SUPPLY_STATUS_CHARGING;
		else
			chip->status = POWER_SUPPLY_STATUS_NOT_CHARGING;
	} else {
		chip->status = POWER_SUPPLY_STATUS_DISCHARGING;
	}

	if (chip->soc > MAX17040_BATTERY_FULL)
		chip->status = POWER_SUPPLY_STATUS_FULL;
}
#endif /* NEVER */

static int max17040_get_property(struct power_supply *psy,
			    enum power_supply_property psp,
			    union power_supply_propval *val)
{
	struct max17040_chip *chip = container_of(psy,
				struct max17040_chip, battery);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = chip->status;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = chip->online;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = chip->vcell;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = chip->soc;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}
/*********************************************************/
/*added by SIMT-liyueyi-20110916                                                */
/*read already stored file for juging whether to reset chip.               */
/*********************************************************/
int boot_complete = 0;
struct i2c_client * max17040_client;
#define GAUGE_SOC_PATH      "/data/system/battery/gauge.soc"
#define GAUGE_VCELL_PATH      "/data/system/battery/gauge.vcell"
#define  MAX17040_FILE_OPTION_SUCCESS       0
#define  MAX17040_ABS(x)        ((x>=0)?x:(0-x))
#define MAX17040_SOC_ERROR      7680 //capacity 15%
#define MAX17040_VCELL_ERROR    200 //mv

static int max17040_read_file(struct file *file, int *raw_data)
{
	mm_segment_t   old_fs;  
	loff_t pos = 0;
	char buff[5] = {0,};
	if (!file->f_op || !file->f_op->read) {
		printk("max17040 :this file can't be read\n");
		return -1;
	}
       pos =  0;
	memset(buff, 0, sizeof(buff));
	old_fs = get_fs();   
	set_fs(KERNEL_DS);
	file->f_op->read(file, buff, sizeof(buff), &pos);   
	sscanf(buff, "%d", raw_data);
	printk("max17040 :raw_data is %d\n", *raw_data);
	filp_close(file, NULL);
	set_fs(old_fs);  
	//printk("this fun is %s\n", __func__);
	return 0;
}
static int max17040_write_file(struct file *file , int raw_data)
{
	mm_segment_t   old_fs;  
	loff_t pos ;
	char buff[10] = {0,};

	if (!file->f_op || !file->f_op->write ) {
		printk("max17040 :this file can't be write\n");
		return -1;
	}
	sprintf(buff, "%d", raw_data);
	old_fs = get_fs();   
	set_fs(KERNEL_DS);
	pos = 0;
	file->f_op->write(file, buff, sizeof(buff), &pos);   
	printk("max17040 :the buff is %s\n", buff);
	filp_close(file, NULL);
	set_fs(old_fs); 
	//printk("this fun is %s\n", __func__);
	return 0;
}
static bool max17040_soc_comparator(int file_soc, int cur_soc)
{   
     int error = 0;
     error = file_soc - cur_soc;
     printk("max17040 : soc comparator file soc %d, cur_soc %d\n",file_soc,cur_soc);
     if(MAX17040_ABS(error) >= MAX17040_SOC_ERROR)
       return true;
     else 
       return false;
}
static bool max17040_vcell_comparator(int file_vcell, int cur_vcell)
{   
     int error = 0;
     error = file_vcell - cur_vcell;
     printk("max17040 : soc comparator file vcell %d, cur_vcell %d\n",file_vcell,cur_vcell);
     if(MAX17040_ABS(error) >= MAX17040_VCELL_ERROR)
       return true;
     else 
       return false;
}
void reset_max17040(void)
{
    struct file *filp = NULL;
    int temp_soc = 0;

    filp = filp_open(GAUGE_VCELL_PATH, O_RDONLY, 0777);
    if(IS_ERR(filp)) 
          printk("max17040 :error occured while opening file %s, exiting...\n",GAUGE_SOC_PATH);   
    else
    {
        if(max17040_read_file(filp, &temp_soc) == MAX17040_FILE_OPTION_SUCCESS)
        {
            if( max17040_vcell_comparator(temp_soc,max17040_get_vcell(max17040_client)) )
            {
                     max17040_reset(max17040_client);
                     msleep(200);
                     max17040_cust_updata_model(max17040_client);
                     printk("max17040 :reset max17040 and updata model \n");
                     boot_complete = 1;
                     return;
            }
        }
        else
            printk("\nmax17040 :%s read file fail!!!\n",__func__);
    }

    filp = filp_open(GAUGE_SOC_PATH, O_RDONLY, 0777);
    if(IS_ERR(filp)) 
          printk("max17040 :error occured while opening file %s, exiting...\n",GAUGE_SOC_PATH);   
    else
    {
        if(max17040_read_file(filp, &temp_soc) == MAX17040_FILE_OPTION_SUCCESS)
        {
            if( max17040_soc_comparator(temp_soc,max17040_get_soc(max17040_client)) )
            {
                     max17040_reset(max17040_client);
                     msleep(200);
                     max17040_cust_updata_model(max17040_client);
                     printk("max17040 :reset max17040 and updata model \n");
            }
        }
        else
            printk("\nmax17040 :%s read file fail!!!\n",__func__);
    }
     boot_complete = 1;         
}
static void max17040_work(struct work_struct *work)
{
	struct max17040_chip *chip;
       struct file *filp = NULL;

	chip = container_of(work, struct max17040_chip, work.work);

	max17040_get_vcell(chip->client);
	max17040_get_soc(chip->client);
	//max17040_get_online(chip->client);
	//max17040_get_status(chip->client);
      if(boot_complete)
      {
          filp = filp_open(GAUGE_SOC_PATH, O_RDWR|O_CREAT, 0777);
          if(IS_ERR(filp)) 
            printk("max17040 :error occured while opening file /data/simcom/gauge.soc, exiting...\n");   
          else
          {
               max17040_write_file(filp,chip->soc);
          }
          filp = filp_open(GAUGE_VCELL_PATH, O_RDWR|O_CREAT, 0777);
          if(IS_ERR(filp)) 
            printk("max17040 :error occured while opening file /data/simcom/gauge.soc, exiting...\n");   
          else
          {
               max17040_write_file(filp,chip->vcell);
          }
      }
      schedule_delayed_work(&chip->work, MAX17040_DELAY);
}

static enum power_supply_property max17040_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
};

static int __devinit max17040_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct max17040_chip *chip;
	int ret;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE))
		return -EIO;

	chip = kzalloc(sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->client = client;
	chip->pdata = client->dev.platform_data;

       i2c_set_clientdata(client, chip);

	chip->battery.name		= "gaugeIC";
	chip->battery.type		= POWER_SUPPLY_TYPE_BATTERY;
	chip->battery.get_property	= max17040_get_property;
	chip->battery.properties	= max17040_battery_props;
	chip->battery.num_properties	= ARRAY_SIZE(max17040_battery_props);

	ret = power_supply_register(&client->dev, &chip->battery);
	if (ret) {
		dev_err(&client->dev, "failed: power supply register\n");
		kfree(chip);
		return ret;
	}

       max17040_client = client;
       max17040_cust_updata_model(client);
	//max17040_reset(client);
	max17040_get_version(client);

      
  	INIT_DELAYED_WORK_DEFERRABLE(&chip->work, max17040_work);
	schedule_delayed_work(&chip->work,HZ);

	return 0;
}

static int __devexit max17040_remove(struct i2c_client *client)
{
	struct max17040_chip *chip = i2c_get_clientdata(client);

	power_supply_unregister(&chip->battery);
	cancel_delayed_work(&chip->work);
	kfree(chip);
	return 0;
}

#ifdef CONFIG_PM

static int max17040_suspend(struct i2c_client *client,
		pm_message_t state)
{
	struct max17040_chip *chip = i2c_get_clientdata(client);

	cancel_delayed_work(&chip->work);
	return 0;
}

static int max17040_resume(struct i2c_client *client)
{
	struct max17040_chip *chip = i2c_get_clientdata(client);

       max17040_get_vcell(chip->client);
	max17040_get_soc(chip->client);
	schedule_delayed_work(&chip->work, MAX17040_DELAY);
	return 0;
}

#else

#define max17040_suspend NULL
#define max17040_resume NULL

#endif /* CONFIG_PM */

static const struct i2c_device_id max17040_id[] = {
	{ "max17040", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, max17040_id);

static struct i2c_driver max17040_i2c_driver = {
	.driver	= {
		.name	= "max17040",
	},
	.probe		= max17040_probe,
	.remove		= __devexit_p(max17040_remove),
	.suspend	= max17040_suspend,
	.resume		= max17040_resume,
	.id_table	= max17040_id,
};

static int __init max17040_init(void)
{
	return i2c_add_driver(&max17040_i2c_driver);
}
module_init(max17040_init);

static void __exit max17040_exit(void)
{
	i2c_del_driver(&max17040_i2c_driver);
}
module_exit(max17040_exit);

MODULE_AUTHOR("Minkyu Kang <mk7.kang@samsung.com>");
MODULE_DESCRIPTION("MAX17040 Fuel Gauge");
MODULE_LICENSE("GPL");
