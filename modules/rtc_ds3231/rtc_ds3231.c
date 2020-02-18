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
#define REG_OFFSET_SEC			0 /* 00-59 */
#define REG_OFFSET_MIN			1 /* 00-59 */
#define REG_OFFSET_HOUR			2 /* 1-12 or 00-23 */
#define REG_OFFSET_WEEK_DAY		3 /* 1-7; 1 equals Sunday and so on */
#define REG_OFFSET_DATE			4 /* 01-31 */
#define REG_OFFSET_MONTH		5 /* 01-12 */
#define REG_OFFSET_YEAR			6 /* 00-99 */

#define REG_OFFSET_ALARM_1_SEC		0 /* 00-59 */
#define REG_OFFSET_ALARM_1_MIN		1 /* 00-59 */
#define REG_OFFSET_ALARM_1_HOUR		2 /* 1-12 or 00-23 */
#define REG_OFFSET_ALARM_1_WEEK_DAY	3 /* 1-7; 1 equals Sunday and so on */
#define REG_OFFSET_ALARM_1_DATE		3 /* 01-31 */

/* count of register to set or read time data */
#define REG_CNT_FOR_TIME	7

/* count of register to set or read alarm data */
#define REG_CNT_FOR_ALARM	4

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
#define REG_ADDR_ALARM_1_DAY	0x0a /* day/date depends on DY/DT bit into this reg */
#define REG_ADDR_ALARM_1_DATE	0x0a /* day/date depends on DY/DT bit into this reg */

/* alarm 2 register addresses */
#define REG_ADDR_ALARM_2_MIN	0x0b
#define REG_ADDR_ALARM_2_HOUR	0x0c
#define REG_ADDR_ALARM_2_DAY	0x0d
#define REG_ADDR_ALARM_2_DATE	0x0d

/* DEFINITIONS FOR BITS OF STATUS REGISTER: */
/*
 * This bit indicates the device is busy executing TCXO functions.
 * It goes to logic 1 when the conversion signal to the temperature
 * sensor is asserted and then is cleared when the device is in 
 * the 1-minute idle state
 */
#define STATUS_BSY_BIT ((unsigned char)(1 << 2))

/*
 * A logic 1 in this bit indicates that the oscillator either
 * is stopped or was stopped for some period
 */
#define STATUS_OSF_BIT ((unsigned char)(1 << 7))

/*
 * This bit controls the status of the 32kHz pin. When set to logic 1, the
 * 32kHz pin is enabled and outputs a 32.768kHz square wave signal.
 */
#define STATUS_EN32kHz_BIT ((unsigned char)(1 << 3))


/* DEFINITIONS FOR BITS OF CONTROL REGISTER: */
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
 * Rate Select bits for wave output mode
 */
#define CNTRL_RS2_BIT ((unsigned char)(1 << 4))
#define CNTRL_RS1_BIT ((unsigned char)(1 << 3))

#define SET_1_HZ(__val__) \
{ \
	do { \
		__val__ &= ~(CNTRL_RS2_BIT); \
		__val__ &= ~(CNTRL_RS1_BIT); \
	} while (0); \
}

#define SET_1024_HZ(__val__) \
{ \
	do { \
		__val__ &= ~(CNTRL_RS2_BIT); \
		__val__ &= ~(CNTRL_RS1_BIT); \
		__val__ |= CNTRL_RS1_BIT; \
	} while (0); \
}

#define SET_4096_HZ(__val__) \
{ \
	do { \
		__val__ &= ~(CNTRL_RS2_BIT); \
		__val__ &= ~(CNTRL_RS1_BIT); \
		__val__ |= CNTRL_RS2_BIT; \
	} while (0); \
}

#define SET_8192_HZ(__val__) \
{ \
	do { \
		__val__ |= CNTRL_RS2_BIT; \
		__val__ |= CNTRL_RS1_BIT; \
	} while (0); \
}

#define SET_RS_DEFAULT_HZ(__val__) SET_1_HZ(__val__)

/*
 * This bit controls the INT/SQW signal.
 * When the INTCN bit is set to logic 0, a square wave is output on the INT/SQW pin.
 */
#define CNTRL_INTCN_BIT ((unsigned char)(1 << 2))

/*
 * When set to logic 1, this bit permits the alarm 2 flag (A2F) bit in the
 * status register to assert INT/SQW (when INTCN = 1).
 * 1=enable interrupts from ALARM 2 matching to PIN
 */
