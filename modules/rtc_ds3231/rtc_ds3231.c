/*
 * RTC client/driver for the Maxim/Dallas DS3231 Real-Time Clock
 */
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/rtc.h>
#include <linux/bcd.h>
#include <linux/device.h>
#include <linux/delay.h>

/* offsets into register area */
#define REG_OFFSET_SEC		0 /* 00-59 */
#define REG_OFFSET_MIN		1 /* 00-59 */
#define REG_OFFSET_HOUR		2 /* 1-12 or 00-23 */
#define REG_OFFSET_WEEK_DAY	3 /* 1-7; 1 equals Sunday and so on */
#define REG_OFFSET_DATE		4 /* 01-31 */
#define REG_OFFSET_MONTH	5 /* 01-12 */
#define REG_OFFSET_YEAR		6 /* 00-99 */

/* count of register to set ot read time data */
#define REG_CNT_FOR_TIME 	7

/* register addresses */
#define REG_ADDR_SEC		0x00
#define REG_ADDR_MIN		0x01
#define REG_ADDR_HOUR		0x02
#define REG_ADDR_DAY		0x03
#define REG_ADDR_DATE		0x04
#define REG_ADDR_MONTH		0x05
#define REG_ADDR_YEAR		0x06
#define REG_ADDR_CONTROL	0x0e
#define REG_ADDR_STATUS		0x0f
#define REG_ADDR_AGING_OFF	0x10
#define REG_ADDR_TEMP_MSB	0x11
#define REG_ADDR_TEMP_LSB	0x12

/* alarm 1 register addresses */
#define REG_ADDR_ALARM_1_SEC	0x07
#define REG_ADDR_ALARM_1_MIN	0x08
#define REG_ADDR_ALARM_1_HOUR	0x09
#define REG_ADDR_ALARM_1_DAY	0x0a
#define REG_ADDR_ALARM_1_DATE	0x0a

/* alarm 2 register addresses */
#define REG_ADDR_ALARM_2_MIN	0x0b
#define REG_ADDR_ALARM_2_HOUR	0x0c
#define REG_ADDR_ALARM_2_DAY	0x0d
#define REG_ADDR_ALARM_2_DATE	0x0d

/* defines for bits of status register */
/*
 * This bit indicates the device is busy executing TCXO functions.
 * It goes to logic 1 when the conversion signal to the temperature
 * sensor is asserted and then is cleared when the device is in 
 * the 1-minute idle state
 */
#define STATUS_BSY_BIT ((unsigned char)(1 << 2))

/* defines for bits of control register */
/*
 * When set to logic 0, the oscillator is started.
 * Set automatically, when ds3231 is powered of Vcc.
 * When EOSC is 1, all register data is static and oscillator is stopped.
 */
#define CNTRL_EOSC_BIT ((unsigned char)(1 << 7))

/*
 * When set to logic 1 with INTCN = 0, this bit enables the square wave.
 * When BBSQW is logic 0, the INT/SQW pin goes high impedance when.
 */
#define CNTRL_BBSQW_BIT ((unsigned char)(1 << 6))

/*
 * Setting this bit to 1 forces the temperature sensor to convert
 * the temperature into digital code and execute the TCXO algorithm to
 * update the capacitance array to the oscillator.
 */
#define CNTRL_CONV_BIT ((unsigned char)(1 << 5))

/*
 * Internal struct for collect all device driver data
 */
struct ds3231_state {
	struct rtc_device *rtc;
#define HOUR_REG_FORMAT_BIT_POS		6
#define HOUR_REG_AM_PM_BIT_POS		5
	unsigned hour_format:1; /* 1 is 12 hours or 0 is 24 hour */
	unsigned am_pm_bit:1; /* 1=PM in 12 hour mode; in 24 hour mode = 20-23 hour */
};

static struct i2c_driver ds3231_driver;
static struct ds3231_state ds3231_data;

