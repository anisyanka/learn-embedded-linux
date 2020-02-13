/*
 * RTC client/driver for the Maxim/Dallas DS3231 Real-Time Clock
 */
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/regmap.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/rtc.h>
#include <linux/bcd.h>
#include <linux/device.h>

/* offsets into register area */
#define R_OFF_SEC			0 /* 00-59 */
#define R_OFF_MIN			1 /* 00-59 */
#define R_OFF_HOUR			2 /* 1-12 or 00-23 */
#define R_OFF_WEEK_DAY			3 /* 1-7; 1 equals Sunday and so on */
#define R_OFF_DATE			4 /* 01-31 */
#define R_OFF_MONTH			5 /* 01-12 */
#define R_OFF_YEAR			6 /* 00-99 */

/* count of register to set ot read time data */
#define REG_CNT_FOR_TIME 		7

/* register addresses */
#define DS3231_REG_ADDR_SEC		0x00
#define DS3231_REG_ADDR_MIN		0x01
#define DS3231_REG_ADDR_HOUR		0x02
#define DS3231_REG_ADDR_DAY		0x03
#define DS3231_REG_ADDR_DATE		0x04
#define DS3231_REG_ADDR_MONTH		0x05
#define DS3231_REG_ADDR_YEAR		0x06
#define DS3231_REG_ADDR_CONTROL		0x0E
#define DS3231_REG_ADDR_STATUS		0x0F
#define DS3231_REG_ADDR_AGING_OFF	0x10
#define DS3231_REG_ADDR_TEMP_MSB	0x11
#define DS3231_REG_ADDR_TEMP_LSB	0x12

/* alarm 1 register addresses */
#define DS3231_REG_ADDR_ALARM_1_SEC	0x0
#define DS3231_REG_ADDR_ALARM_1_MIN	0x0
#define DS3231_REG_ADDR_ALARM_1_HOUR	0x0
#define DS3231_REG_ADDR_ALARM_1_DAY	0x0
#define DS3231_REG_ADDR_ALARM_1_DATE	0x0

/* alarm 2 register addresses */
#define DS3231_REG_ADDR_ALARM_2_MIN	0x0
#define DS3231_REG_ADDR_ALARM_2_HOUR	0x0
#define DS3231_REG_ADDR_ALARM_2_DAY	0x0
#define DS3231_REG_ADDR_ALARM_2_DATE	0x0

/*
 * Internal struct for collect all device driver data
 */
struct ds3231_state {
	struct rtc_device *rtc;
	unsigned hour_format:1; /* 1 is 12 hours or 0 is 24 hour */
	unsigned am_pm_bit:1; /* 1=PM in 12 hour mode; in 24 hour mode = 20-23 hour */
};

static struct i2c_driver ds3231_driver;
static struct ds3231_state ds3231_data;

static unsigned char ds3231_get_status(struct i2c_client *client)
{
	unsigned char status = 0;
	unsigned char data[] = { DS3231_REG_ADDR_STATUS };
	struct i2c_msg msgs[] = {
		{ /* setup read ptr */
			.addr = client->addr,
			.len = 1,
			.buf = &data[0]
		},
		{ /* read status */
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = 1,
			.buf = &status
		},
	};

	if (i2c_transfer(client->adapter, &msgs[0], 2) != 2) {
		dev_err(&client->dev, "%s: read error\n", __func__);
		return -EIO;
	}

	return status;
}

static int ds3231_get_datetime(struct i2c_client *client, struct rtc_time *tm,
		unsigned char reg_base)
{
	return 0;
}

static int ds3231_set_datetime(struct i2c_client *client, struct rtc_time *tm,
		unsigned char reg_base, unsigned char alm_enable)
{
	return 0;
}

static int ds3231_rtc_proc(struct device *dev, struct seq_file *seq)
{
	dev_dbg(dev, "%s\n", __func__);

	return 0;
}

