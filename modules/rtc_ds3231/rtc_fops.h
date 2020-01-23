#ifndef __RTC_DS3231_RTC_FOPS_H_
#define __RTC_DS3231_RTC_FOPS_H_

#include <linux/fs.h> /* file operations */

const struct file_operations *rtc_get_fops_implementation(void);

#endif  // __RTC_DS3231_RTC_FOPS_H_