#define CNTRL_A2IE_BIT ((unsigned char)(1 << 1))

/*
 * When set to logic 1, this bit permits the alarm 1 flag (A1F) bit in the
 * status register to assert INT/SQW (when INTCN = 1).
 * 1=enable interrupts from ALARM 1 matching to PIN
 */
#define CNTRL_A1IE_BIT ((unsigned char)(1 << 0))

/*
 * Internal struct for collect all device driver data
 */
struct ds3231_state {
	struct rtc_device *rtc;
	struct rtc_wkalrm alarm_time;

#define HOUR_REG_FORMAT_BIT_POS		6
	unsigned hour_format_bit:1; /* 1 is 12 hours or 0 is 24 hour */

#define HOUR_REG_AM_PM_BIT_POS		5
	unsigned am_pm_bit:1; /* 1=PM in 12 hour mode; 0: in 24 hour mode = 20-23 hour */

#define ALARM_REG_DY_DT_BIT_POS		6
	unsigned alarm_dy_dt_bit:1; /* 1=alarm will be the result of a match with day of the week. 0=date of the month */

#define ALARM_A1Mx_BIT_POS		7
	unsigned a1m1:1;
	unsigned a1m2:1;
	unsigned a1m3:1;
	unsigned a1m4:1;
};

#define HOURS_FORMAT_12(__ds3231_data__) __ds3231_data__->hour_format_bit = 1
#define HOURS_FORMAT_24(__ds3231_data__) __ds3231_data__->hour_format_bit = 0

#define ALARM_RESULT_OF_DAY_OF_WEEK(__ds3231_data__) __ds3231_data__->alarm_dy_dt_bit = 1
#define ALARM_RESULT_OF_DAY_OF_MONTH(__ds3231_data__) __ds3231_data__->alarm_dy_dt_bit = 0

#define ALARM_ONCE_PER_SEC(__ds3231_data__) \
{ \
	do { \
		__ds3231_data__->a1m1 = 1; \
		__ds3231_data__->a1m2 = 1; \
		__ds3231_data__->a1m3 = 1; \
		__ds3231_data__->a1m4 = 1; \
	} while (0); \
}

#define ALARM_WHEN_SEC_MATCH(__ds3231_data__) \
{ \
	do { \
		__ds3231_data__->a1m1 = 0; \
		__ds3231_data__->a1m2 = 1; \
		__ds3231_data__->a1m3 = 1; \
		__ds3231_data__->a1m4 = 1; \
	} while (0); \
}

#define ALARM_WHEN_SEC_AND_MIN_MATCH(__ds3231_data__) \
{ \
	do { \
		__ds3231_data__->a1m1 = 0; \
		__ds3231_data__->a1m2 = 0; \
		__ds3231_data__->a1m3 = 1; \
		__ds3231_data__->a1m4 = 1; \
	} while (0); \
}

#define ALARM_WHEN_SEC_AND_MIN_AND_HOUR_MATCH(__ds3231_data__) \
{ \
	do { \
		__ds3231_data__->a1m1 = 0; \
		__ds3231_data__->a1m2 = 0; \
		__ds3231_data__->a1m3 = 0; \
		__ds3231_data__->a1m4 = 1; \
	} while (0); \
}

#define ALARM_WHEN_WHOLE_MATCH(__ds3231_data__) \
{ \
	do { \
		__ds3231_data__->a1m1 = 0; \
		__ds3231_data__->a1m2 = 0; \
		__ds3231_data__->a1m3 = 0; \
		__ds3231_data__->a1m4 = 0; \
	} while (0); \
}

/* return 1 if true */
#define IS_DATE_FORMAT_12_HOUR(__ds3231_data__) (__ds3231_data__->hour_format_bit)

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
		dev_info(&client->dev, "%s: i2c read reg 0x%x error\n",
				__func__, base_reg);
		return -EIO;
	}

	return 0;
}