static int ds3231_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	dev_dbg(dev, "%s\n", __func__);

	return ds3231_get_datetime(to_i2c_client(dev),
			tm, DS3231_REG_ADDR_SEC);
}

static int ds3231_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	dev_dbg(dev, "%s\n", __func__);

	return ds3231_set_datetime(to_i2c_client(dev),
			tm, DS3231_REG_ADDR_SEC, 0);
}

static int ds3231_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	dev_dbg(dev, "%s\n", __func__);

	return ds3231_get_datetime(to_i2c_client(dev),
			&alrm->time, DS3231_REG_ADDR_ALARM_1_SEC);
}

static int ds3231_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	dev_dbg(dev, "%s\n", __func__);

	return ds3231_set_datetime(to_i2c_client(dev),
			&alrm->time, DS3231_REG_ADDR_ALARM_1_SEC, 1);
}

static const struct rtc_class_ops ds3231_rtc_ops = {
	.proc		= ds3231_rtc_proc,
	.read_time	= ds3231_rtc_read_time,
	.set_time	= ds3231_rtc_set_time,
	.read_alarm	= ds3231_rtc_read_alarm,
	.set_alarm	= ds3231_rtc_set_alarm,
};

static int ds3231_validate_client(struct i2c_client *client)
{
	return 0;
}

/*
 * We can create own entries in sysfs.
 * Regisert and unregister it with this functions
 */
static int ds3231_sysfs_register(struct device *dev)
{
	return 0;
}

static int ds3231_sysfs_unregister(struct device *dev)
{
	return 0;
}

static int ds3231_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int err = 0;
	struct ds3231_state *data = &ds3231_data;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
		return -ENODEV;

	/* Probe array. We will read the register at the specified
	 * address and check if the given bits are zero.
	 */
	if (ds3231_validate_client(client) < 0)
		return -ENODEV;

	/* Allocate driver state, point i2c client data to it */
	data->rtc = devm_rtc_device_register(&client->dev,
			ds3231_driver.driver.name, &ds3231_rtc_ops, THIS_MODULE);

	if (IS_ERR(data->rtc))
		return PTR_ERR(data->rtc);

	i2c_set_clientdata(client, data);

	/* create specific files in sysfs */
	err = ds3231_sysfs_register(&client->dev);
	if (err)
		dev_err(&client->dev, "Unable to create sysfs entries\n");

	return 0;
}

static int ds3231_remove(struct i2c_client *client)
{
	return ds3231_sysfs_unregister(&client->dev);
}

/*
 * Struct used for automatically matching a device described in the device tree.
 *
 * `compatible` must be same as in the device tree.
 * Kernel will detect into device tree that will search device driver in
 * its device table. If the one will be founded, `probe` function will be used
 * for check real hardware.
 *
 * If we do not want use device tree, we can not use this struct
 * and juct register driver as module.
 * 
 * See the https://stackoverflow.com/questions/42018032/what-is-use-of-struct-i2c-device-id-if-we-are-already-using-struct-of-device-id?rq=1
 * to understand different between i2c_device_id and of_device_id.
 */
static const struct of_device_id ds3231_of_match[] = {
	{
		.compatible = "my-custom,my-ds3231",
		.data = NULL
	},
	{ }
};
MODULE_DEVICE_TABLE(of, ds3231_of_match);

static const struct i2c_device_id ds3231_id[] = {
	{ "my-ds3231", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ds3231_id);

static struct i2c_driver ds3231_driver = {
	.driver = {
		.name = "my-ds3231-rtc",
		.of_match_table = of_match_ptr(ds3231_of_match),
	},
	.probe = ds3231_probe,
	.remove = ds3231_remove,
	.id_table = ds3231_id,
};

module_i2c_driver(ds3231_driver);

MODULE_DESCRIPTION("Maxim-Integrated DS3231 in my realization for learning");
MODULE_AUTHOR("Anisimov Aleksandr <anisimov.alexander.s@gmail.com>");
MODULE_LICENSE("GPL");
