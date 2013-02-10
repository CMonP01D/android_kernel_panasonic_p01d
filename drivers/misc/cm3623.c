/*
 *     Copyright 2011 Smart Wireless Corporatoin, All rights reserved.
 *
 *     Filename: cm3623.c
 *
 *     Description:
 *
 *     ------------------------------------------------------------------------------
 *     Revision History:
 *     Date            Author Name         Brief Description
 *     2011-07-19      caoxiangteng        the driver of ps & als sensors              
 *     ------------------------------------------------------------------------------
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/cm3623.h> 
#include <mach/gpio.h>
#include <asm/thread_info.h>
#include <asm/uaccess.h>

#include <asm/uaccess.h>
#include <linux/slab.h>

#define CM3623_ARA                      (0x18>>1)
#define CM3623_INT                      (0x19>>1)
#define CM3623_ALS_WR                   (0x20>>1)
#define CM3623_ALS_RD_MSB               (0x21>>1)
#define CM3623_INIT                     (0x22>>1)
#define CM3623_ALS_RD_LSB               (0x23>>1)
#define CM3623_PS_WR                    (0xB0>>1)
#define CM3623_PS_RD                    (0xB1>>1)
#define CM3623_PS_THD_WR                (0xB2>>1)
#define CM3623_VENDORID                 (0x0001)
#define CM3623_ALS                      (0)
#define CM3623_PS                       (1)
#define CM3623_ALS_BIT                  (1<<CM3623_ALS)
#define CM3623_PS_BIT                   (1<<CM3623_PS)
#define CM3623_UP 			(1)
#define CM3623_DOWN 			(0)
#define CM3623_RETRY_COUNT		(3)

#define CM3623_FILE_DATA_20       	"/data/simcom/cm3623_ps/ps_20_nv.file"
#define CM3623_FILE_DATA_30      	"/data/simcom/cm3623_ps/ps_30_nv.file"
#define CM3623_DATA_0			(0)
#define CM3623_DATA_20			(1)
#define CM3623_DATA_30			(2)


MODULE_LICENSE("GPL v2");
MODULE_ALIAS("cm3623");

#define dim(x) (sizeof(x)/sizeof((x)[0]))

struct cm3623_sensor_data {
	int (*power)(int on);
	struct i2c_client *client;
	int gpio_int;
	bool use_irq;
	struct hrtimer timer;
	struct work_struct  work_pull_als;
	struct work_struct  work_pull_ps;
	struct work_struct  work_push;	
	struct work_struct  work_cfg;	
	struct mutex lock;
	int light[2];
	int distance[2];
	int open_push;
	int ps_threshold;
	bool ps_wake;
	bool fake_data;
};

static int state[2] = {CM3623_DOWN, CM3623_DOWN}; /* state[0] is for als ,satate[1] is for ps, 0 is shutdown and 1 is up */

static ssize_t cm3623_als_read(struct file *, char __user *, size_t, loff_t *) ;
static ssize_t cm3623_ps_read(struct file *, char __user *, size_t, loff_t *) ;
static int cm3623_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg) ;
static void cm3623_report(struct cm3623_sensor_data *sd, int type) ;
static int cm3623_up_ps(struct cm3623_sensor_data *sd);


static struct workqueue_struct *cm3623_wq;

static struct file_operations cm3623_als_fops = {
	.owner = THIS_MODULE,
	.read = cm3623_als_read,
};

static struct miscdevice cm3623_als_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "cm3623_als",
	.fops = &cm3623_als_fops,
};

static struct file_operations cm3623_ps_fops = {
	.owner =   THIS_MODULE,
	.read = cm3623_ps_read,
	.ioctl = cm3623_ioctl,
};

static struct miscdevice cm3623_ps_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "cm3623_ps",
	.fops = &cm3623_ps_fops,
};

static enum hrtimer_restart cm3623_timer(struct hrtimer *timer)
{
	struct cm3623_sensor_data *data = container_of(timer, struct cm3623_sensor_data, timer);

	//printk("cm3623_timer\n");
	queue_work(cm3623_wq, &data->work_push);
	hrtimer_start(&data->timer, ktime_set(0, 200000000), HRTIMER_MODE_REL);

	return HRTIMER_NORESTART;
}

static irqreturn_t cm3623_irq(int irq, void *dev_id)
{
	struct cm3623_sensor_data *data = dev_id;

	//printk("cm3623_irq\n");
	disable_irq_nosync(data->client->irq);
	queue_work(cm3623_wq, &data->work_push);

	return IRQ_HANDLED;
}