static int ds3231_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	unsigned int pm = 0;
	unsigned char buf_addr[] = { REG_ADDR_SEC };
	unsigned char buf[REG_CNT_FOR_TIME] = { 0 };
	struct i2c_msg msgs[2] = { { 0 }, { 0 } };
	struct ds3231_state *driver_data = NULL;

	driver_data = i2c_get_clientdata(to_i2c_client(dev));
	if (!driver_data) {
		dev_info(dev, "%s: i2c_get_clientdata: error\n", __func__);
		return -EIO;
	}

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

	/* check if PM bit set */
	if (buf[REG_OFFSET_HOUR] & (1 << HOUR_REG_AM_PM_BIT_POS))
		pm = 1;

	tm->tm_sec = bcd2bin(buf[REG_OFFSET_SEC] & 0x7f);
	tm->tm_min = bcd2bin(buf[REG_OFFSET_MIN] & 0x7f);
	tm->tm_hour = bcd2bin(buf[REG_OFFSET_HOUR] & \
			(IS_DATE_FORMAT_12_HOUR(driver_data) ? 0x1f : 0x3f));
	tm->tm_wday = bcd2bin(buf[REG_OFFSET_WEEK_DAY] & 0x07) -1; /* tm_wday:0-6, but rtc:1-7 */
	tm->tm_mday = bcd2bin(buf[REG_OFFSET_DATE] & 0x3f);
	tm->tm_mon = bcd2bin(buf[REG_OFFSET_MONTH] & 0x1f) - 1; /* tm_mon:0-11, but rtc chip is 1-12 */
	tm->tm_year = bcd2bin(buf[REG_OFFSET_YEAR] & 0xff);

	if (IS_DATE_FORMAT_12_HOUR(driver_data)) {
		/*
		 * PM bit = 1 in 12-hour mode,
		 * but Linux rtc structure has 0-23 hour format
		 */
		if (pm) {
			tm->tm_hour += 12;
			driver_data->am_pm_bit = 1;
		}
	}

	dev_info(dev, "%s: secs=%d, mins=%d, hours=%d, "
		"date=%d, month=%d, year=%d, weekday=%d\n",
		__func__,
		tm->tm_sec, tm->tm_min, tm->tm_hour,
		tm->tm_mday, tm->tm_mon, tm->tm_year + 1900, tm->tm_wday);

	return 0;
}

static int ds3231_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	unsigned char set_data[REG_CNT_FOR_TIME + 1] = { REG_ADDR_SEC };
	unsigned char *buf = set_data + 1;
	struct i2c_msg msg = { 0 };
	struct ds3231_state *driver_data = NULL;

	driver_data = i2c_get_clientdata(to_i2c_client(dev));
	if (!driver_data) {
		dev_info(dev, "%s: i2c_get_clientdata: error\n", __func__);
		return -EIO;
	}

	driver_data->am_pm_bit = 0;
	if (IS_DATE_FORMAT_12_HOUR(driver_data)) {
		if (tm->tm_hour >= 12) {
			tm->tm_hour -= 12;
			driver_data->am_pm_bit = 1;
		}
	}

	buf[REG_OFFSET_SEC] = bin2bcd(tm->tm_sec) & 0x7f;
	buf[REG_OFFSET_MIN] = bin2bcd(tm->tm_min) & 0x7f;
	buf[REG_OFFSET_HOUR] = (bin2bcd(tm->tm_hour) & (IS_DATE_FORMAT_12_HOUR(driver_data) ? 0x1f : 0x3f)) | \
			((unsigned char)(driver_data->hour_format_bit << HOUR_REG_FORMAT_BIT_POS)) | \
			((unsigned char)(driver_data->am_pm_bit << HOUR_REG_AM_PM_BIT_POS));
	buf[REG_OFFSET_WEEK_DAY] = (bin2bcd(tm->tm_wday) + 1) & 0x07; /* tm_wday:0-6, but rtc chip is 1-7 */
	buf[REG_OFFSET_DATE] = bin2bcd(tm->tm_mday) & 0x3f;
	buf[REG_OFFSET_MONTH] = (bin2bcd(tm->tm_mon) + 1) & 0x1f; /* tm_mon:0-11, but rtc chip is 1-12 */
	buf[REG_OFFSET_YEAR] = bin2bcd(tm->tm_year) & 0xff;

	msg.addr = (to_i2c_client(dev))->addr;
	msg.len = sizeof(set_data)/sizeof(set_data[0]);
	msg.buf = set_data;

	if (i2c_transfer((to_i2c_client(dev))->adapter, &msg, 1) != 1) {
		dev_info(dev, "%s: i2c write error\n", __func__);
		return -EIO;
	}

	dev_info(dev, "%s: secs=%d, mins=%d, hours=%d, "
		"date=%d, month=%d, year=%d, weekday=%d\n",
		__func__,
		tm->tm_sec, tm->tm_min, tm->tm_hour,
		tm->tm_mday, tm->tm_mon, tm->tm_year + 1900, tm->tm_wday);

	return 0;
}

