/*
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
 
#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#define MAX17040_P01D_VCELL_MSB	0x02
#define MAX17040_P01D_VCELL_LSB	0x03
#define MAX17040_P01D_SOC_MSB	0x04
#define MAX17040_P01D_SOC_LSB	0x05
#define MAX17040_P01D_MODE_MSB	0x06
#define MAX17040_P01D_MODE_LSB	0x07
#define MAX17040_P01D_VER_MSB	0x08
#define MAX17040_P01D_VER_LSB	0x09
#define MAX17040_P01D_RCOMP_MSB	0x0C
#define MAX17040_P01D_RCOMP_LSB	0x0D
#define MAX17040_P01D_CMD_MSB	0xFE
#define MAX17040_P01D_CMD_LSB	0xFF

#define MAX17040_P01D_DELAY		1000

struct max17040_p01d_chip {
	struct i2c_client		*client;
	struct delayed_work		work;

	/* battery voltage */
	int vcell;
	/* battery capacity */
	int soc;
};

static struct max17040_p01d_chip *g_chip = NULL;

static int max17040_p01d_write_reg(struct i2c_client *client, int reg, u8 value)
{
	int ret;

	ret = i2c_smbus_write_byte_data(client, reg, value);

	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return ret;
}

static int max17040_p01d_read_reg(struct i2c_client *client, int reg)
{
	int ret;

	ret = i2c_smbus_read_byte_data(client, reg);

	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return ret;
}

static void max17040_p01d_reset(struct i2c_client *client)
{
	max17040_p01d_write_reg(client, MAX17040_P01D_CMD_MSB, 0x54);
	max17040_p01d_write_reg(client, MAX17040_P01D_CMD_LSB, 0x00);
}

static void max17040_p01d_get_vcell(struct i2c_client *client)
{
	struct max17040_p01d_chip *chip = i2c_get_clientdata(client);
	u8 msb;
	u8 lsb;

	msb = max17040_p01d_read_reg(client, MAX17040_P01D_VCELL_MSB);
	lsb = max17040_p01d_read_reg(client, MAX17040_P01D_VCELL_LSB);

	chip->vcell = (msb << 4) + (lsb >> 4);
}

static void max17040_p01d_get_soc(struct i2c_client *client)
{
	struct max17040_p01d_chip *chip = i2c_get_clientdata(client);
	u8 msb;
	u8 lsb;

	msb = max17040_p01d_read_reg(client, MAX17040_P01D_SOC_MSB);
	lsb = max17040_p01d_read_reg(client, MAX17040_P01D_SOC_LSB);
    
	chip->soc = msb;
}

int max17040_p01d_get_capacity(void)
{
	if (!g_chip)
		return -1;
	return g_chip->soc;
}
EXPORT_SYMBOL(max17040_p01d_get_capacity);

static void max17040_p01d_get_version(struct i2c_client *client)
{
	u8 msb;
	u8 lsb;

	msb = max17040_p01d_read_reg(client, MAX17040_P01D_VER_MSB);
	lsb = max17040_p01d_read_reg(client, MAX17040_P01D_VER_LSB);

	dev_info(&client->dev, "MAX17040_P01D Fuel-Gauge Ver %d%d\n", msb, lsb);
}

static void max17040_p01d_work(struct work_struct *work)
{
	struct max17040_p01d_chip *chip;

	chip = container_of(work, struct max17040_p01d_chip, work.work);

	max17040_p01d_get_vcell(chip->client);
	max17040_p01d_get_soc(chip->client);

	schedule_delayed_work(&chip->work, MAX17040_P01D_DELAY);
}

static int __devinit max17040_p01d_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct max17040_p01d_chip *chip;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE))
		return -EIO;

	chip = kzalloc(sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->client = client;

	i2c_set_clientdata(client, chip);

	max17040_p01d_reset(client);
	max17040_p01d_get_version(client);

	INIT_DELAYED_WORK_DEFERRABLE(&chip->work, max17040_p01d_work);
	schedule_delayed_work(&chip->work, MAX17040_P01D_DELAY);

	g_chip = chip;

	return 0;
}

static int __devexit max17040_p01d_remove(struct i2c_client *client)
{
	struct max17040_p01d_chip *chip = i2c_get_clientdata(client);

	cancel_delayed_work(&chip->work);
	kfree(chip);
	return 0;
}

#ifdef CONFIG_PM

static int max17040_p01d_suspend(struct i2c_client *client,
		pm_message_t state)
{
	struct max17040_p01d_chip *chip = i2c_get_clientdata(client);

	cancel_delayed_work(&chip->work);
	return 0;
}

static int max17040_p01d_resume(struct i2c_client *client)
{
	struct max17040_p01d_chip *chip = i2c_get_clientdata(client);

	max17040_p01d_get_vcell(chip->client);
	max17040_p01d_get_soc(chip->client);
	schedule_delayed_work(&chip->work, MAX17040_P01D_DELAY);
	return 0;
}

#else

#define max17040_p01d_suspend NULL
#define max17040_p01d_resume NULL

#endif /* CONFIG_PM */

static const struct i2c_device_id max17040_p01d_id[] = {
	{ "max17040-p01d", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, max17040_p01d_id);

static struct i2c_driver max17040_p01d_i2c_driver = {
	.driver	= {
		.name	= "max17040-p01d",
	},
	.probe		= max17040_p01d_probe,
	.remove		= __devexit_p(max17040_p01d_remove),
	.suspend	= max17040_p01d_suspend,
	.resume		= max17040_p01d_resume,
	.id_table	= max17040_p01d_id,
};

static int __init max17040_p01d_init(void)
{
	return i2c_add_driver(&max17040_p01d_i2c_driver);
}
module_init(max17040_p01d_init);

static void __exit max17040_p01d_exit(void)
{
	i2c_del_driver(&max17040_p01d_i2c_driver);
}
module_exit(max17040_p01d_exit);

MODULE_AUTHOR("Hiroyuki Ikezoe <hiikezoe@gnome.org>");
MODULE_DESCRIPTION("MAX17040 P-01D Fuel Gauge");
MODULE_LICENSE("GPL");
