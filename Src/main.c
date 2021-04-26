#include "main.h"
#include "printf.h"
#include "lcd.h"

#include <math.h>

void Clock_Init(void);

void draw_capacity(uint16_t cap);
void draw_power(int16_t pow);
void draw_speed(uint16_t spd);

int main(void)
{
	Clock_Init();
	LCD_init();
	RTC_Init();
	//time_last = RTC_Get_timestamp();

    uint16_t cap = 0;
    uint16_t spd = 0;
    int16_t pow = -300;

	while (1)
	{
        LL_mDelay(10);

        draw_capacity(cap+=10);
        if (cap > 8000)
            cap = 0;
        draw_power(pow++);
        if (pow > 1000)
            pow = -300;
        //draw_speed(299);
        draw_speed(spd++);
        if (spd > 300)
            spd = 0;
	}
}

#define SPD_MAX_VAL 300
#define SPD_H 15
#define SPD_Y 150
#define SPD_BORDER 20
#define SPD_MARK_STEP 50

void draw_speed(uint16_t spd)
{
    if (spd > SPD_MAX_VAL)
        spd = SPD_MAX_VAL;

    uint16_t x_stop = spd * (LCD_W-SPD_BORDER*2) / SPD_MAX_VAL;
    uint16_t mark_period = SPD_MARK_STEP * (LCD_W-SPD_BORDER*2) / SPD_MAX_VAL;
    uint8_t mark_cnt = 0;

    for (uint16_t x = SPD_BORDER; x < LCD_W-SPD_BORDER; x+= mark_period)
    {
        if (mark_cnt % 2 == 0)
            LCD_Fill(x, SPD_Y+SPD_H+1, 1, 4, WHITE);
        else
            LCD_Fill(x, SPD_Y+SPD_H+1, 1, 1, WHITE);
        mark_cnt++;
    }

    for (uint16_t x = 0; x < x_stop; x++)
    {
        LCD_Fill(x+SPD_BORDER, SPD_Y, 1, SPD_H, BLUE);
    }
    LCD_Fill(x_stop+SPD_BORDER, SPD_Y, LCD_W-SPD_BORDER*2-x_stop, SPD_H, BLACK);
}

#define CAP_STEP_VAL 500
#define CAP_MAX_VAL 8000
#define CAP_STEP_NB (CAP_MAX_VAL / CAP_STEP_VAL)
#define CAP_H 15
#define CAP_MARK_STEP 1000

void draw_capacity(uint16_t cap)
{
    if (cap > CAP_MAX_VAL)
        cap = CAP_MAX_VAL;

    uint16_t color = 0;
    if (cap < 2000)
        color = RED;
    else if (cap < 4000)
        color = YELLOW;
    else
        color = GREEN;

    uint16_t x_stop = cap * LCD_W / CAP_MAX_VAL;
    uint16_t mark_period = CAP_MARK_STEP * LCD_W / CAP_MAX_VAL;

    for (uint8_t x = mark_period; x < LCD_W; x+= mark_period)
    {
        LCD_Fill(x-1, LCD_H-CAP_H-3, 1, 2, WHITE);
    }

    for (uint8_t x = 0; x < x_stop; x++)
    {
        if ((x+1) % (LCD_W/CAP_STEP_NB) == 0)
            LCD_Fill(x, LCD_H-CAP_H, 1, CAP_H, BLACK);
        else
            LCD_Fill(x, LCD_H-CAP_H, 1, CAP_H, color);
    }
    LCD_Fill(x_stop, LCD_H-CAP_H, LCD_W-x_stop, CAP_H, BLACK);
}

#define POW_MIN -300
#define POW_MAX 1000
#define POW_ZERO_X (-POW_MIN*LCD_W/(POW_MAX-POW_MIN))
#define POW_Y 200
#define POW_H 10
#define POW_STEP 50
#define POW_STEP_LEN (LCD_W / ((POW_MAX-POW_MIN) / POW_STEP))