static int ds3231_read_alarm(struct device *dev, struct rtc_wkalrm *alarm)
{
	unsigned int pm = 0;
	unsigned char buf_addr[] = { REG_ADDR_ALARM_1_SEC };
	unsigned char buf[REG_CNT_FOR_ALARM] = { 0 };
	struct i2c_msg msgs[2] = { { 0 }, { 0 } };
	struct ds3231_state *driver_data = NULL;

	driver_data = i2c_get_clientdata(to_i2c_client(dev));
	if (!driver_data) {
		dev_info(dev, "%s: i2c_get_clientdata: error\n", __func__);
		return -EIO;
	}

	/* setup read ptr */
	msgs[0].addr = (to_i2c_client(dev))->addr;
	msgs[0].len = sizeof(buf_addr)/sizeof(buf_addr[0]);
	msgs[0].buf = buf_addr;

	/* read date */
	msgs[1].addr = (to_i2c_client(dev))->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = REG_CNT_FOR_ALARM;
	msgs[1].buf = buf;

	/* read date registers */
	if (i2c_transfer((to_i2c_client(dev))->adapter, &msgs[0], 2) != 2) {
		dev_info(dev, "%s: i2c read error\n", __func__);
		return -EIO;
	}

	/* check if PM bit set */
	if (buf[REG_OFFSET_HOUR] & (1 << HOUR_REG_AM_PM_BIT_POS))
		pm = 1;

	alarm->time.tm_sec = bcd2bin(buf[REG_OFFSET_ALARM_1_SEC] & 0x7f);
	alarm->time.tm_min = bcd2bin(buf[REG_OFFSET_ALARM_1_MIN] & 0x7f);
	alarm->time.tm_hour = bcd2bin(buf[REG_OFFSET_ALARM_1_HOUR] & \
			(IS_DATE_FORMAT_12_HOUR(driver_data) ? 0x1f : 0x3f));

	if (IS_DATE_FORMAT_12_HOUR(driver_data)) {
		/*
		 * PM bit = 1 in 12-hour mode,
		 * but Linux rtc structure has 0-23 hour format
		 */
		if (pm) {
			alarm->time.tm_hour += 12;
			driver_data->am_pm_bit = 1;
		}
	}

	if (driver_data->alarm_dy_dt_bit)
		alarm->time.tm_wday = bcd2bin(buf[REG_OFFSET_ALARM_1_WEEK_DAY] & 0x07) - 1;
	else
		alarm->time.tm_mday = bcd2bin(buf[REG_OFFSET_ALARM_1_DATE] & 0x3f);

	dev_info(dev, "%s: alarm: secs=%d, mins=%d, hours=%d, "
		"date=%d, weekday=%d\n",
		__func__,
		alarm->time.tm_sec, alarm->time.tm_min, alarm->time.tm_hour,
		alarm->time.tm_mday, alarm->time.tm_wday);

	/* save alarm time to local alarm structure */
	driver_data->alarm_time.time.tm_sec = alarm->time.tm_sec;
	driver_data->alarm_time.time.tm_min = alarm->time.tm_min;
	driver_data->alarm_time.time.tm_hour = alarm->time.tm_hour;
	driver_data->alarm_time.time.tm_wday = alarm->time.tm_wday;
	driver_data->alarm_time.time.tm_mday = alarm->time.tm_mday;

	return 0;
}