static int write_reg(struct i2c_client *client, unsigned char base_reg,
		unsigned char value)
{
	unsigned char data[2];
	struct i2c_msg msg = { 0 };

	data[0] = base_reg;
	data[1] = value;

	msg.addr = client->addr;
	msg.len = 2;
	msg.buf = data;

	if (i2c_transfer(client->adapter, &msg, 1) != 1) {
		dev_info(&client->dev, "%s: write to reg 0x%x error\n", 
				__func__, base_reg);
		return -EIO;
	}

	return 0;
}

static int read_reg(struct i2c_client *client, unsigned char base_reg,
		unsigned char *read_byte)
{
	struct i2c_msg msgs[] = {
		{ /* setup read ptr */
			.addr = client->addr,
			.len = 1,
			.buf = &base_reg
		},
		{ /* read count registers */
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = 1,
			.buf = read_byte
		},
	};

	if (i2c_transfer(client->adapter, &msgs[0], 2) != 2) {
		dev_info(&client->dev, "%s: reg reg 0x%x error\n",
				__func__, base_reg);
		return -EIO;
	}

	return 0;
}

/* 1=busy; */
static int ds3231_check_bsy_bit(struct i2c_client *client)
{
	unsigned char status = 0;

	if (read_reg(client, REG_ADDR_STATUS, &status) < 0) {
		dev_info(&client->dev, "%s: unable to get chip status reg\n", __func__);
		return -EIO;
	}

	if (status & STATUS_BSY_BIT)
		return 1;

	return 0;
}

static int ds3231_force_tcxo(struct i2c_client *client)
{
	unsigned char control = 0;

	while (ds3231_check_bsy_bit(client))
		msleep(20);

	if (read_reg(client, REG_ADDR_CONTROL, &control) < 0) {
		dev_info(&client->dev, "%s: unable to get chip control reg\n", __func__);
		return -EIO;
	}

	/* force temprature sensor bit enable */
	control |= CNTRL_CONV_BIT;

	if (write_reg(client, REG_ADDR_CONTROL, control) < 0) {
		dev_info(&client->dev, "%s: can't set CNTRL_CONV_BIT\n", __func__);
		return -EIO;
	}

	/* wait while tcxo operation finished */
	msleep(20);

	return 0;
}

static int ds3231_init(struct i2c_client *client)
{
	unsigned char control = 0;

	if (read_reg(client, REG_ADDR_CONTROL, &control) < 0) {
		dev_info(&client->dev, "%s: unable to get chip control reg\n", __func__);
		return -EIO;
	}

	/* check that oscillator is running */
	if (control & CNTRL_EOSC_BIT) {
		dev_info(&client->dev, "%s: error: oscillator stopped\n", __func__);
		return -EIO;
	}

	/* need to adjust oscillator with environmental temperature */
	if (ds3231_force_tcxo(client)) {
		dev_info(&client->dev, "%s: tcxo operation failed \n", __func__);
		return -EIO;
	}

	return 0;
}

static int ds3231_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	unsigned char buf_addr[] = { REG_ADDR_SEC };
	unsigned char buf[REG_CNT_FOR_TIME] = { 0 };
	struct i2c_msg msgs[2] = { { 0 }, { 0 } };

	/* setup read ptr */
	msgs[0].addr = (to_i2c_client(dev))->addr;
	msgs[0].len = sizeof(buf_addr)/sizeof(buf_addr[0]);
	msgs[0].buf = buf_addr;

	/* read date */
	msgs[1].addr = (to_i2c_client(dev))->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = REG_CNT_FOR_TIME;
	msgs[1].buf = buf;

	/* read date registers */
	if (i2c_transfer((to_i2c_client(dev))->adapter, &msgs[0], 2) != 2) {
		dev_info(dev, "%s: read error\n", __func__);
		return -EIO;
	}

	tm->tm_sec = bcd2bin(buf[REG_OFFSET_SEC] & 0x7f);
	tm->tm_min = bcd2bin(buf[REG_OFFSET_MIN] & 0x7f);
	tm->tm_hour = bcd2bin(buf[REG_OFFSET_HOUR] & 0x3f);
	tm->tm_wday = bcd2bin(buf[REG_OFFSET_WEEK_DAY] & 0x07) -1; /* tm_wday:0-6, but rtc:1-7 */
	tm->tm_mday = bcd2bin(buf[REG_OFFSET_DATE] & 0x3f);
	tm->tm_mon = bcd2bin(buf[REG_OFFSET_MONTH] & 0x1f) - 1; /* tm_mon:0-1, but rtc:1-12 */
	tm->tm_year = bcd2bin(buf[REG_OFFSET_YEAR] & 0xff);

	dev_info(dev, "%s: secs=%d, mins=%d, hours=%d, "
		"date=%d, month=%d, year=%d, weekday=%d\n",
		__func__,
		tm->tm_sec, tm->tm_min, tm->tm_hour,
		tm->tm_mday, tm->tm_mon, tm->tm_year + 1900, tm->tm_wday);

	return 0;
}

