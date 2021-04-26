#include "RTC.h"

void RTC_TIME_structUpadate(void);
void RTC_DATE_structUpdate(void);

time_struct RTC_TimeStruct;
date_struct RTC_DateStruct;

const uint8_t EndOfMonth[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

volatile uint32_t timeCounter = 0;
uint8_t dateUpdate = 0;
uint8_t timeUpdate = 0;

void RTC_Init(void)
{
	/* Ќастраиваем тактирование RTC */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_BKP);

	LL_PWR_EnableBkUpAccess();

	if (LL_RCC_LSE_IsReady() != 1)
	{
		LL_RCC_ForceBackupDomainReset();
		LL_RCC_ReleaseBackupDomainReset();
		LL_RCC_LSE_Enable();
		while (LL_RCC_LSE_IsReady() != 1);
	}

	if (LL_RCC_GetRTCClockSource() != LL_RCC_RTC_CLKSOURCE_LSE)
		LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSE);

	/* Ќастраиваем RTC */
	LL_RCC_EnableRTC();
	LL_RTC_DisableWriteProtection(RTC);
	LL_RTC_EnterInitMode(RTC);
	LL_RTC_SetAsynchPrescaler(RTC, RTC_ASYNCH_PREDIV);

	LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_17);
	LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINE_17);

	NVIC_SetPriority(RTC_IRQn, 0);
	NVIC_EnableIRQ(RTC_IRQn);

	if (LL_RTC_BKP_GetRegister(BKP, LL_RTC_BKP_DR10) == 0xAA55)
	{
		timeCounter = LL_RTC_TIME_Get(RTC);
		timeUpdate = 1;
		RTC_DateStruct.day = 1;
		RTC_DateStruct.month = 1;
		RTC_DateStruct.year = 19; // TODO: обновление даты
	}
	else
	{
		RTC_TimeStruct.hour = 0;
		RTC_TimeStruct.min = 0;
		RTC_TimeStruct.sec = 0;
		LL_RTC_TIME_Set(RTC,((RTC_TimeStruct.hour * 3600) + (RTC_TimeStruct.min * 60) + RTC_TimeStruct.sec));
		RTC_DateStruct.day = 1;
		RTC_DateStruct.month = 1;
		RTC_DateStruct.year = 19;
		LL_RTC_BKP_SetRegister(BKP, LL_RTC_BKP_DR10, 0xAA55);
	}


	LL_RTC_EnableIT_SEC(RTC);

	LL_RTC_ExitInitMode(RTC);
	LL_RTC_EnableWriteProtection(RTC);
}

void RTC_Get_Time(time_struct * time)
{
	RTC_TIME_structUpadate();
	time->hour = RTC_TimeStruct.hour;
	time->min = RTC_TimeStruct.min;
	time->sec = RTC_TimeStruct.sec;
}

void RTC_Get_Date(date_struct * date)
{
	RTC_TIME_structUpadate();
	RTC_DATE_structUpdate();
	date->day = RTC_DateStruct.day;
	date->month = RTC_DateStruct.month;
	date->year = RTC_DateStruct.year;
}

inline uint32_t RTC_Get_timestamp(void)
{
	return timeCounter;
}

uint8_t RTC_is_Time_passed(uint32_t time_last, uint32_t delay)
{
	int32_t diff = (int32_t)(timeCounter - time_last);
	if (diff < 0)
		diff += 0x0001517F;

	if (diff >= delay)
		return 1;
	return 0;
}

uint32_t RTC_get_Time_passed(uint32_t time_last)
{
	int32_t diff = (int32_t)(timeCounter - time_last);
	if (diff < 0)
		diff += 0x0001517F;

	return (uint32_t)diff;
}

void RTC_Set_Time(time_struct * time)
{
	RTC_TimeStruct.hour = time->hour;
	RTC_TimeStruct.min = time->min;
	RTC_TimeStruct.sec = time->sec;

	LL_RTC_DisableWriteProtection(RTC);
	LL_RTC_EnterInitMode(RTC);
	LL_RTC_TIME_Set(RTC,((RTC_TimeStruct.hour * 3600) + (RTC_TimeStruct.min * 60) + RTC_TimeStruct.sec));
	LL_RTC_WaitForSynchro(RTC);
	LL_RTC_ExitInitMode(RTC);
	LL_RTC_EnableWriteProtection(RTC);
}

void RTC_Set_Date(date_struct * date)
{
	RTC_DateStruct.day = date->day;
	RTC_DateStruct.month = date->month;
	RTC_DateStruct.year = date->year;
}

void RTC_Set_BL_flag(void)
{
	LL_RTC_DisableWriteProtection(RTC);
	LL_RTC_BKP_SetRegister(BKP, LL_RTC_BKP_DR4, 0x424C);
	LL_RTC_EnableWriteProtection(RTC);
}

void RTC_DATE_structUpdate(void)
{
	if (dateUpdate != 0U)
	{
		dateUpdate = 0;
		if (RTC_DateStruct.day == EndOfMonth[RTC_DateStruct.month - 1U])
		{
			RTC_DateStruct.day = 1U;
			if (RTC_DateStruct.month == 12U)
			{
				RTC_DateStruct.month = 1U;
				RTC_DateStruct.year += 1U; // TODO: добавить определение високосного года
			}
			else
			{
				RTC_DateStruct.month += 1U;
			}
		}
		else
		{
			RTC_DateStruct.day = RTC_DateStruct.day + 1U;
		}
	}
}

void RTC_TIME_structUpadate(void)
{
	if (timeUpdate != 0)
	{
		timeUpdate = 0;
		RTC_TimeStruct.hour = (timeCounter / 3600) % 24;
		RTC_TimeStruct.min = (timeCounter % 3600) / 60;
		RTC_TimeStruct.sec = (timeCounter % 3600) % 60;
	}
}

void RTC_IRQHandler(void)
{
	if (LL_RTC_IsEnabledIT_SEC(RTC) != 0)
	{
		LL_RTC_ClearFlag_SEC(RTC);

		timeCounter = LL_RTC_TIME_Get(RTC);
		timeUpdate = 1;
		if (timeCounter >= 0x0001517FU)
		{
			dateUpdate = 1;
			LL_RTC_DisableWriteProtection(RTC);
			LL_RTC_EnterInitMode(RTC);
			LL_RTC_TIME_Set(RTC, 0x0U);
			LL_RTC_WaitForSynchro(RTC);
			LL_RTC_ExitInitMode(RTC);
			LL_RTC_EnableWriteProtection(RTC);
		}
		LL_RTC_WaitForSynchro(RTC);
	}
	LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_17);
}