static int ds3231_set_alarm(struct device *dev, struct rtc_wkalrm *alarm)
{
	unsigned char set_data[REG_CNT_FOR_ALARM + 1] = { REG_ADDR_ALARM_1_SEC };
	unsigned char *buf = set_data + 1;
	struct i2c_msg msg = { 0 };
	struct ds3231_state *driver_data = NULL;

	driver_data = i2c_get_clientdata(to_i2c_client(dev));
	if (!driver_data) {
		dev_info(dev, "%s: i2c_get_clientdata: error\n", __func__);
		return -EIO;
	}

	/* save alarm time to local alarm structure */
	driver_data->alarm_time.time.tm_sec = alarm->time.tm_sec;
	driver_data->alarm_time.time.tm_min = alarm->time.tm_min;
	driver_data->alarm_time.time.tm_hour = alarm->time.tm_hour;
	driver_data->alarm_time.time.tm_wday = alarm->time.tm_wday;
	driver_data->alarm_time.time.tm_mday = alarm->time.tm_mday;

	driver_data->am_pm_bit = 0;
	if (IS_DATE_FORMAT_12_HOUR(driver_data)) {
		if (alarm->time.tm_hour >= 12) {
			alarm->time.tm_hour -= 12;
			driver_data->am_pm_bit = 1;
		}
	}

	buf[REG_OFFSET_ALARM_1_SEC] = (bin2bcd(alarm->time.tm_sec) & 0x7f) | \
			(unsigned char)(driver_data->a1m1 << ALARM_A1Mx_BIT_POS);
	buf[REG_OFFSET_ALARM_1_MIN] = (bin2bcd(alarm->time.tm_min) & 0x7f) | \
			(unsigned char)(driver_data->a1m2 << ALARM_A1Mx_BIT_POS);
	buf[REG_OFFSET_ALARM_1_HOUR] = (bin2bcd(alarm->time.tm_hour) & \
				(IS_DATE_FORMAT_12_HOUR(driver_data) ? 0x1f : 0x3f)) | \
			((unsigned char)(driver_data->a1m3 << ALARM_A1Mx_BIT_POS)) | \
			((unsigned char)(driver_data->hour_format_bit << HOUR_REG_FORMAT_BIT_POS)) | \
			((unsigned char)(driver_data->am_pm_bit << HOUR_REG_AM_PM_BIT_POS));
	if (driver_data->alarm_dy_dt_bit)
		buf[REG_OFFSET_ALARM_1_WEEK_DAY] = (bin2bcd(alarm->time.tm_wday) & 0x07) | \
				((unsigned char)(driver_data->a1m4 << ALARM_A1Mx_BIT_POS)) | \
				((unsigned char)(driver_data->alarm_dy_dt_bit << ALARM_REG_DY_DT_BIT_POS));
	else
		buf[REG_OFFSET_ALARM_1_DATE] = (bin2bcd(alarm->time.tm_mday) & 0x3f) | \
				((unsigned char)(driver_data->a1m4 << ALARM_A1Mx_BIT_POS)) | \
				((unsigned char)(driver_data->alarm_dy_dt_bit << ALARM_REG_DY_DT_BIT_POS));

	msg.addr = (to_i2c_client(dev))->addr;
	msg.len = sizeof(set_data)/sizeof(set_data[0]);
	msg.buf = set_data;

	if (i2c_transfer((to_i2c_client(dev))->adapter, &msg, 1) != 1) {
		dev_info(dev, "%s: i2c write error\n", __func__);
		return -EIO;
	}

	/*
	 * ToDo: ADD WRITE TO CONTROL REGISTER HERE (bit A1IE)
	 */

	return 0;
}

static const struct rtc_class_ops ds3231_rtc_ops = {
	.read_time	= ds3231_rtc_read_time,
	.set_time	= ds3231_rtc_set_time,
	.read_alarm	= ds3231_read_alarm,
	.set_alarm	= ds3231_set_alarm,
};

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

static int ds3231_force_tcxo_set_bit(struct i2c_client *client, unsigned char *control)
{
	unsigned int while_iterator = 0;

	while (ds3231_check_bsy_bit(client)) {
		/* bsy more than 1 second means hard error into rtc chip */
		if (while_iterator == 50) {
			dev_info(&client->dev, "%s: hard error into ds3231 chip\n", __func__);
			return -EIO;
		}

		msleep(20);
		++while_iterator;
	}

	/* force temprature sensor bit enable */
	*control |= CNTRL_CONV_BIT;

	return 0;
}

/* returns control register value or error */
static int ds3231_check_osc_reset_osf_bit(struct i2c_client *client,
		unsigned char *control, unsigned char *status)
{
	/* check in control that oscillator is running */
	if (*control & CNTRL_EOSC_BIT) {
		dev_info(&client->dev, "%s: control error: oscillator stopped\n", __func__);
		return -EIO;
	}

	/* check in status that oscillator is running */
	if (*status & STATUS_OSF_BIT) {
		/* This bit remains at logic 1 until written to logic 0 */
		(*status) &= ~(STATUS_OSF_BIT); /* reset OSF bit */
	}

	return 0;
}

