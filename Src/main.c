#include "main.h"
#include "printf.h"
#include "lcd.h"
#include "rtc_pcf2129.h"
#include "m365_uart.h"

#include <math.h>
#include <stdlib.h>

void clock_init(void);

void screen_main_draw(void);
void screen_main_update(void);

void screen_charge_draw(void);
void screen_charge_update(void);

void screen_locked_draw(void);

void draw_capacity(uint16_t cap);
void draw_power(int16_t pow);
void draw_speed(uint16_t spd);

static m365_data_t * m365_data;

int main(void)
{
    uint32_t lock_timeout = 0;
    uint8_t screen_cur = 0;
    clock_init();
    LCD_init();
    rtc_init();

    m365_data = m365_uart_init();

    screen_cur = 0;
    m365_uart_set_req_mode(1);
    screen_main_draw();

    while (1)
    {
        if (m365_data->update_flag)
        {
            m365_data->update_flag = 0;
            if (screen_cur == 0)
                screen_main_update();
            else if (screen_cur == 1)
                screen_charge_update();
        }
        m365_uart_handler();

        if ((m365_data->trottle_pos > 0x60) && (m365_data->brake_pos > 0x60))
        {
            lock_timeout++;
            if ((lock_timeout > 1000000) && (screen_cur != 2))
            {
                screen_cur = 2;
                m365_uart_set_req_mode(0);
                screen_locked_draw();
            }
        }
        else
        {
            lock_timeout = 0;
        }

        if ((m365_data->bms_flags & (1 << 6)) && (screen_cur == 0)) // Charge flag
        {
            if (m365_data->speed == 0)
            {
                screen_cur = 1;
                m365_uart_set_req_mode(2);
                screen_charge_draw();
            }
        }
        else if (!(m365_data->bms_flags & (1 << 6)) && (screen_cur == 1))
        {
            screen_cur = 0;
            m365_uart_set_req_mode(1);
            screen_main_draw();
        }

    }
    return (0);
}

#define SPD_TEXT_Y 84
#define COLOR_VAL       WHITE
#define COLOR_UNITS     DGRAY
#define COLOR_SYMBOL    GREEN
#define COLOR_LINES     DGRAY

void screen_main_draw(void)
{
    LCD_fill(0, 0, LCD_W, LCD_H, BLACK);

    LCD_set_bg_color(BLACK);

    LCD_set_text_color(COLOR_VAL);
    LCD_set_font(&clock_digits_32x50);
    LCD_set_text_pos(0, SPD_TEXT_Y);
    LCD_printf("//");

    LCD_set_text_color(COLOR_UNITS);
    LCD_set_font(&t_12x24_full);
    LCD_set_text_pos(65, SPD_TEXT_Y-4);
    LCD_printf("\xCA\xCC");

    LCD_set_text_pos(71, SPD_TEXT_Y+17);
    LCD_printf("\xD7");

    LCD_fill(64, SPD_TEXT_Y+17, 26, 2, COLOR_UNITS);

    LCD_set_text_color(COLOR_VAL);
    LCD_set_text_pos(112+32, SPD_TEXT_Y);
    LCD_set_font(&clock_digits_32x50);
    LCD_printf("///");

    LCD_set_text_color(COLOR_UNITS);
    LCD_set_font(&t_12x24_full);
    LCD_set_text_pos(104, SPD_TEXT_Y+31);
    LCD_printf("\xC2\xD2");

    LCD_set_text_color(COLOR_LINES);
    LCD_draw_line(0, 54, LCD_W-1, 54, 1);
    LCD_draw_line(138, 0, 138, 54, 1);

    LCD_draw_line(0, SPD_TEXT_Y+50+4, 69, SPD_TEXT_Y+50+4, 1);
    LCD_draw_line(69, SPD_TEXT_Y+50+4, 69+50+5+4, SPD_TEXT_Y-5, 1);
    LCD_draw_line(69+50+5+4, SPD_TEXT_Y-5, LCD_W-1, SPD_TEXT_Y-5, 1);

    LCD_draw_line(0, 163, LCD_W-1, 163, 1);

    LCD_set_text_color(COLOR_SYMBOL);
    LCD_set_font(&symbols_m365);
    LCD_set_text_pos(141, 0);
    LCD_putchar(0);

    LCD_set_font(&symbols_m365);
    LCD_set_text_pos(141, 28);
    LCD_putchar(1);

    LCD_set_text_color(COLOR_UNITS);
    LCD_set_font(&t_12x24_full);
    LCD_set_text_pos(160, 0);
    LCD_printf("    °C");
    LCD_set_text_pos(160, 26);
    LCD_printf("    °C");

    LCD_set_text_color(COLOR_SYMBOL);
    LCD_set_font(&symbols_m365);
    LCD_set_text_pos(0, 167);
    LCD_putchar(3);
    LCD_set_text_pos(120, 167);
    LCD_putchar(4);
    LCD_set_text_pos(0, 192);
    LCD_putchar(2);

    LCD_set_font(&t_12x24_full);
    LCD_set_text_color(COLOR_UNITS);
    LCD_set_text_pos(15, 167);
    LCD_printf("     KM");
    LCD_set_text_pos(135, 167);
    LCD_printf("      KM");

    LCD_set_text_pos(15, 192);
    LCD_printf("   %%     mAh     B");
}