static int cm3623_open_push(struct cm3623_sensor_data *data)
{
	int rc = 0;

	if (!data->open_push) {
		data->use_irq = false;
		if (data->client->irq) {
			rc = request_irq(data->client->irq,
					&cm3623_irq,
					IRQF_TRIGGER_LOW,
					CM3623_NAME,
					data);
			data->use_irq = rc ? false : true;
		}
		else {
			hrtimer_init(&data->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
			data->timer.function = cm3623_timer;
			rc = hrtimer_start(&data->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
		}
	}
	if (!rc) {
		data->open_push++;
	}
	return rc;
}

static void cm3623_release_push(struct cm3623_sensor_data *data)
{    
	if (data->open_push)
		data->open_push--;

	if (!data->open_push) {
		if (data->use_irq) {
			free_irq(data->client->irq, data);
			data->use_irq = false;
		}
		else {
			hrtimer_cancel(&data->timer);
		}
	}
}

static int write_sensor_calibration_data(struct file *file , int raw_data)
{
	mm_segment_t   old_fs;  
	loff_t pos ;
	char buff[4] = {0,};

	if (!file->f_op || !file->f_op->write ) {
		printk("this file can't be write\n");
		return -1;
	}
	sprintf(buff, "%d", raw_data);
	old_fs = get_fs();   
	set_fs(KERNEL_DS);
	pos = 0;
	file->f_op->write(file, buff, sizeof(buff), &pos);   
	printk("the buff is %s\n", buff);
	filp_close(file, NULL);
	set_fs(old_fs); 
	//printk("this fun is %s\n", __func__);

	return 0;
}

#if 0
static int read_sensor_calibration_data(struct file *file, int *raw_data)
{
	mm_segment_t   old_fs;  
	loff_t pos = 0;
	char buff[4] = {0,};
	if (!file->f_op || !file->f_op->read) {
		printk("this file can't be read\n");
		return -1;
	}

	memset(buff, 0, sizeof(buff));
	old_fs = get_fs();   
	set_fs(KERNEL_DS);
	file->f_op->read(file, buff, sizeof(buff), &pos);   
	sscanf(buff, "%d", raw_data);
	printk("raw_data is %d\n", *raw_data);
	filp_close(file, NULL);
	set_fs(old_fs);  
	//printk("this fun is %s\n", __func__);
	return 0;
}
#endif

static int cm3623_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	int data[4] = {0};
	struct i2c_client *client = container_of(cm3623_ps_device.parent, struct i2c_client, dev);
	struct cm3623_sensor_data *sd = i2c_get_clientdata(client);
	void __user *pa = (void __user *)arg;
	int rc;
	struct   file   *filep[2] = {NULL, NULL}; 

	if ((rc = cm3623_up_ps(sd)) < 0) {
		if (sd->power) {
			sd->power(0);
			return -1;
		}
	}
	usleep(1000000);

	switch(cmd)
	{
		case CM3623_CALI_0:  
			{
				//mutex_lock(&sd->lock);
				//cm3623_report(sd, CM3623_PS_BIT);
				//mutex_unlock(&sd->lock);
				flush_work(&sd->work_pull_ps);
				queue_work(cm3623_wq, &sd->work_pull_ps);
				flush_work(&sd->work_pull_ps);

				//printk("CM3623_CALI_0 IS ON");
				data[CM3623_DATA_0] = sd->distance[0];
				if (copy_to_user(pa,&data[CM3623_DATA_0],sizeof(data[CM3623_DATA_0])))
				{
					printk("copy_to_user is error!");
					return -EFAULT;
				}
				break;
			}	

		case CM3623_CALI_20:  
			{
				//mutex_lock(&sd->lock);
				//cm3623_report(sd, CM3623_PS_BIT);
				//mutex_unlock(&sd->lock);
				flush_work(&sd->work_pull_ps);
				queue_work(cm3623_wq, &sd->work_pull_ps);
				flush_work(&sd->work_pull_ps);

				//printk("CM3623_CALI_20 IS ON");
				data[CM3623_DATA_20] = sd->distance[0];
				filep[CM3623_DATA_20] = filp_open(CM3623_FILE_DATA_20, O_RDWR, 0777);   
				if(IS_ERR(filep[CM3623_DATA_20])) {
					printk("error occured while opening file %s, exiting...\n",  CM3623_FILE_DATA_20);   
					return -1;   
				}
				rc = write_sensor_calibration_data(filep[CM3623_DATA_20], data[CM3623_DATA_20]);
				if (rc) {
					printk("error while write sensor data\n");
					return -1;
				}
				//printk("data[CM3623_DATA_20] is %d\n", data[CM3623_DATA_20]);
				if (copy_to_user(pa,&data[CM3623_DATA_20],sizeof(data[CM3623_DATA_20])))
				{
					printk("copy_to_user is error!");
					return -EFAULT;
				}
				break;
			}	
		case CM3623_CALI_30:
			{
				//mutex_lock(&sd->lock);
				//cm3623_report(sd, CM3623_PS_BIT);
				//mutex_unlock(&sd->lock);
				flush_work(&sd->work_pull_ps);
				queue_work(cm3623_wq, &sd->work_pull_ps);
				flush_work(&sd->work_pull_ps);

				//printk("CM3623_CALI_30 IS ON");
				data[CM3623_DATA_30] = sd->distance[0];
				filep[CM3623_DATA_30] = filp_open(CM3623_FILE_DATA_30, O_RDWR, 0777);   
				if(IS_ERR(filep[CM3623_DATA_30])) {
					printk("error occured while opening file %s, exiting...\n",  CM3623_FILE_DATA_30);   
					return -1;
				}
				rc = write_sensor_calibration_data(filep[CM3623_DATA_30], data[CM3623_DATA_30]);
				if (rc) {
					printk("error while write sensor data\n");
					return -1;
				}
				if (copy_to_user(pa,&data[CM3623_DATA_30],sizeof(data[CM3623_DATA_30])))
				{
					printk("copy_to_user is error!");
					return -EFAULT;
				}
				//cali_state[CM3623_DATA_30] = CM3623_CALI_DATA;
				break;
			}
		default:
			break;
	}
	return 0;
}

#define cm3623_calc_light(code) ((int)(code * 5)) // update time is 100ms

const int n[] = {8, 20, 40, 80, 180};
const int k[] = {8125, 1250, 350, 125, 50}; // 26% Gray level
const int b[] = {102, 37, 22, 15, 9, 0}; // 26% Gray level

static int cm3623_calc_distance(int code) {
	int i, v;

	for (v = i = 0; i < dim(n); i++) {
		if (code <= n[i]) {
			v = (b[i] - (code * k[i] / 1000));
			break;
		}
	}
	return v;
}

static int cm3623_calc_code(int distance) {
	int i, v;

	v = 0;
	if (distance < b[0]) {        
		for (i = 0; i < (dim(b) - 1); i++) {
			if (distance > b[i + 1]) {
				v = (b[i] - distance) * 1000 / k[i];
				break;
			}
		}
	}
	return v;
}

static void cm3623_report(struct cm3623_sensor_data *sd, int type)
{
	u16 val;
	int rc;

	do {
		if (type & CM3623_ALS_BIT) {
			sd->client->addr = CM3623_ALS_RD_LSB;
			rc = i2c_smbus_read_byte(sd->client);
			if (rc < 0) 
				break;
			rc &= 0xFF;
			val = rc;
			sd->client->addr = CM3623_ALS_RD_MSB;
			rc = i2c_smbus_read_byte(sd->client);
			if (rc < 0) 
				break;
			rc &= 0xFF;
			val |= (rc << 8);

			sd->light[0] = val;
			sd->light[1] = cm3623_calc_light(val);
			//printk("the old data of light %d lux\n", sd->light[0]);
			//printk("the raw data of light %d lux\n", sd->light[1]);
		}

		if (type & CM3623_PS_BIT) {
			sd->client->addr = CM3623_PS_RD;
			rc = i2c_smbus_read_byte(sd->client);
			if (rc < 0) 
				break;
			sd->distance[0] = rc;
			sd->distance[1] = cm3623_calc_distance(rc);
			//printk("the old data of distance %d mm\n", sd->distance[0]);
			//printk("the raw data of ps code %d \n", sd->distance[0]);
		}
	} while(0);
}

static int cm3623_cfg_tigger(struct cm3623_sensor_data *sd) {
	int rc = 0;

	if (sd->ps_threshold) {
		sd->client->addr = CM3623_PS_THD_WR;
		rc = i2c_smbus_write_byte(sd->client, sd->ps_threshold);
		if (sd->gpio_int) {
			rc = gpio_get_value(sd->gpio_int);
			//printk("CM3623_INT_PIN %d %d\n", sd->gpio_int, rc);
			set_irq_type(sd->client->irq, rc ? IRQF_TRIGGER_LOW : IRQF_TRIGGER_HIGH);
		}
	}
	else {
		rc = -EINVAL;
	}

	return rc;
}

static void cm3623_work_pull_als(struct work_struct *work_pull_als)
{
	struct cm3623_sensor_data *sd =
		container_of(work_pull_als, struct cm3623_sensor_data, work_pull_als);

	//printk("cm3623_pull_als\n");
	mutex_lock(&sd->lock);
	cm3623_report(sd, CM3623_ALS_BIT);
	mutex_unlock(&sd->lock);    
}

static void cm3623_work_pull_ps(struct work_struct *work_pull_ps)
{
	struct cm3623_sensor_data *sd =
		container_of(work_pull_ps, struct cm3623_sensor_data, work_pull_ps);

	//printk("cm3623_pull_ps\n");
	mutex_lock(&sd->lock);
	cm3623_report(sd, CM3623_PS_BIT);
	mutex_unlock(&sd->lock);    
}

static void cm3623_work_push(struct work_struct *work_push)
{
	int rc;
	struct cm3623_sensor_data *sd =
		container_of(work_push, struct cm3623_sensor_data, work_push);

	mutex_lock(&sd->lock);
#if 0
	if (sd->gpio_int) {
		for (val = 0; val < 2; val++) {
			rc = gpio_get_value(sd->gpio_int);
			printk("CM3623_INT %d %d\n", sd->gpio_int, rc);
			if (rc) {
				break;
			}
			sd->client->addr = CM3623_ARA;
			rc = i2c_smbus_read_byte(sd->client);
			printk("CM3623_ARA 0x%X\n", rc);
		}
	}
#endif
	if (sd->ps_threshold)
	{
		cm3623_report(sd, CM3623_PS_BIT);
	}
	if (sd->use_irq) {
		if (sd->gpio_int) {
			rc = gpio_get_value(sd->gpio_int);
			//printk("CM3623_INT %d %d\n", sd->gpio_int, rc);
			set_irq_type(sd->client->irq, rc ? IRQF_TRIGGER_LOW : IRQF_TRIGGER_HIGH);
		}
		enable_irq(sd->client->irq);
	}

	mutex_unlock(&sd->lock);    
}

static void cm3623_work_cfg(struct work_struct *work_cfg)
{
	struct cm3623_sensor_data *sd =
		container_of(work_cfg, struct cm3623_sensor_data, work_cfg);

	mutex_lock(&sd->lock);
	cm3623_cfg_tigger(sd);
	mutex_unlock(&sd->lock);
}

static ssize_t cm3623_als_read(struct file *fp, char __user *buf, size_t count, loff_t *pos)
{
	ssize_t rc = 0;	
	struct i2c_client *client = 
		container_of(cm3623_als_device.parent, struct i2c_client, dev);
	struct cm3623_sensor_data *sd = i2c_get_clientdata(client);

	do {
		if (sd->fake_data) {
			sd->light[1] = -1; 
		}
		else {
			flush_work(&sd->work_pull_als);
			queue_work(cm3623_wq, &sd->work_pull_als);
			flush_work(&sd->work_pull_als);
		}
		if (copy_to_user(buf, &(sd->light[1]), sizeof(sd->light[1]))) {
			rc = -EFAULT;
			break;
		}
		rc = sizeof(sd->light[1]);
	} while(0);

	return rc;
}

static ssize_t cm3623_ps_read(struct file *fp, char __user *buf, size_t count, loff_t *pos)
{
	ssize_t rc = 0;	
	struct i2c_client *client = 
		container_of(cm3623_ps_device.parent, struct i2c_client, dev);
	struct cm3623_sensor_data *sd = i2c_get_clientdata(client);

	do {
		if (sd->fake_data) {
			sd->distance[0] = -1; 
		}
		else {
			flush_work(&sd->work_pull_ps);
			queue_work(cm3623_wq, &sd->work_pull_ps);
			flush_work(&sd->work_pull_ps);
		}
		if (copy_to_user(buf, &(sd->distance[0]), sizeof(sd->distance[0]))) {
			rc = -EFAULT;
			break;
		}
		rc = sizeof(sd->distance[0]);
	} while(0);

	return rc;
}

static int cm3623_init_addr(struct cm3623_sensor_data *sd)
{
	int rc;
	u8 val;

	sd->client->addr = CM3623_INIT;
	val = 0x20;
	rc = i2c_smbus_write_byte(sd->client, val);
    //rc = -1; // for test;
	printk("%s %d\n", __func__, rc);
	return rc;
}

static int cm3623_check_int(struct cm3623_sensor_data *sd)
{
    int rc = -1;
	printk("%s\n", __func__);
    if (sd->gpio_int) {
        rc = gpio_get_value(sd->gpio_int);
    	printk("%s INT lv %d\n", __func__, rc);
        rc = !rc; // low level is INT
    }

    return rc;
}

static int cm3623_clean_int(struct cm3623_sensor_data *sd)
{
	int rc, i, j;

	printk("%s\n", __func__);
    for (j = 0; j < 10; j++) {
    	for (i = 0; i < 2; i++) {
#if 0
    		sd->client->addr = CM3623_INT;
    		rc = i2c_smbus_read_byte(sd->client);
    		if (rc < 0) {
            	printk("%s clear CM3623_INT failed\n", __func__);
		    }
#endif
    		sd->client->addr = CM3623_ARA;
    		rc = i2c_smbus_read_byte(sd->client);
    		if (rc < 0) {
            	printk("%s clear CM3623_ARA failed\n", __func__);
		    }
    	}
        rc = cm3623_check_int(sd);
        if (rc <= 0) { 
            rc = 0;
            break;
        }
    }

    if (10 == j || rc < 0) {
    	printk("%s INT clean failed(%d)\n", __func__, rc);
        rc = -1;
    }
	return rc;
}

static int cm3623_disable_int(struct cm3623_sensor_data *sd)
{
	int rc;
	u8 val;

	sd->client->addr = CM3623_PS_WR;
	val = 0x00;
	rc = i2c_smbus_write_byte(sd->client, val);
	printk("%s %d\n", __func__, rc);
	return rc;
}


static int cm3623_try_check(struct cm3623_sensor_data *sd)
{
    int i, rc;
	printk("%s\n", __func__);
    for (i = 0; i < 10; i++) {
    	if ((rc = cm3623_init_addr(sd)) < 0) {
    		if ((rc = cm3623_clean_int(sd)) < 0) {
				break;
    		}
	    }
        else if ((rc = cm3623_disable_int(sd)) < 0) {
        	if ((rc = cm3623_clean_int(sd)) < 0) {
        		break;
        	}
        }
        else if ((rc = cm3623_disable_int(sd)) < 0) {
        	if ((rc = cm3623_clean_int(sd)) < 0) {
        		break;
        	}
        }
        else if ((rc = cm3623_check_int(sd)) > 0) {
        	if ((rc = cm3623_clean_int(sd)) < 0) {
        		break;
        	}
        }
        else {
            rc = 0;
            break;
        }
    }

	if (rc < 0 || 10 == i) {
        if (sd->power) {
		    sd->power(0);
        }
	}
    return rc;
}

static int cm3623_config(struct cm3623_sensor_data *sd)
{
	int rc; 
	u8 val;

	printk("cm3623_config\n");

	sd->client->addr = CM3623_ALS_WR;
	val = (1 << 0);   // SD_ALS
	val |= (1 << 1);  // WDM
	val |= (0 << 2);  // IT_ALS
	val |= (0 << 4);  // THD_ALS
	val |= (0 << 6);  // GAIN_ALS
	rc = i2c_smbus_write_byte(sd->client, val);

	sd->client->addr = CM3623_PS_THD_WR;
	val = sd->ps_threshold; // THD_PS
	rc = i2c_smbus_write_byte(sd->client, val);

	sd->client->addr = CM3623_PS_WR;
	val = (1 << 0);   // SD_PS
	val |= (0 << 2);  // INT_PS
	val |= (0 << 3);  // INT_ALS
	val |= (3 << 4);  // IT_PS    
	val |= (0 << 6);  // DR_PS    
	rc = i2c_smbus_write_byte(sd->client, val);

	return rc;
}

static int cm3623_up_ps(struct cm3623_sensor_data *sd)
{

	int i;

	//printk("%s\n", __func__);
	sd->client->addr = CM3623_PS_WR;
	for (i=0; i<CM3623_RETRY_COUNT; i++) {
		if (i2c_smbus_write_byte(sd->client, 0x30) >= 0) {
			break;
		}
	}
	mdelay(10);

	if (i >= CM3623_RETRY_COUNT) {
		printk(KERN_WARNING "cm3623 shutdown PS fail\n");
		return -1;
	}
	return 0;
}

static int cm3623_shutdown_ps(struct cm3623_sensor_data *sd)
{

	int i;

	sd->client->addr = CM3623_PS_WR;
	for (i=0; i<CM3623_RETRY_COUNT; i++) {
		if (i2c_smbus_write_byte(sd->client, 0x31) >= 0) {
			break;
		}
	}
	mdelay(10);

	if (i >= CM3623_RETRY_COUNT) {
		printk(KERN_WARNING "cm3623 shutdown PS fail\n");
		return -1;
	}
	return 0;
}

static int cm3623_up_als(struct cm3623_sensor_data *sd)
{
	int i;

	sd->client->addr = CM3623_ALS_WR;
	for (i=0; i<CM3623_RETRY_COUNT; i++) {
		if (i2c_smbus_write_byte(sd->client, 0x02) >= 0) {
			break;
		}
	}
	mdelay(10);

	if (i >= CM3623_RETRY_COUNT) {
		printk(KERN_WARNING "cm3623 up ALS fail\n");
		return -1;
	}
	return 0;
}

static int cm3623_shutdown_als(struct cm3623_sensor_data *sd)
{
	int i;

	sd->client->addr = CM3623_ALS_WR;
	for (i=0; i<CM3623_RETRY_COUNT; i++) {
		if (i2c_smbus_write_byte(sd->client, 0x03) >= 0) {
			break;
		}
	}
	mdelay(10);

	if (i >= CM3623_RETRY_COUNT) {
		printk(KERN_WARNING "cm3623 shutdown ALS fail\n");
		return -1;
	}
	return 0;
}


#ifdef CONFIG_PM
static int cm3623_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cm3623_sensor_data *data = i2c_get_clientdata(client);
	int rc;

	printk("cm3623_suspend\n");
	mutex_lock(&data->lock);

	if ((rc = cm3623_shutdown_als(data)) < 0) {
			return -1;
	}
	//printk("ps_wake %d\n", data->ps_wake);
	if (!data->ps_wake) {
		if (data->use_irq)
			disable_irq(data->client->irq);
		if ((rc = cm3623_shutdown_ps(data) < 0)){
			if (data->power) {
				data->power(0);
				return -1;
			}
		}
	}
	return 0;
}

