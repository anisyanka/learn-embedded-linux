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
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/stat.h>
#include <linux/interrupt.h>

/*
 * After loading this module to kernel (or statically linked with kernel) in the
 * /sys/class/rtc/rtc0/device will appear to new files(attributes):
 *
 * 1. alarm_int_enabled - file to enable or disable detect alarm interrupt
 *      write:
 *            `echo 0 > /sys/class/rtc/rtc0/device/alarm_int_enabled` - disable INT
 *            `echo 1 > /sys/class/rtc/rtc0/device/alarm_int_enabled` - enable INT
 *      read:
 *            `cat /sys/class/rtc/rtc0/device/alarm_int_enabled` - returns current int status
 *             1 - interrupts enabled
 *             0 - interrupts disabled
 *
 * 2. alarm_time - file to set time for alaram
 *       write:
 *            `echo "Tue May 26 21:51:50 2015" >> /sys/class/rtc/rtc0/device/alarm_time` - set alaram
 *       read:
 *            `cat /sys/class/rtc/rtc0/device/alarm_time` - read current setted alaram time
 */

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

/*
 * A logic 1 in the alarm 1/2 flag bit indicates that the time matched the
 * alarm 1/2 registers. If the A1IE bit is logic 1 and the INTCN bit is set
 * to logic 1, the INT/SQW pin is also asserted.
 *
 * A1F is cleared when written to logic 0. This bit can only be written
 * to logic 0. Attempting to write to logic 1 leaves the value unchanged.
 */
#define STATUS_A2F_BIT ((unsigned char)(1 << 1))
#define STATUS_A1F_BIT ((unsigned char)(1 << 0))

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
#ifdef CONFIG_RTC_DRV_DS3231_ALARM_INTERRUPTS_EN
	/* '1' means that alarm was enabled from user space */
	int alarm_int_enabled;
	int alarm_int_enabled_old;

	/* It used in interrupt handler and created kernel thread.*/
	struct semaphore alarm_interrupt_sem;
	struct task_struct *thread;
	int task_exit:1;

	/*
	 * P9.15 in beaglebone board.
	 * This pin fall to logic zero when alarm interrupt happens.
	 *
	 * ToDo: remove hardcoded GPIO pin. Do it with devicetree.
	 * https://stackoverflow.com/questions/39212771/linux-4-5-gpio-interrupt-through-devicetree-on-xilinx-zynq-platform?rq=1
	 */
	#define ALARM_INT_GPIO 48
	#define ALARM_INT_GPIO_LABEL "ds3231 GPIO"
	int irq;
#endif
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

#ifdef CONFIG_RTC_DRV_DS3231_ALARM_INTERRUPTS_EN
static int ds3231_disable_onchip_alarm_detect(struct i2c_client *client)
{
	unsigned char control;

	if (read_reg(client, REG_ADDR_CONTROL, &control) < 0) {
		dev_info(&client->dev,
			"%s: unable to get chip control reg\n", __func__);
		return -EIO;
	}

	control &= ~(CNTRL_A2IE_BIT);
	control &= ~(CNTRL_A1IE_BIT);

	if (write_reg(client, REG_ADDR_CONTROL, control) < 0) {
		dev_info(&client->dev,
			"%s: can't set control register\n",
			__func__);
		return -EIO;
	}

	return 0;
}

static int ds3231_clear_onchip_alarm_flags(struct i2c_client *client)
{
	unsigned char status;

	if (read_reg(client, REG_ADDR_STATUS, &status) < 0) {
		dev_info(&client->dev,
			"%s: unable to get chip status reg\n", __func__);
		return -EIO;
	}

	status &= ~(STATUS_A2F_BIT);
	status &= ~(STATUS_A1F_BIT);

	if (write_reg(client, REG_ADDR_STATUS, status) < 0) {
		dev_info(&client->dev,
			"%s: can't set status register\n",
			__func__);
		return -EIO;
	}

	return 0;
}