void screen_main_update(void)
{
    static rtc_time_t time;
    rtc_get_time(&time);

    LCD_set_text_color(COLOR_VAL);
    LCD_set_text_pos(0, 0);

    LCD_set_font(&clock_digits_32x50);
    LCD_printf("%02u", time.hour);
    LCD_set_font(&clock_symbols);
    if (time.sec%2)
        LCD_putchar(1);
    else
        LCD_putchar(0);
    LCD_set_font(&clock_digits_32x50);
    LCD_printf("%02u", time.min);

    int32_t pow = (m365_data->voltage * m365_data->current) / 10000;

    int32_t spd = m365_data->speed;
    if (spd < 0)
        spd = 0;

    draw_capacity(m365_data->bms_mah);
    draw_power(pow);
    draw_speed(spd/100);

    LCD_set_text_pos(0, SPD_TEXT_Y);
    LCD_set_text_color(COLOR_VAL);
    LCD_set_font(&clock_digits_32x50);
    LCD_printf("% 2u", spd/1000);

    if (pow < 0)
    {
        LCD_set_text_pos(112+2, SPD_TEXT_Y+22);
        LCD_set_font(&clock_minus_24x5);
        LCD_putchar(1);

        LCD_set_text_pos(112+32, SPD_TEXT_Y);
        LCD_set_font(&clock_digits_32x50);
        LCD_printf("% 3u", -pow);
    }
    else if (pow < 1000)
    {
        LCD_set_text_pos(112+2, SPD_TEXT_Y+22);
        LCD_set_font(&clock_minus_24x5);
        LCD_putchar(0);

        LCD_set_text_pos(112+22, SPD_TEXT_Y);
        LCD_set_font(&clock_symbols);
        LCD_putchar(0);

        LCD_set_text_pos(112+32, SPD_TEXT_Y);
        LCD_set_font(&clock_digits_32x50);
        LCD_printf("% 3u", pow);
    }
    else // pow > 1000
    {
        LCD_set_text_pos(112+22, SPD_TEXT_Y);
        LCD_set_font(&clock_symbols);
        LCD_putchar(2);

        LCD_set_text_pos(112+32, SPD_TEXT_Y);
        LCD_set_font(&clock_digits_32x50);
        LCD_printf("%03u", pow-1000);
    }

    LCD_set_font(&t_12x24_full);
    LCD_set_text_color(COLOR_VAL);

    LCD_set_text_pos(15, 167);
    LCD_printf("% 2u.%02u", m365_data->trip/100, m365_data->trip%100);

    LCD_set_text_pos(135, 167);
    LCD_printf("% 4u.%1u", m365_data->odo/1000, m365_data->odo/100%10);

    LCD_set_text_pos(15, 192);
    LCD_printf("% 3u", m365_data->bms_percent);

    LCD_set_text_pos(15+5*12, 192);
    LCD_printf("% 4u", m365_data->bms_mah);

    LCD_set_text_pos(15+5*12+8*12, 192);
    LCD_printf("%2u.%1u", m365_data->voltage/100, m365_data->voltage/10%10);

    LCD_set_text_pos(165, 0);
    LCD_printf("% 2d", m365_data->esc_temp/10);

    int16_t bat_t = ((m365_data->bms_temp[0]-20) + (m365_data->bms_temp[1]-20))/2;
    LCD_set_text_pos(165, 26);
    LCD_printf("% 2d", bat_t);
}