static int cm3623_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cm3623_sensor_data *data = i2c_get_clientdata(client);
	u8 rc;

	printk("cm3623_resume\n");

#if 0
	if (data->power) {
		data->power(1);
	}
#endif
	//cm3623_config(data);
	if (state[CM3623_ALS] == CM3623_UP ) {	
		if ((rc = cm3623_up_als(data)) < 0) {
				return -1;
		}
	}
	if (state[CM3623_PS] == CM3623_UP ) {
		if ((rc = cm3623_up_ps(data)) < 0) {
			if (data->power) {
				data->power(0);
				return -1;
			}
		}
	}
	if (!data->ps_wake) {
		if (data->use_irq) {
			enable_irq(data->client->irq);
		}
	}
	mutex_unlock(&data->lock);
	return 0;
}
#else
#define cm3623_suspend NULL
#define cm3623_resume NULL
#endif /* CONFIG_PM */

static int cm3623_ps_trigger(struct cm3623_sensor_data *sd, const char *arg)
{
	int threshold;
	int rc = 0;

	rc = sscanf(arg, "%d", &threshold);
	if (1 != rc) {
		printk("Invalid arguments. Usage: <threshold>\n");
		rc = -EINVAL;
		return rc;
	}

	printk("cm3623_ps_trigger %d \n", threshold);

	sd->ps_threshold = cm3623_calc_code(threshold);
	if (sd->ps_threshold < 2) sd->ps_threshold = 2;

	if (sd->use_irq && sd->ps_threshold) {
		flush_work(&sd->work_cfg);
		disable_irq(sd->client->irq);
		queue_work(cm3623_wq, &sd->work_cfg);
		flush_work(&sd->work_cfg);
		enable_irq(sd->client->irq);
	}
	return 0;
}