static int ds3231_enable_onchip_alarm_detect(struct i2c_client *client)
{
	unsigned char control;

	if (read_reg(client, REG_ADDR_CONTROL, &control) < 0) {
		dev_info(&client->dev,
			"%s: unable to get chip control reg\n", __func__);
		return -EIO;
	}

	control |= CNTRL_INTCN_BIT;
	control |= CNTRL_A1IE_BIT;

	if (write_reg(client, REG_ADDR_CONTROL, control) < 0) {
		dev_info(&client->dev,
			"%s: can't set control register\n",
			__func__);
		return -EIO;
	}

	return 0;
}

#endif

static int ds3231_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	unsigned int pm = 0;
	unsigned char buf_addr[] = { REG_ADDR_SEC };
	unsigned char buf[REG_CNT_FOR_TIME] = { 0 };
	struct i2c_msg msgs[2] = { { 0 }, { 0 } };
	struct ds3231_state *driver_data;

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
	struct ds3231_state *driver_data;

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
	struct ds3231_state *driver_data;

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
		alarm->time.tm_wday = bcd2bin(buf[REG_OFFSET_ALARM_1_WEEK_DAY] & 0x07) - 1; /*tm_wday: 0-6, rtc: 1-7*/
	else
		alarm->time.tm_mday = bcd2bin(buf[REG_OFFSET_ALARM_1_DATE] & 0x3f);

	dev_info(dev, "%s: alarm: secs=%d, mins=%d, hours=%d, "
		"date=%d, weekday=%d\n",
		__func__,
		alarm->time.tm_sec, alarm->time.tm_min, alarm->time.tm_hour,
		alarm->time.tm_mday, alarm->time.tm_wday);

	return 0;
}

static int ds3231_set_alarm(struct device *dev, struct rtc_wkalrm *alarm)
{
	unsigned char set_data[REG_CNT_FOR_ALARM + 1] = { REG_ADDR_ALARM_1_SEC };
	unsigned char *buf = set_data + 1;
	struct i2c_msg msg = { 0 };
	struct ds3231_state *driver_data;

	driver_data = i2c_get_clientdata(to_i2c_client(dev));
	if (!driver_data) {
		dev_info(dev, "%s: i2c_get_clientdata: error\n", __func__);
		return -EIO;
	}

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
	if (driver_data->alarm_dy_dt_bit) /*tm_wday: 0-6, rtc: 1-7*/
		buf[REG_OFFSET_ALARM_1_WEEK_DAY] = ((bin2bcd(alarm->time.tm_wday) + 1) & 0x07) | \
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

	return 0;
}

#ifdef CONFIG_RTC_DRV_DS3231_ALARM_INTERRUPTS_EN
/*
 * Interface for exporting `alarm_int_enabled` variable
 */
static ssize_t alarm_int_enabled_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int len;
	struct ds3231_state *driver_data;

	driver_data = dev_get_drvdata((const struct device *)dev);
	if (!driver_data) {
		dev_info(dev, "%s: dev_get_drvdata: error\n", __func__);
		return -EIO;
	}

	len = sprintf(buf, "%d\n", driver_data->alarm_int_enabled);
	if (len <= 0) {
		dev_info(dev, "%s: sprintf: error\n", __func__);
		return -EIO;
	}

	return len;
}

