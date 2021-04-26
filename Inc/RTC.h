#ifndef RTC_H_
#define RTC_H_

#include "stm_lib.h"

#define RTC_ASYNCH_PREDIV          ((uint32_t)0x7FFF)

typedef struct time_struct
{
	uint8_t sec;
	uint8_t min;
	uint8_t hour;
}time_struct;

typedef struct date_struct
{
	uint8_t month;
	uint8_t day;
	uint8_t year;
}date_struct;

void RTC_Init(void);
void RTC_Get_Time(time_struct * time);
void RTC_Get_Date(date_struct * date);
uint32_t RTC_Get_timestamp(void);
uint8_t RTC_is_Time_passed(uint32_t time_last, uint32_t delay);
uint32_t RTC_get_Time_passed(uint32_t time_last);
void RTC_Set_Time(time_struct * time);
void RTC_Set_Date(date_struct * date);
void RTC_Set_BL_flag(void);


#endif /* RTC_H_ */