static ssize_t cm3623_ps_trigger_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int rc = 0;
	struct i2c_client *client = container_of(cm3623_ps_device.parent, struct i2c_client, dev);
	struct cm3623_sensor_data *sd = i2c_get_clientdata(client);

	rc = cm3623_ps_trigger(sd, buf);

	return rc ? : size;
}

static DEVICE_ATTR(ps_trigger, 0220, NULL, cm3623_ps_trigger_store);

static int cm3623_ps_wake(struct cm3623_sensor_data *sd, const char *arg)
{
	int wake;
	int rc = 0;

	rc = sscanf(arg, "%d", &wake);
	if (1 != rc) {
		printk("Invalid arguments. Usage: <wake>\n");
		rc = -EINVAL;
		return rc;
	}

	//printk("cm3623_ps_wake %d \n", wake);

	mutex_lock(&sd->lock);
	sd->ps_wake = (wake ? true : false);
	mutex_unlock(&sd->lock);

	return 0;
}

static ssize_t cm3623_ps_wake_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int rc = 0;
	struct i2c_client *client = container_of(cm3623_ps_device.parent, struct i2c_client, dev);
	struct cm3623_sensor_data *sd = i2c_get_clientdata(client);

	rc = cm3623_ps_wake(sd, buf);

	return rc ? : size;
}