static ssize_t alarm_int_enabled_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret;
	struct ds3231_state *driver_data;

	driver_data = dev_get_drvdata((const struct device *)dev);
	if (!driver_data) {
		dev_info(dev, "%s: dev_get_drvdata: error\n", __func__);
		return -EIO;
	}

	ret = kstrtoint(buf, 10, &driver_data->alarm_int_enabled);
	if (ret != 0) {
		dev_info(dev, "%s: kstrtoint: error\n", __func__);
		return -EIO;
	}

	/* conversion to 1 or 0 */
	driver_data->alarm_int_enabled = (int)(!!driver_data->alarm_int_enabled);

	/* avoid continuously write same values to a chip */
	if (driver_data->alarm_int_enabled == \
			driver_data->alarm_int_enabled_old)
		return count;

	/* save new value */
	driver_data->alarm_int_enabled_old = driver_data->alarm_int_enabled;

	/*
	 * Disable on chip interrupts to avoid interrupt during setting
	 * new data to ds3231 alarm registers
	 */
	ret = ds3231_disable_onchip_alarm_detect(to_i2c_client(dev));
	if (ret != 0) {
		dev_info(dev,
			"%s: ds3231_disable_onchip_alarm_detect: error\n",
			__func__);
		return -EIO;
	}

	ret = ds3231_clear_onchip_alarm_flags(to_i2c_client(dev));
	if (ret != 0) {
		dev_info(dev,
			"%s: ds3231_clear_onchip_alarm_flags: error\n",
			__func__);
		return -EIO;
	}

	if (driver_data->alarm_int_enabled) {
		ret = ds3231_set_alarm(dev, &driver_data->alarm_time);
		if (ret != 0) {
			dev_info(dev,
				"%s: ds3231_set_alarm: error\n",
				__func__);
			return -EIO;
		}

		/* read alarm time just for save to internal driver date and print log */
		(void)ds3231_read_alarm(dev, &driver_data->alarm_time);

		ret = ds3231_enable_onchip_alarm_detect(to_i2c_client(dev));
		if (ret != 0) {
			dev_info(dev,
				"%s: ds3231_enable_onchip_alarm_detect: error\n",
				__func__);
			return -EIO;
		}
	}

	return count;
}

static DEVICE_ATTR(alarm_int_enabled, (S_IWUSR | S_IRUGO), \
		alarm_int_enabled_show, alarm_int_enabled_store);

enum weekdays { SUN = 0, MON, TUE, WED,
		THU, FRI, SAT };

enum months { JAN = 0, FEB, MAR, APR, MAY, JUN,
              JUL, AUG, SEP, OCT, NOV, DEC };

#define WEEKDAY_CNT 7
static char wday_name[7][3] = {
	[SUN] = "Sun",
	[MON] = "Mon",
	[TUE] = "Tue",
	[WED] = "Wed",
	[THU] = "Thu",
	[FRI] = "Fri",
	[SAT] = "Sat"
};

static char mon_name[12][3] = {
	[JAN] = "Jan",
	[FEB] = "Feb",
	[MAR] = "Mar",
	[APR] = "Apr",
	[MAY] = "May",
	[JUN] = "Jun",
	[JUL] = "Jul",
	[AUG] = "Aug",
	[SEP] = "Sep",
	[OCT] = "Oct",
	[NOV] = "Nov",
	[DEC] = "Dec"
};

/*
 * Interface for exporting `alarm_time` variable
 */
static ssize_t alarm_time_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int len;
	struct rtc_time *alarm_timeptr;
	struct ds3231_state *driver_data;

	driver_data = dev_get_drvdata((const struct device *)dev);
	if (!driver_data) {
		dev_info(dev, "%s: dev_get_drvdata: error\n", __func__);
		return -EIO;
	}

	alarm_timeptr = &driver_data->alarm_time.time;

	/* convert alarm time to string */
	len = sprintf(buf, "%.3s %.3s%3d %.2d:%.2d:%.2d %d\n",
			wday_name[alarm_timeptr->tm_wday],
			mon_name[alarm_timeptr->tm_mon],
			alarm_timeptr->tm_mday, alarm_timeptr->tm_hour,
			alarm_timeptr->tm_min, alarm_timeptr->tm_sec,
			1900 + alarm_timeptr->tm_year);

	return len;
}

/*
 * Convert input string to rtc_time driver data
 *
 * There is nothing in Linux kernel for parsing time string,
 * that's why I try to do own bicycle
 */