void screen_charge_draw(void)
{
    LCD_fill(0, 0, LCD_W, LCD_H, BLACK);

    LCD_set_text_color(COLOR_LINES);

    LCD_draw_line(0, 27, LCD_W-1, 27, 1);
    LCD_draw_line(0, 164, LCD_W-1, 164, 1);

    LCD_set_font(&t_12x24_full);

    for (uint8_t i = 0; i < 5; i++)
    {
        LCD_set_text_pos(0, 44+i*20);
        LCD_set_text_color(COLOR_SYMBOL);
        LCD_printf("% 2u:", i+1);
        LCD_set_text_color(COLOR_UNITS);
        LCD_printf("     B");
    }
    for (uint8_t i = 5; i < 10; i++)
    {
        LCD_set_text_pos(120, 44+(i-5)*20);
        LCD_set_text_color(COLOR_SYMBOL);
        LCD_printf("% 2u:", i+1);
        LCD_set_text_color(COLOR_UNITS);
        LCD_printf("     B");
    }

    LCD_set_text_color(COLOR_UNITS);

    LCD_set_text_pos(0, 2);
    LCD_printf("      B       A");

    LCD_set_text_pos(0, 167);
    LCD_printf("    /   °C");

    LCD_set_text_pos(0, 192);
    LCD_printf("    %%      mAh");
}

void screen_charge_update(void)
{
    draw_capacity(m365_data->bms_mah);

    LCD_set_text_color(COLOR_VAL);
    for (uint8_t i = 0; i < 5; i++)
    {
        if (m365_data->bms_balance_flags & (1 << i))
            LCD_set_text_color(RED);
        else
            LCD_set_text_color(WHITE);
        LCD_set_text_pos(0+36, 44+i*20);
        LCD_printf("%1u.%03u", m365_data->bms_cell_voltage[i]/1000, m365_data->bms_cell_voltage[i]%1000);
    }
    for (uint8_t i = 5; i < 10; i++)
    {
        if (m365_data->bms_balance_flags & (1 << i))
            LCD_set_text_color(RED);
        else
            LCD_set_text_color(WHITE);
        LCD_set_text_pos(120+36, 44+(i-5)*20);
        LCD_printf("%1u.%03u", m365_data->bms_cell_voltage[i]/1000, m365_data->bms_cell_voltage[i]%1000);
    }

    LCD_set_text_color(COLOR_VAL);

    LCD_set_text_pos(12, 2);
    LCD_printf("% 2u.%02u", m365_data->bms_voltage/100, m365_data->bms_voltage%100);

    uint16_t cur_temp = abs(m365_data->bms_current);
    LCD_set_text_pos(108, 2);
    LCD_printf("% 2u.%02u", cur_temp/100, cur_temp%100);

    LCD_set_text_pos(12, 167);
    LCD_printf("% 2d", m365_data->bms_temp[0]-20);
    LCD_set_text_pos(60, 167);
    LCD_printf("% 2d", m365_data->bms_temp[1]-20);

    LCD_set_text_pos(12, 192);
    LCD_printf("% 3u", m365_data->bms_percent);
    LCD_set_text_pos(84, 192);
    LCD_printf("% 4u", m365_data->bms_mah);
}

