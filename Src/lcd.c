/* Функции для работы с дисплеем 240x240 (ST7789) */

#include "lcd.h"
#include <math.h>
#include <stdlib.h>
#include <stdarg.h>
#include "printf.h"

static uint16_t BGColor = WHITE;
static uint16_t TextColor = BLACK;

static uint16_t text_x = 0;
static uint16_t text_y = 0;
static uint16_t text_x_start = 0;

static lcd_font_t * font;

static void LCD_hw_init(void);
static void LCD_send_cmd(uint8_t val);
static void LCD_send_data(uint8_t val);
static void LCD_send_pixel(uint16_t data);
static void LCD_set_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
static void LCD_send_char(uint8_t * start);

void LCD_init(void)
{
    LCD_hw_init();
    LCD_RST_DN; // Дергаем ресет
    LL_mDelay(10);
    LCD_RST_UP;
    LL_mDelay(50);

    LCD_send_cmd(0x01); // SWreset
    LL_mDelay(120);
    LCD_send_cmd(0x11); // Sleep Out
    LL_mDelay(5);

    LCD_send_cmd(0x3A);
    LCD_send_data(0x55);
    LL_mDelay(1);

    LCD_send_cmd(0x2A);
    LCD_send_data(0x00);
    LCD_send_data(0x00);
    LCD_send_data(0x00);
    LCD_send_data(0xF0);

    LCD_send_cmd(0x2B);
    LCD_send_data(0x00);
    LCD_send_data(0x00);
    LCD_send_data(0x00);
    LCD_send_data(0xF0);

    LCD_send_cmd(0x21);
    LL_mDelay(1);

    LCD_send_cmd(0x13);
    LL_mDelay(1);

    LCD_send_cmd(0x29);
    LL_mDelay(1);

    LCD_send_cmd(0x36);
    LCD_send_data(0xC0);
    LL_mDelay(1);

    LCD_fill(0, 0, LCD_W, LCD_H, 0x00);
    LL_mDelay(1);
}

void LCD_sleep(void)
{
    LCD_send_cmd(0x10);
    LL_mDelay(5);
}

static void LCD_hw_init(void) // Настраиваем пины, SPI
{
    LL_SPI_InitTypeDef SPI_InitStruct;
    LL_GPIO_InitTypeDef GPIO_InitStruct;

    LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_SPI1);

    LCD_DC_DN;
    GPIO_InitStruct.Pin = LL_GPIO_PIN_6; // DC
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_1;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_1; // RST
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_5 | LL_GPIO_PIN_7; // SCL, SDA
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    SPI_InitStruct.TransferDirection = LL_SPI_FULL_DUPLEX;
    SPI_InitStruct.Mode = LL_SPI_MODE_MASTER;
    SPI_InitStruct.DataWidth = LL_SPI_DATAWIDTH_8BIT;
    SPI_InitStruct.ClockPolarity = LL_SPI_POLARITY_HIGH;
    SPI_InitStruct.ClockPhase = LL_SPI_PHASE_2EDGE;
    SPI_InitStruct.NSS = LL_SPI_NSS_SOFT;
    SPI_InitStruct.BaudRate = LL_SPI_BAUDRATEPRESCALER_DIV2;
    SPI_InitStruct.BitOrder = LL_SPI_MSB_FIRST;
    SPI_InitStruct.CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE;
    SPI_InitStruct.CRCPoly = 7;
    LL_SPI_Init(SPI1, &SPI_InitStruct);

    LL_SPI_DisableNSSPulseMgt(SPI1);
    LL_SPI_Enable(SPI1);
}

static void LCD_send_cmd(uint8_t val)
{
    while (!SPI1_TXE); //wait buffer empty
    LCD_DC_DN; // A0 = 0 - CMD
    *(uint8_t *)&SPI1->DR = val;
    while (SPI1_BSY); //wait finish sending
}

static void LCD_send_data(uint8_t val)
{
    while (!SPI1_TXE); //wait buffer empty
    LCD_DC_UP; // A0 = 1 - DATA
    *(uint8_t *)&SPI1->DR = val & 0xFF;
    while (SPI1_BSY); //wait finish sending
}

static void LCD_send_pixel(uint16_t data) // Шлем 2 байта (данные)
{
    while (!SPI1_TXE); //wait buffer empty
//    *(uint8_t *)&SPI1->DR = (data >> 8);
//    while (!SPI1_TXE); //wait buffer empty
//    *(uint8_t *)&SPI1->DR = (data & 0xFF);
    SPI1->DR = __REV16(data); // Пользуемся тем, что на 030 можно отправить по 2 байта за раз
}

// Установка окна x, y - координаты начала; w, h - ширина, высота окна
static void LCD_set_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    uint16_t y_t = y;
    LCD_send_cmd(0x2A); // CASET
    LCD_send_data(0x00);
    LCD_send_data(x);
    LCD_send_data(0x00);
    LCD_send_data(x + w - 1);
    LCD_send_cmd(0x2B); // RASET

    y_t += 80;
    LCD_send_data(y_t >> 8);
    LCD_send_data(y_t & 0xFF);
    LCD_send_data((y_t+h-1) >> 8);
    LCD_send_data((y_t+h-1) & 0xFF);

    LCD_send_cmd(0x2C); // RAMWR
}

// Заполняем область цветом
void LCD_fill(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    uint16_t count = w * h;
    LCD_set_window(x, y, w, h);
    LCD_DC_UP;
    while (count--)
        LCD_send_pixel(color);
    while (SPI1_BSY);
}