static int parse_time_buffer_to_rtc_struct(const char *buf, struct rtc_time *time)
{
	int i;
	int ret = 0;
	int sec = -1;
	int min = -1;
	int hours = -1;
	int weekday = -1;
	int monthday = -1;
	int cur_pos; /* current position in input buffer */
	int cur_expect_strfield_size = 3; /* size of word of day name <f.e Mon>*/
	char temp[4];

	/* 1. parse weekday */
	cur_pos = 0;
	for (i = 0; i < WEEKDAY_CNT; ++i) {
		if (strncmp(&wday_name[i][0],
				buf + cur_pos,
				cur_expect_strfield_size) == 0) {
			weekday = i; /* value: 0 - 6 */
			break;
		}
	}
	if (weekday == -1)
		goto parse_err;

	/* 2. parse monthday */
	cur_pos = 4;
	cur_expect_strfield_size = 2; /* size of date word <f.e. 23>*/

	memset(temp, 0, sizeof(temp));
	memcpy(temp, buf + cur_pos, cur_expect_strfield_size);
	if (kstrtoint(temp, 10, &monthday) != 0)
		goto parse_err;
	
	/* 3. parse hour */
	cur_pos = 7;
	cur_expect_strfield_size = 2; /* size of hour word <f.e. 15>*/

	memset(temp, 0, sizeof(temp));
	memcpy(temp, buf + cur_pos, cur_expect_strfield_size);
	if (kstrtoint(temp, 10, &hours) != 0)
		goto parse_err;

	/* 4. parse min */
	cur_pos = 10;
	cur_expect_strfield_size = 2; /* size of hour word <f.e. 15>*/

	memset(temp, 0, sizeof(temp));
	memcpy(temp, buf + cur_pos, cur_expect_strfield_size);
	if (kstrtoint(temp, 10, &min) != 0)
		goto parse_err;

	/* 5. parse sec */
	cur_pos = 13;
	cur_expect_strfield_size = 2; /* size of hour word <f.e. 15>*/

	memset(temp, 0, sizeof(temp));
	memcpy(temp, buf + cur_pos, cur_expect_strfield_size);
	if (kstrtoint(temp, 10, &sec) != 0)
		goto parse_err;

	/*
	 * Save time to driver data.
	 * Use it when `1` write to the `alarm_int_enabled` file.
	 */
	time->tm_sec = sec;
	time->tm_min = min;
	time->tm_hour = hours;
	time->tm_wday = weekday;
	time->tm_mday = monthday;

	goto parse_ok;

parse_err:
	ret = -1;

parse_ok:
	return ret;
}

static ssize_t alarm_time_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct ds3231_state *driver_data;
	static char example[] = "<weekday(f.e Wen)> <monthday(f.e 23)> hh:mm:ss";

	driver_data = dev_get_drvdata((const struct device *)dev);
	if (!driver_data) {
		dev_info(dev, "%s: dev_get_drvdata: error\n", __func__);
		return -EIO;
	}

	if (parse_time_buffer_to_rtc_struct(buf, &driver_data->alarm_time.time) != 0) {
		dev_info(dev,
			"%s: parsing time error. Try to format str like: <%s>\n",
			__func__, example);
		return -EIO;
	}

	return count;
}

static DEVICE_ATTR(alarm_time, (S_IWUSR | S_IRUGO), \
		alarm_time_show, alarm_time_store);
#endif

/*
 * ToDo: linux rtc subsystem suports rtc alarms with help ioctl() interface,
 * bit now driver exports two attribute in sysfs to enale/disable
 * rtc alarm interrupts. Try to do more standart interface.
 * See https://linux.die.net/man/4/rtc.
 */
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
		dev_info(&client->dev,
			"%s: unable to get chip status reg\n", __func__);
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
		/* busy more than 1 second means hard error into rtc chip */
		if (while_iterator == 50) {
			dev_info(&client->dev,
				"%s: hard error into ds3231 chip\n", __func__);
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
		dev_info(&client->dev,
			"%s: control error: oscillator stopped\n", __func__);
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
		dev_info(&client->dev,
			"%s: i2c_get_clientdata: error\n", __func__);
		return -EIO;
	}

	if (read_reg(client, REG_ADDR_CONTROL, &control) < 0) {
		dev_info(&client->dev,
			"%s: unable to get chip control reg\n", __func__);
		return -EIO;
	}

	if (read_reg(client, REG_ADDR_STATUS, &status) < 0) {
		dev_info(&client->dev,
			"%s: unable to get chip status reg\n", __func__);
		return -EIO;
	}

	if (ds3231_check_osc_reset_osf_bit(client, &control, &status) < 0) {
		dev_info(&client->dev,
			"%s: ds3231_check_osc_reset_osf_bit failed \n", __func__);
		return -EIO;
	}

	/* need to adjust oscillator with environmental temperature */
	if (ds3231_force_tcxo_set_bit(client, &control) < 0) {
		dev_info(&client->dev,
			"%s: tcxo operation failed \n", __func__);
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
	status &= ~(STATUS_A2F_BIT); /* clear alarm 2 flag */
	status &= ~(STATUS_A1F_BIT); /* clear alarm 1 flag */

#ifdef CONFIG_RTC_DRV_DS3231_32kHZ_OUTPUT_EN
	status  |= STATUS_EN32kHz_BIT;
#endif

	/*
	 * Apply default value.
	 *
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
# endif

# ifdef CONFIG_RTC_DRV_DS3231_ALARM_INTERRUPTS_EN
	control |= CNTRL_INTCN_BIT; /* enable INT */
# endif
#endif

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

	return 0;
}