void screen_locked_draw(void)
{
    LCD_fill(0, 0, LCD_W, LCD_H, BLACK);
    LCD_set_bg_color(BLACK);
    LCD_set_text_color(RED);
    LCD_draw_line(40, 40, 200, 200, 5);
    LCD_draw_line(40, 200, 200, 40, 5);
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
            LCD_fill(x, SPD_Y+SPD_H+1, 1, 4, WHITE);
        else
            LCD_fill(x, SPD_Y+SPD_H+1, 1, 1, WHITE);
        mark_cnt++;
    }

    for (uint16_t x = 0; x < x_stop; x++)
    {
        LCD_fill(x+SPD_BORDER, SPD_Y, 1, SPD_H, BLUE);
    }
    LCD_fill(x_stop+SPD_BORDER, SPD_Y, LCD_W-SPD_BORDER*2-x_stop, SPD_H, BLACK);
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
        LCD_fill(x-1, LCD_H-CAP_H-3, 1, 2, WHITE);
    }

    for (uint8_t x = 0; x < x_stop; x++)
    {
        if ((x+1) % (LCD_W/CAP_STEP_NB) == 0)
            LCD_fill(x, LCD_H-CAP_H, 1, CAP_H, BLACK);
        else
            LCD_fill(x, LCD_H-CAP_H, 1, CAP_H, color);
    }
    LCD_fill(x_stop, LCD_H-CAP_H, LCD_W-x_stop, CAP_H, BLACK);
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

    LCD_fill(POW_ZERO_X, POW_Y-5, 1, 4, WHITE);//POW_H+5, WHITE);

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
                LCD_fill(x+POW_ZERO_X+2, POW_Y, 1, POW_H, BLACK);
            else
                LCD_fill(x+POW_ZERO_X+2, POW_Y, 1, POW_H, color);
        }
        LCD_fill(x_stop+POW_ZERO_X+2, POW_Y, LCD_W-x_stop-POW_ZERO_X-2, POW_H, BLACK);
        LCD_fill(0, POW_Y, POW_ZERO_X-1, POW_H, BLACK);
    }
    else
    {
        uint16_t x_stop = -pow * POW_STEP_LEN / POW_STEP;
        for (uint8_t x = 0; x < x_stop; x++)
        {
            if ((x+1) % POW_STEP_LEN == 0)
                LCD_fill(POW_ZERO_X-x-2, POW_Y, 1, POW_H, BLACK);
            else
                LCD_fill(POW_ZERO_X-x-2, POW_Y, 1, POW_H, CYAN);
        }
        LCD_fill(0, POW_Y, POW_ZERO_X-x_stop, POW_H, BLACK);
        LCD_fill(POW_ZERO_X+2, POW_Y, LCD_W-POW_ZERO_X-2, POW_H, BLACK);
    }
}


void clock_init(void)
{
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_1);
    LL_FLASH_EnablePrefetch();

    LL_RCC_HSI_Enable();
    while (LL_RCC_HSI_IsReady() != 1);

    LL_RCC_HSI14_Enable();
    while (LL_RCC_HSI14_IsReady() != 1);

    LL_RCC_LSI_Enable();
    while (LL_RCC_LSI_IsReady() != 1);

    LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSI_DIV_2, LL_RCC_PLL_MUL_12);
    LL_RCC_PLL_Enable();
    while (LL_RCC_PLL_IsReady() != 1);

    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
    while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL);

    LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_SYSCFG);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

    LL_Init1msTick(48000000);
    LL_SYSTICK_SetClkSource(LL_SYSTICK_CLKSOURCE_HCLK);
    LL_SetSystemCoreClock(48000000);

    LL_RCC_HSI14_EnableADCControl();
    LL_RCC_SetUSARTClockSource(LL_RCC_USART1_CLKSOURCE_PCLK1);

//  NVIC_SetPriority(SysTick_IRQn, 0);
//  NVIC_EnableIRQ(SysTick_IRQn);

    // Тактирование портов
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOF);
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
	//while(1);
	NVIC_SystemReset();
}