static int ds3231_init(struct i2c_client *client)
{
	unsigned char control = 0;
	unsigned char status = 0;
	struct ds3231_state *driver_data;

	driver_data = i2c_get_clientdata(client);
	if (!driver_data) {
		dev_info(&client->dev, "%s: i2c_get_clientdata: error\n", __func__);
		return -EIO;
	}

	if (read_reg(client, REG_ADDR_CONTROL, &control) < 0) {
		dev_info(&client->dev, "%s: unable to get chip control reg\n", __func__);
		return -EIO;
	}

	if (read_reg(client, REG_ADDR_STATUS, &status) < 0) {
		dev_info(&client->dev, "%s: unable to get chip status reg\n", __func__);
		return -EIO;
	}

	if (ds3231_check_osc_reset_osf_bit(client, &control, &status) < 0) {
		dev_info(&client->dev, "%s: ds3231_check_osc_reset_osf_bit failed \n", __func__);
		return -EIO;
	}

	/* need to adjust oscillator with environmental temperature */
	if (ds3231_force_tcxo_set_bit(client, &control) < 0) {
		dev_info(&client->dev, "%s: tcxo operation failed \n", __func__);
		return -EIO;
	}

	/* set default driver data */
	HOURS_FORMAT_24(driver_data);
	ALARM_RESULT_OF_DAY_OF_MONTH(driver_data);
	ALARM_WHEN_WHOLE_MATCH(driver_data);

#ifdef CONFIG_RTC_DRV_DS3231_FORMAT_12
	HOURS_FORMAT_12(driver_data);
#endif

	status &= ~(STATUS_EN32kHz_BIT); /* disable 32kHz output */

#ifdef CONFIG_RTC_DRV_DS3231_32kHZ_OUTPUT_EN
	status  |= STATUS_EN32kHz_BIT;
#endif

	/*
	 * Apply default value
	 * Code below will rewrite this values if
	 * corresponding definition is defined
	 */
	control &= ~(CNTRL_BBSQW_BIT); /* disable wave output */
	control &= ~(CNTRL_INTCN_BIT); /* disable interrupts */
	control &= ~(CNTRL_A2IE_BIT); /* disable alarm 2 interrupt */
	control &= ~(CNTRL_A1IE_BIT); /* disable alarm 1 interrupt */

	SET_RS_DEFAULT_HZ(control);

#ifndef CONFIG_RTC_DRV_DS3231_NOT_USE_SQW_PIN
# ifdef CONFIG_RTC_DRV_DS3231_SQUARE_WAVE_EN
	control |= CNTRL_BBSQW_BIT;

#  ifdef CONFIG_RTC_DRV_DS3231_SQUARE_1_HZ
	SET_1_HZ(control);
#  endif

#  ifdef CONFIG_RTC_DRV_DS3231_SQUARE_1024_HZ
	SET_1024_HZ(control);
#  endif

#  ifdef CONFIG_RTC_DRV_DS3231_SQUARE_4096_HZ
	SET_4096_HZ(control);
#  endif

#  ifdef CONFIG_RTC_DRV_DS3231_SQUARE_8192_HZ
	SET_8192_HZ(control);
#  endif
# endif  // CONFIG_RTC_DRV_DS3231_SQUARE_WAVE_EN

# ifdef CONFIG_RTC_DRV_DS3231_ALARM_INTERRUPTS_EN
	control |= CNTRL_INTCN_BIT; /* enable INT */

	/*
	 * ToDo: Create kernel thread and semaphore for handler interrupts
	 * In IT handler you need reset RTC IT ALARM A1F flag in status register.
	 * When this flag is 1, INT/SQW pin is also asserted.
	 * This flag can only be written to logic 0
	 */
# endif
#endif  // CONFIG_RTC_DRV_DS3231_NOT_USE_SQW_PIN

	if (write_reg(client, REG_ADDR_CONTROL, control) < 0) {
		dev_info(&client->dev, "%s: can't set control register\n", __func__);
		return -EIO;
	}

	if (write_reg(client, REG_ADDR_STATUS, status) < 0) {
		dev_info(&client->dev, "%s: can't set control register\n", __func__);
		return -EIO;
	}

	/* wait untill tcxo operation is finished */
	msleep(20);


	/*
	 * ToDo: creatre sysfs files to set/read alarms
	 */

	return 0;
}

static int ds3231_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct ds3231_state *driver_data = &ds3231_data;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
		return -ENODEV;

	i2c_set_clientdata(client, driver_data);

	if (ds3231_init(client) < 0)
		return -ENODEV;

	driver_data->rtc = devm_rtc_device_register(&client->dev,
			ds3231_driver.driver.name, &ds3231_rtc_ops, THIS_MODULE);

	if (IS_ERR(driver_data->rtc))
		return PTR_ERR(driver_data->rtc);

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
