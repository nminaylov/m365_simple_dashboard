#include "main.h"
#include "printf.h"
#include "lcd.h"

#include <math.h>

void screen_main_draw(void);
void screen_main_update(void);

void draw_capacity(uint16_t cap);
void draw_power(int16_t pow);
void draw_speed(uint16_t spd);

int main(void)
{
    Clock_Init();
    LCD_init();
    RTC_Init();

    LCD_SetBGColor(BLACK);
    LCD_SetTextColor(RED);
    screen_main_draw();

    while (1)
    {
        LL_mDelay(1);
        screen_main_update();
    }
    return(0);
}

#define SPD_TEXT_Y 84

void screen_main_draw(void)
{
    LCD_SetTextColor(WHITE);
    LCD_SetTextPos(0, 0);
    LCD_SetFont(&clock_digits_32x50);
    LCD_printf("02");

    LCD_SetFont(&clock_symbols);
    if (1%2)
        LCD_putchar(1);
    else
        LCD_putchar(0);

    LCD_SetFont(&clock_digits_32x50);
    LCD_printf("36");

    LCD_SetTextPos(0, SPD_TEXT_Y);
    LCD_printf("//");

    LCD_SetFont(&t_12x24_full);
    LCD_SetTextPos(65, SPD_TEXT_Y-4);
    LCD_printf("\xCA\xCC");

    LCD_SetTextPos(71, SPD_TEXT_Y+17);
    LCD_printf("\xD7");

    LCD_Fill(64, SPD_TEXT_Y+17, 26, 2, WHITE);

    LCD_SetTextPos(112+32, SPD_TEXT_Y);
    LCD_SetFont(&clock_digits_32x50);
    LCD_printf("///");

    LCD_SetFont(&t_12x24_full);
    LCD_SetTextPos(104, SPD_TEXT_Y+31);
    LCD_printf("\xC2\xD2");

    LCD_SetTextColor(LGRAY);

    LCD_draw_line(0, 54, LCD_W-1, 54, 1);
    LCD_draw_line(138, 0, 138, 54, 1);

    LCD_draw_line(0, SPD_TEXT_Y+50+4, 69, SPD_TEXT_Y+50+4, 1);
    LCD_draw_line(69, SPD_TEXT_Y+50+4, 69+50+5+4, SPD_TEXT_Y-5, 1);
    LCD_draw_line(69+50+5+4, SPD_TEXT_Y-5, LCD_W-1, SPD_TEXT_Y-5, 1);

    LCD_draw_line(0, 163, LCD_W-1, 163, 1);

    LCD_SetTextColor(WHITE);

    LCD_SetFont(&symbols_m365);
    LCD_SetTextPos(141, 0);
    LCD_putchar(0);

    LCD_SetFont(&symbols_m365);
    LCD_SetTextPos(141, 28);
    LCD_putchar(1);

    LCD_SetFont(&t_12x24_full);
    LCD_SetTextPos(160, 0);
    LCD_printf("----°C");
    LCD_SetTextPos(160, 26);
    LCD_printf("----°C");
}

void screen_main_update(void)
{
    static uint16_t cap = 0;
    static uint16_t col = 0;
    static uint16_t spd = 0;
    static int16_t pow = -300;

    draw_capacity(cap+=10);
    if (cap > 8000)
        cap = 0;
    draw_power(pow++);
    if (pow > 1200)
        pow = -300;
    draw_speed(spd++);
    if (spd > 350)
        spd = 0;

    LCD_SetTextPos(0, SPD_TEXT_Y);
    LCD_SetFont(&clock_digits_32x50);
    LCD_printf("% 2u", spd/10);


    if (pow < 0)
    {
        LCD_SetTextPos(112+2, SPD_TEXT_Y+22);
        LCD_SetFont(&clock_minus_24x5);
        LCD_putchar(1);

        LCD_SetTextPos(112+32, SPD_TEXT_Y);
        LCD_SetFont(&clock_digits_32x50);
        LCD_printf("% 3u", -pow);
    }
    else if (pow < 1000)
    {
        LCD_SetTextPos(112+2, SPD_TEXT_Y+22);
        LCD_SetFont(&clock_minus_24x5);
        LCD_putchar(0);

        LCD_SetTextPos(112+22, SPD_TEXT_Y);
        LCD_SetFont(&clock_symbols);
        LCD_putchar(0);

        LCD_SetTextPos(112+32, SPD_TEXT_Y);
        LCD_SetFont(&clock_digits_32x50);
        LCD_printf("% 3u", pow);
    }
    else // pow > 1000
    {
        LCD_SetTextPos(112+22, SPD_TEXT_Y);
        LCD_SetFont(&clock_symbols);
        LCD_putchar(2);

        LCD_SetTextPos(112+32, SPD_TEXT_Y);
        LCD_SetFont(&clock_digits_32x50);
        LCD_printf("%03u", pow-1000);
    }

}

#define SPD_MAX_VAL 350
#define SPD_H 10
#define SPD_Y 59
#define SPD_BORDER 4
#define SPD_MARK_STEP 50

void draw_speed(uint16_t spd)
{
    if (spd > SPD_MAX_VAL)
        spd = SPD_MAX_VAL;

    uint16_t x_stop = spd * (LCD_W-SPD_BORDER*2) / SPD_MAX_VAL;
    uint16_t mark_period = SPD_MARK_STEP * (LCD_W-SPD_BORDER*2) / SPD_MAX_VAL;
    uint8_t mark_cnt = 0;

    for (uint16_t x = SPD_BORDER; x < LCD_W-SPD_BORDER+1; x+= mark_period)
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
#define POW_Y 149
#define POW_H 10
#define POW_STEP 50
#define POW_STEP_LEN (LCD_W / ((POW_MAX-POW_MIN) / POW_STEP))

void draw_power(int16_t pow)
{
    if (pow < POW_MIN)
        pow = POW_MIN;
    if (pow > POW_MAX)
        pow = POW_MAX;

    LCD_Fill(POW_ZERO_X, POW_Y-5, 1, 4, WHITE);//POW_H+5, WHITE);

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