static DEVICE_ATTR(ps_wake, 0220, NULL, cm3623_ps_wake_store);

static int cm3623_fake_data(struct cm3623_sensor_data *sd, const char *arg)
{
	int fake_data;
	int rc = 0;

	rc = sscanf(arg, "%d", &fake_data);
	if (1 != rc) {
		printk("Invalid arguments. Usage: <fake_data>\n");
		rc = -EINVAL;
		return rc;
	}

	//printk("cm3623_fake_data %d \n", fake_data);

	mutex_lock(&sd->lock);
	sd->fake_data = (fake_data ? true : false);
	mutex_unlock(&sd->lock);

	return 0;
}

static ssize_t cm3623_fake_data_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int rc = 0;
	struct i2c_client *client = container_of(cm3623_ps_device.parent, struct i2c_client, dev);
	struct cm3623_sensor_data *sd = i2c_get_clientdata(client);

	rc = cm3623_fake_data(sd, buf);

	return rc ? : size;
}

static DEVICE_ATTR(fake_data, 0220, NULL, cm3623_fake_data_store);

static ssize_t cm3623_cal_ps_data_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = container_of(cm3623_ps_device.parent, struct i2c_client, dev);
	struct cm3623_sensor_data *sd = i2c_get_clientdata(client);
	return sprintf(buf, "%d\n", sd->distance[1]);

}
static DEVICE_ATTR(cal_ps_data, S_IRUGO, cm3623_cal_ps_data_show, NULL);