#ifdef CONFIG_RTC_DRV_DS3231_ALARM_INTERRUPTS_EN

static irqreturn_t alarm_irq_handler(int irq, void *dev)
{
	struct ds3231_state *driver_data;

	driver_data = i2c_get_clientdata(to_i2c_client(dev));
	if (!driver_data)
		return IRQ_NONE;

	up(&driver_data->alarm_interrupt_sem);

	return IRQ_HANDLED;
}

static int setup_cpu_gpio_pin_for_interrupt(struct i2c_client *client)
{
	int ret;
	struct ds3231_state *driver_data;

	gpio_request(ALARM_INT_GPIO, ALARM_INT_GPIO_LABEL);
	gpio_direction_input(ALARM_INT_GPIO);
	gpio_set_debounce(ALARM_INT_GPIO, 10);

	driver_data = i2c_get_clientdata(client);
	if (!driver_data) {
		dev_info(&client->dev,
			"%s: failed to get i2c_get_clientdata\n",
			__func__);
		do_exit(2);
	}

	driver_data->irq = gpio_to_irq(ALARM_INT_GPIO);

	ret = request_irq(driver_data->irq,
			alarm_irq_handler,
			IRQF_TRIGGER_FALLING,
			"ds3231-irq-handler",
			&client->dev);

	if (ret < 0)
		return 1;

	return 0;
}

static int interrupt_handler_thread(void *data)
{
	struct ds3231_state *driver_data;
	struct i2c_client *client;
	struct semaphore *alarm_sem;
	int ret;

	client = (struct i2c_client *)data;
	if (!client) {
		dev_info(&client->dev,
			"%s: failed to i2c client pointer\n",
			__func__);
		do_exit(1);
	}

	driver_data = i2c_get_clientdata(client);
	if (!driver_data) {
		dev_info(&client->dev,
			"%s: failed to get i2c_get_clientdata\n",
			__func__);
		do_exit(2);
	}

	alarm_sem = &driver_data->alarm_interrupt_sem;
	if (!alarm_sem) {
		dev_info(&client->dev,
			"%s: failed to get alarm_sem pointer\n",
			__func__);
		do_exit(3);
	}

	/* duty cycle */
	while (!kthread_should_stop()) {
		/*
		 * If semaphore has been uped in the interrupt, we will
		 * acquire it here, else thread will go to sleep.
		 */
		if (!down_interruptible(alarm_sem)) {
			if (driver_data->task_exit) {
				msleep(20);
				continue;
			}

			/* proccess ds3231 gpio interrupt */
			dev_info(&client->dev, "ds3231 gpio alarm interrupt detect\n");

			ret = ds3231_disable_onchip_alarm_detect(client);
			if (ret != 0)
				dev_info(&client->dev,
					"%s: ds3231_disable_onchip_alarm_detect: error\n",
					__func__);

			ret = ds3231_clear_onchip_alarm_flags(client);
			if (ret != 0)
				dev_info(&client->dev,
					"%s: ds3231_clear_onchip_alarm_flags: error\n",
					__func__);

			/*
			 * Clear sysfs alarm-enabled flag.
			 * If we don't clear the one, we can't
			 * set alarm again.
			 *
			 * ToDo: fix a potential bug
			 * This is a potential critical section,
			 * because the value can change from user space
			 * with help sysfs attribute and here.
			 */
			driver_data->alarm_int_enabled = 0;
			driver_data->alarm_int_enabled_old = 0;
		}
	}

	return 0;
}
#endif