static int ds3231_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	unsigned char rdata[REG_CNT_FOR_TIME + 1] = { REG_ADDR_SEC };
	unsigned char *buf = rdata + 1;
	struct i2c_msg msg = { 0 };
	struct ds3231_state *driver_data = NULL;

	driver_data = i2c_get_clientdata(to_i2c_client(dev));
	if (!driver_data) {
		dev_info(dev, "%s: i2c_get_clientdata: error\n", __func__);
		return -EIO;
	}

	buf[REG_OFFSET_SEC] = bin2bcd(tm->tm_sec) & 0x7f;
	buf[REG_OFFSET_MIN] = bin2bcd(tm->tm_min) & 0x7f;
	buf[REG_OFFSET_HOUR] = (bin2bcd(tm->tm_hour) & 0x3f) | \
		(unsigned char)(driver_data->hour_format << HOUR_REG_FORMAT_BIT_POS) | \
		(unsigned char)(driver_data->am_pm_bit << HOUR_REG_AM_PM_BIT_POS);
	buf[REG_OFFSET_WEEK_DAY] = (bin2bcd(tm->tm_wday) + 1) & 0x07;
	buf[REG_OFFSET_DATE] = bin2bcd(tm->tm_mday) & 0x3f;
	buf[REG_OFFSET_MONTH] = (bin2bcd(tm->tm_mon) + 1) & 0x1f;
	buf[REG_OFFSET_YEAR] = bin2bcd(tm->tm_year) & 0xff;

	msg.addr = (to_i2c_client(dev))->addr;
	msg.len = sizeof(rdata)/sizeof(rdata[0]);
	msg.buf = rdata;

	dev_info(dev, "%s: secs=%d, mins=%d, hours=%d, "
		"date=%d, month=%d, year=%d, weekday=%d\n",
		__func__,
		tm->tm_sec, tm->tm_min, tm->tm_hour,
		tm->tm_mday, tm->tm_mon, tm->tm_year + 1900, tm->tm_wday);

	if (i2c_transfer((to_i2c_client(dev))->adapter, &msg, 1) != 1) {
		dev_info(dev, "%s: write error\n", __func__);
		return -EIO;
	}

	return 0;
}

static const struct rtc_class_ops ds3231_rtc_ops = {
	.read_time	= ds3231_rtc_read_time,
	.set_time	= ds3231_rtc_set_time,
	.read_alarm	= NULL,
	.set_alarm	= NULL,
};

static int ds3231_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct ds3231_state *driver_data = &ds3231_data;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
		return -ENODEV;

	if (ds3231_init(client) < 0)
		return -ENODEV;

	driver_data->rtc = devm_rtc_device_register(&client->dev,
			ds3231_driver.driver.name, &ds3231_rtc_ops, THIS_MODULE);

	if (IS_ERR(driver_data->rtc))
		return PTR_ERR(driver_data->rtc);

	i2c_set_clientdata(client, driver_data);

	return 0;
}

static int ds3231_remove(struct i2c_client *client)
{
	return 0;
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

MODULE_DESCRIPTION("Maxim-Integrated DS3231 driver in my realization for learning");
MODULE_AUTHOR("Anisimov Aleksandr <anisimov.alexander.s@gmail.com>");
MODULE_LICENSE("GPL");