static ssize_t cm3623_raw_als_data_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = container_of(cm3623_als_device.parent, struct i2c_client, dev);
	struct cm3623_sensor_data *sd = i2c_get_clientdata(client);
	return sprintf(buf, "%d\n", sd->light[0]);

}
static DEVICE_ATTR(raw_als_data, S_IRUGO, cm3623_raw_als_data_show, NULL);

static ssize_t cm3623_ps_control_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int rc;
	struct i2c_client *client = container_of(cm3623_ps_device.parent, struct i2c_client, dev);
	struct cm3623_sensor_data *sd = i2c_get_clientdata(client);

	mutex_lock(&sd->lock);
	if (!(rc = strncmp(buf, "als_u", 5))) {
		if (state[CM3623_ALS] != CM3623_UP) {
			state[CM3623_ALS] = CM3623_UP;
			if ((rc = cm3623_up_als(sd)) < 0) {
					goto end;
			}
		}
	}
	else if (!(rc = strncmp(buf, "als_d", 5))) {
		if (state[CM3623_ALS] != CM3623_DOWN) {
			state[CM3623_ALS] = CM3623_DOWN;
			if ((rc = cm3623_shutdown_als(sd)) < 0) {
					goto end;
			}
		}
	}
	else if (!(rc = strncmp(buf, "ps_u", 4))) {
		if (state[CM3623_PS] != CM3623_UP) {
			state[CM3623_PS] = CM3623_UP;
			if ((rc = cm3623_up_ps(sd)) < 0) {
				if (sd->power) {
					sd->power(0);
					goto end;
				}
			}
		}
	}
	else if (!(rc = strncmp(buf, "ps_d", 4))) {
		if (state[CM3623_PS] != CM3623_DOWN){
			state[CM3623_PS] = CM3623_DOWN;
			if ((rc = cm3623_shutdown_ps(sd)) < 0) {
				if (sd->power) {
					sd->power(0);
					goto end;
				}
			}
		}
	}
	else
		rc = -EINVAL;