static int ds3231_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
#ifdef CONFIG_RTC_DRV_DS3231_ALARM_INTERRUPTS_EN
	/* ISO C90 forbids mixed declarations and code --> declaring its here */
	int sysfs_ret;
#endif
	struct ds3231_state *driver_data = &ds3231_data;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_info(&client->dev,
			"%s: i2c_check_functionality() error\n",
			__func__);
		return -ENODEV;
	}

	i2c_set_clientdata(client, driver_data);

	if (ds3231_init(client) < 0) {
		dev_info(&client->dev,
			"%s: ds3231_init() error\n",
			__func__);
		return -ENODEV;
	}

	driver_data->rtc = devm_rtc_device_register(&client->dev,
			ds3231_driver.driver.name, &ds3231_rtc_ops, THIS_MODULE);

	if (IS_ERR(driver_data->rtc)) {
		dev_info(&client->dev,
			"%s: devm_rtc_device_register() error\n",
			__func__);
		return PTR_ERR(driver_data->rtc);
	}

#ifdef CONFIG_RTC_DRV_DS3231_ALARM_INTERRUPTS_EN
	/*
	 * Thread waits, when interrupt happens and into
	 * handler we will increment semaphore and thread will wake up.
	 */
	sema_init(&driver_data->alarm_interrupt_sem, 0);

	if (setup_cpu_gpio_pin_for_interrupt(client) != 0) {
		dev_info(&client->dev,
			"%s: setup_cpu_gpio_pin_for_interrupt() error\n",
			__func__);
		return -ENODEV;
	}

	/* create sysfs file to enable/disable alarm */
	sysfs_ret = device_create_file(&client->dev, &dev_attr_alarm_int_enabled);
	if (sysfs_ret) {
		dev_info(&client->dev,
			"%s: device_create_file() int file error\n",
			__func__);
		return -ENODEV;
	}

	/* create sysfs file to read/write alarm time */
	sysfs_ret = device_create_file(&client->dev, &dev_attr_alarm_time);
	if (sysfs_ret) {
		dev_info(&client->dev,
			"%s: device_create_file() alarm_time file error\n",
			__func__);
		return -ENODEV;
	}

	/*
	 * Create kernel thread to handle GPIO alarm interrupt
	 * See https://lwn.net/Articles/65178/
	 *
	 * In IT handler you need reset RTC IT ALARM A1F flag in status register.
	 * When this flag is 1, INT/SQW pin is also asserted.
	 * This flag can only be written to logic 0
	 */
	driver_data->thread = kthread_create(
				interrupt_handler_thread,
				client,
				"rtc-int-handler");
	if (IS_ERR(driver_data->thread)) {
		dev_info(&client->dev,
			"%s: kthread_create() error\n",
			__func__);
		return PTR_ERR(driver_data->thread);
	}

	wake_up_process(driver_data->thread);
#endif

	/*
	 * ToDo: create debug interface to this driver to ability to read chip register
	 * May by with help procfs
	 */

	return 0;
}

static int ds3231_remove(struct i2c_client *client)
{
#ifdef CONFIG_RTC_DRV_DS3231_ALARM_INTERRUPTS_EN
	struct ds3231_state *driver_data;

	driver_data = i2c_get_clientdata(client);
	if (!driver_data) {
		dev_info(&client->dev,
			"%s: failed to get i2c_get_clientdata\n", __func__);
		return -EIO;
	}

	free_irq(driver_data->irq, &client->dev);

	ds3231_disable_onchip_alarm_detect(client);
	ds3231_clear_onchip_alarm_flags(client);

	device_remove_file(&client->dev, &dev_attr_alarm_time);
	device_remove_file(&client->dev, &dev_attr_alarm_int_enabled);

	/* set exit flag to true exit from kernel thread */
	driver_data->task_exit = 1;

	/*
	 * Release the alaram semaphore to return
	 * from down_interruptible() function
	 */
	up(&driver_data->alarm_interrupt_sem);

	/*
	 * After this call kthread_should_stop() in the thread will return TRUE.
	 * See https://lwn.net/Articles/118935/
	 */
	kthread_stop(driver_data->thread);
#endif
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