void draw_power(int16_t pow)
{
    if (pow < POW_MIN)
        pow = POW_MIN;
    if (pow > POW_MAX)
        pow = POW_MAX;

    LCD_Fill(POW_ZERO_X, POW_Y-2, 1, POW_H+4, WHITE);

    if (pow >= 0)
    {
        uint16_t x_stop = pow * POW_STEP_LEN / POW_STEP;
        for (uint8_t x = 0; x < x_stop; x++)
        {
            uint16_t color = 0;
            if (x < (250 * POW_STEP_LEN / POW_STEP))
                color = GREEN;
            else if (x < (500 * POW_STEP_LEN / POW_STEP))
                color = YELLOW;
            else
                color = RED;
            if ((x+1) % POW_STEP_LEN == 0)
                LCD_Fill(x+POW_ZERO_X+2, POW_Y, 1, POW_H, BLACK);
            else
                LCD_Fill(x+POW_ZERO_X+2, POW_Y, 1, POW_H, color);
        }
        LCD_Fill(x_stop+POW_ZERO_X+2, POW_Y, LCD_W-x_stop-POW_ZERO_X-2, POW_H, BLACK);
        LCD_Fill(0, POW_Y, POW_ZERO_X-1, POW_H, BLACK);
    }
    else
    {
        uint16_t x_stop = -pow * POW_STEP_LEN / POW_STEP;
        for (uint8_t x = 0; x < x_stop; x++)
        {
            if ((x+1) % POW_STEP_LEN == 0)
                LCD_Fill(POW_ZERO_X-x-2, POW_Y, 1, POW_H, BLACK);
            else
                LCD_Fill(POW_ZERO_X-x-2, POW_Y, 1, POW_H, CYAN);
        }
        LCD_Fill(0, POW_Y, POW_ZERO_X-x_stop, POW_H, BLACK);
        LCD_Fill(POW_ZERO_X+2, POW_Y, LCD_W-POW_ZERO_X-2, POW_H, BLACK);
    }
}


void Clock_Init(void)
{
	LL_RCC_HSI_Enable(); // На всякий случай
	while (LL_RCC_HSI_IsReady() != 1);
	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI); // Стартуем от HSI
	while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_HSI);
	LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);

	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_AFIO);
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

	LL_FLASH_EnablePrefetch(); // Настраиваем флешь
	LL_FLASH_SetLatency(LL_FLASH_LATENCY_2);
	while (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_2);

	LL_RCC_HSE_Enable(); // Теперь можно переходить на HSE+PLL
	while (LL_RCC_HSE_IsReady() != 1);

	LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE_DIV_1, LL_RCC_PLL_MUL_9);
	LL_RCC_PLL_Enable();
	while (LL_RCC_PLL_IsReady() != 1);

	LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
	LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_2);
	LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);

	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
	while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL);

	LL_SetSystemCoreClock(72000000);
	LL_SYSTICK_SetClkSource(LL_SYSTICK_CLKSOURCE_HCLK);
	LL_Init1msTick(72000000);

	NVIC_SetPriority(SysTick_IRQn,
			NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));

	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOA);
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOB);
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOC);
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOD);

	LL_GPIO_AF_Remap_SWJ_NOJTAG();
}

void UART_Init(void)
{
	LL_USART_InitTypeDef USART_InitStruct;
	LL_GPIO_InitTypeDef GPIO_InitStruct;

	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART3);

	GPIO_InitStruct.Pin = LL_GPIO_PIN_10;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = LL_GPIO_PIN_11;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_FLOATING;
	LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	USART_InitStruct.BaudRate = 115200;
	USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
	USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
	USART_InitStruct.Parity = LL_USART_PARITY_NONE;
	USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
	USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
	LL_USART_Init(USART3, &USART_InitStruct);

	LL_USART_ConfigAsyncMode(USART3);
	LL_USART_Enable(USART3);
}

void _putchar(char character)
{
#ifdef _DEBUG_PRINTF_ENABLED
	while (!LL_USART_IsActiveFlag_TXE(USART3));
	LL_USART_TransmitData8(USART3,character);
#endif
}

void HardFault_Handler(void)
{
	while(1);
	NVIC_SystemReset();
}