end:
	mutex_unlock(&sd->lock);
	return rc ? rc : size;
}
static DEVICE_ATTR(ps_control, 0220, NULL, cm3623_ps_control_store);


static int cm3623_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int rc;

	struct cm3623_sensor_data *sensor_data;
	struct cm3623_platform_data *platform_data;

	//printk("cm3623_probe\n");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		printk(KERN_ERR "cm3623_probe: need I2C_FUNC_I2C\n");
		rc = -ENODEV;
		goto probe_err_check;
	}

	sensor_data = kzalloc(sizeof(*sensor_data), GFP_KERNEL);
	if (!sensor_data) {
		rc = -ENOMEM;
		goto probe_err_alloc;
	}

	INIT_WORK(&sensor_data->work_pull_als, cm3623_work_pull_als);
	INIT_WORK(&sensor_data->work_pull_ps, cm3623_work_pull_ps);	
	INIT_WORK(&sensor_data->work_push, cm3623_work_push);
	INIT_WORK(&sensor_data->work_cfg, cm3623_work_cfg);
	sensor_data->client = client;
	i2c_set_clientdata(client, sensor_data);
	platform_data = client->dev.platform_data;
	if (platform_data) {
		sensor_data->power = platform_data->power;

		if (sensor_data->power) {
			rc = sensor_data->power(1);
			if (rc < 0) {
				printk(KERN_ERR "cm3623_probe power on failed\n");
				goto probe_err_power;
			}
		}

		sensor_data->gpio_int = platform_data->gpio_int;
	}    
	else {
		sensor_data->power = NULL;
		sensor_data->gpio_int = 0;
	}

	if (sensor_data->gpio_int) {
		rc = gpio_request(sensor_data->gpio_int, "cm3623_irq");
		if (rc) {
			printk(KERN_ERR "cm3623_probe gpio_request failed\n");
			goto probe_err_gpio;
		}
		//gpio_tlmm_config(GPIO_CFG(sensor_data->gpio_int, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		client->irq = gpio_to_irq(sensor_data->gpio_int);
	}
	else {
		client->irq = 0;
	}

	sensor_data->ps_wake = false;
	sensor_data->ps_threshold = 0x60;
	rc = cm3623_try_check(sensor_data);
	if (rc < 0) {
		printk(KERN_ERR "cm3623  check failed\n");
		goto probe_err_cfg;	
	}
	rc = cm3623_config(sensor_data);
	if (rc < 0) {
		printk(KERN_ERR "cm3623 config failed\n");
		goto probe_err_cfg;
	}

	cm3623_als_device.parent = &(client->dev);
	rc = misc_register(&cm3623_als_device);
	if (rc) {
		printk(KERN_ERR "cm3623 can't register ALS misc device\n");
		goto probe_err_reg_misc_als;
	}

	cm3623_ps_device.parent = &(client->dev);
	rc = misc_register(&cm3623_ps_device);
	if (rc) {
		printk(KERN_ERR "cm3623 can't register PS misc device\n");
		goto probe_err_reg_misc_ps;
	}

	cm3623_open_push(sensor_data);
	rc = device_create_file(cm3623_ps_device.this_device, &dev_attr_ps_trigger);
	if (rc) {
		printk(KERN_ERR "cm3623 can't create PS trigger file\n");
		goto probe_err_create_ps_trigger;
	}

	rc = device_create_file(cm3623_ps_device.this_device, &dev_attr_ps_wake);
	if (rc) {
		printk(KERN_ERR "cm3623 can't create PS wake file\n");
		goto probe_err_create_ps_wake;
	}

	rc = device_create_file(cm3623_ps_device.this_device, &dev_attr_fake_data);
	if (rc) {
		printk(KERN_ERR "cm3623 can't create fake data file\n");
		goto probe_err_create_fake_data;
	}

	rc = device_create_file(cm3623_ps_device.this_device, &dev_attr_cal_ps_data);
	if (rc) {
		printk(KERN_ERR "cm3623 can't create cal ps  data file\n");
		goto probe_err_create_cal_ps_data;
	}
	rc = device_create_file(cm3623_ps_device.this_device, &dev_attr_ps_control);
	if (rc) {
		printk(KERN_ERR "cm3623 can't create ps control  file\n");
		goto probe_err_create_ps_control;
	}
	rc = device_create_file(cm3623_als_device.this_device, &dev_attr_raw_als_data);
	if (rc) {
		printk(KERN_ERR "cm3623 can't create raw als  data file\n");
		goto probe_err_create_raw_als_data;
	}

	mutex_init(&sensor_data->lock);