void LCD_putchar(char chr)
{
    LCD_set_window(text_x, text_y, font->char_w, font->char_h);
    LCD_DC_UP;
    if (chr == '\n') // Если символ переноса
    {
#ifdef  SAVE_X_OFFSET
        text_x = text_x_start;
#else
        text_x = 0;
#endif
        text_y += font->char_h;
        chr = 0;
    }
    else
    {
        text_x += font->char_w;
        if (text_x > (LCD_W - font->char_w))
        {
#ifdef  SAVE_X_OFFSET
            text_x = text_x_start;
#else
            text_x = 0;
#endif
            text_y += font->char_h;
        }
    }
    if (text_y > (LCD_H - font->char_h))
        text_y = 0;

    if (chr < font->offset)
        chr = font->offset;

    uint8_t * chardata = NULL;

    if (font->type == 0)
    {
        if (chr < font->offset)
            chr = font->offset;
        chardata = (uint8_t *) &(font->data[(font->char_w * font->char_h / 8) * (chr - font->offset)]);
    }
    if (font->type == 1)
    {
        if (chr < font->offset)
            chr = font->offset;
        chardata = (uint8_t *) &(font->data[(font->char_w - 1) * (chr - font->offset)]);
    }
    LCD_send_char(chardata);
    while (SPI1_BSY);
}

static void LCD_send_char(uint8_t * start)
{
    if (font->type == 0)
    {
        uint8_t len = font->char_w * font->char_h / 8;
        for (uint8_t byte_cnt = 0; byte_cnt < len; byte_cnt++)
        {
            for (uint8_t bit_cnt = 0; bit_cnt < 8; bit_cnt++)
            {
                if ((start[byte_cnt] << bit_cnt) & 0x80)
                    LCD_send_pixel(TextColor);
                else
                    LCD_send_pixel(BGColor);
            }
        }
    }
    if (font->type == 1)
    {
        for (uint8_t bit_cnt = 0; bit_cnt < 8; bit_cnt++)
        {
            for (uint8_t byte_cnt = 0; byte_cnt < (font->char_w - 1); byte_cnt++)
            {
                if ((start[byte_cnt] >> bit_cnt) & 0x01)
                    LCD_send_pixel(TextColor);
                else
                    LCD_send_pixel(BGColor);
            }
            LCD_send_pixel(BGColor);
        }
    }
}

// Вывод монохромного изображения
void LCD_draw_image_mono(lcd_image_t img, uint16_t x, uint16_t y)
{
    LCD_set_window(x, y, img.width, img.height);
    LCD_DC_UP;
    for (uint16_t byte_cnt = 0; byte_cnt < img.width * img.width / 8; byte_cnt++)
    {
        for (uint8_t bit_cnt = 0; bit_cnt < 8; bit_cnt++)
        {
            if ((img.data[byte_cnt] << bit_cnt) & 0x80)
                LCD_send_pixel(TextColor);
            else
                LCD_send_pixel(BGColor);
        }
    }
    while (SPI1_BSY);
}

void LCD_set_pixel(uint16_t x, uint16_t y, uint16_t color) // Установка одного пикселя (медленно!)
{
    LCD_set_window(x, y, 1, 1);
    LCD_DC_UP;
    LCD_send_pixel(color);
    while (SPI1_BSY);
}

void LCD_draw_circle(int16_t x0, int16_t y0, int16_t radius)
{
    int x = 0;
    int y = radius;
    int delta = 1 - 2 * radius;
    int error = 0;

    while (y >= 0) {
        LCD_draw_line(x0 + x, y0 - y, x0 + x, y0 + y, 1);
        LCD_draw_line(x0 - x, y0 - y, x0 - x, y0 + y, 1);
        error = 2 * (delta + y) - 1;

        if (delta < 0 && error <= 0) {
            ++x;
            delta += 2 * x + 1;
            continue;
        }

        error = 2 * (delta - x) - 1;

        if (delta > 0 && error > 0) {
            --y;
            delta += 1 - 2 * y;
            continue;
        }

        ++x;
        delta += 2 * (x - y);
        --y;
    }
}

void LCD_draw_line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t thick)
{
    const int16_t deltaX = abs(x2 - x1);
    const int16_t deltaY = abs(y2 - y1);
    const int16_t signX = x1 < x2 ? 1 : -1;
    const int16_t signY = y1 < y2 ? 1 : -1;

    int16_t error = deltaX - deltaY;

    if (thick > 1)
        LCD_draw_circle(x2, y2, thick >> 1);
    else
        LCD_set_pixel(x2, y2, TextColor);

    while (x1 != x2 || y1 != y2)
    {
        if (thick > 1)
            LCD_draw_circle(x1, y1, thick >> 1);
        else
            LCD_set_pixel(x1, y1, TextColor);

        const int16_t error2 = error * 2;
        if (error2 > -deltaY)
        {
            error -= deltaY;
            x1 += signX;
        }
        if (error2 < deltaX)
        {
            error += deltaX;
            y1 += signY;
        }
    }
}

void LCD_print(char * str)
{
    //text_x_start = text_x;
    while (*str)
        LCD_putchar(*str++);
}

inline void LCD_out(char character, void* arg)
{
    LCD_putchar(character);
}

inline void LCD_set_text_color(uint16_t color)
{
    TextColor = color;
}

inline void LCD_set_bg_color(uint16_t color)
{
    BGColor = color;
}

inline void LCD_set_text_pos(uint16_t x, uint16_t y)
{
    text_x = x;
    text_y = y;
    text_x_start = text_x;
}

inline void LCD_set_font(const lcd_font_t * fnt)
{
    font = (lcd_font_t *) fnt;
}
