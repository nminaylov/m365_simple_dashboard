#ifndef RTC_PCF2129_H_
#define RTC_PCF2129_H_

#include "stm_lib.h"

typedef struct
{
    uint8_t sec;
    uint8_t min;
    uint8_t hour;
} rtc_time_t;

typedef struct
{
    uint8_t month;
    uint8_t day;
    uint8_t year;
    uint8_t wday;
} rtc_date_t;


uint8_t rtc_init(void);
uint8_t rtc_get_time(rtc_time_t * t);
uint8_t rtc_set_time(rtc_time_t * t);
uint8_t rtc_get_date(rtc_date_t * d);
uint8_t rtc_set_date(rtc_date_t * d);

#endif /* RTC_PCF2129_H_ */