#ifdef CONFIG_CM3623_FAKE_DATA
	sensor_data->fake_data = 1;
#endif

	return rc;

probe_err_create_fake_data:
	device_remove_file(cm3623_ps_device.this_device, &dev_attr_ps_wake);
probe_err_create_ps_wake:
	device_remove_file(cm3623_ps_device.this_device, &dev_attr_ps_trigger);
probe_err_create_ps_control:
	device_remove_file(cm3623_ps_device.this_device, &dev_attr_ps_control);
probe_err_create_cal_ps_data:
	device_remove_file(cm3623_ps_device.this_device, &dev_attr_cal_ps_data);	
probe_err_create_raw_als_data:
	device_remove_file(cm3623_als_device.this_device, &dev_attr_raw_als_data);
probe_err_create_ps_trigger:
	cm3623_release_push(sensor_data);
probe_err_reg_misc_ps:
	misc_deregister(&cm3623_als_device);	
probe_err_reg_misc_als:
	i2c_set_clientdata(client, NULL);
probe_err_cfg:
	if (sensor_data->gpio_int) {
		gpio_free(sensor_data->gpio_int);
	}
probe_err_gpio:
probe_err_power:
	kfree(sensor_data);
probe_err_alloc:
probe_err_check:
	return rc;
}

static int __devexit cm3623_remove(struct i2c_client *client)
{
	struct cm3623_sensor_data *sensor_data;

	sensor_data = i2c_get_clientdata(client);

	device_remove_file(cm3623_ps_device.this_device, &dev_attr_fake_data);
	device_remove_file(cm3623_ps_device.this_device, &dev_attr_ps_wake);
	device_remove_file(cm3623_ps_device.this_device, &dev_attr_ps_trigger);
	device_remove_file(cm3623_ps_device.this_device, &dev_attr_cal_ps_data);
	device_remove_file(cm3623_als_device.this_device, &dev_attr_raw_als_data);
	device_remove_file(cm3623_ps_device.this_device, &dev_attr_ps_control);
	cm3623_release_push(sensor_data);
	misc_deregister(&cm3623_ps_device);	
	misc_deregister(&cm3623_als_device);	
	i2c_set_clientdata(client, NULL);
	if (sensor_data->gpio_int) {
		gpio_free(sensor_data->gpio_int);
	}
	kfree(sensor_data);

	return 0;
}

static const struct i2c_device_id cm3623_id[] = {
	{ CM3623_NAME, 0 },
	{ }
};

static const struct dev_pm_ops cm3623_pm_ops = {
	.suspend = cm3623_suspend,
	.resume = cm3623_resume,
};

static struct i2c_driver cm3623_driver = {
	.probe      = cm3623_probe,
	.remove     = __devexit_p(cm3623_remove),
	.id_table   = cm3623_id,
	.driver = {
		.name   = CM3623_NAME,
		.pm     = &cm3623_pm_ops,
	},
};

static int __init cm3623_init(void)
{
	int rc = 0;

	do {
		cm3623_wq = create_singlethread_workqueue("cm3623_wq");
		if (!cm3623_wq) {
			rc = -ENOMEM;
			break;
		}

		rc = i2c_add_driver(&cm3623_driver);
		if (rc < 0) {
			break;
		}

	} while(0);
	return rc;
}
module_init(cm3623_init);

static void __exit cm3623_exit(void)
{
	i2c_del_driver(&cm3623_driver);
	if (cm3623_wq)
		destroy_workqueue(cm3623_wq);
}
module_exit(cm3623_exit);
