/* Функции для работы с дисплеем 240x240 (ST7789) */

#include "LCD.h"
#include <math.h>
#include "printf.h"
#include <stdarg.h>

uint16_t BGColor = WHITE;
uint16_t TextColor = BLACK;

uint16_t Text_X = 0;
uint16_t Text_Y = 0;
uint16_t Text_X_start = 0;

tFont * Font;

void LCD_init(void)
{
    LCD_HWinit();
    LCD_RST_DN; // Дергаем ресет
    LL_mDelay(10);
    LCD_RST_UP;
    LL_mDelay(50);

    LCD_SendCMD(0x01); // SWreset
    LL_mDelay(120);
    LCD_SendCMD(0x11); // Sleep Out
    LL_mDelay(5);

    LCD_SendCMD(0x3A);
    LCD_SendData(0x55);
    LL_mDelay(1);

    LCD_SendCMD(0x2A);
    LCD_SendData(0x00);
    LCD_SendData(0x00);
    LCD_SendData(0x00);
    LCD_SendData(0xF0);

    LCD_SendCMD(0x2B);
    LCD_SendData(0x00);
    LCD_SendData(0x00);
    LCD_SendData(0x00);
    LCD_SendData(0xF0);

    LCD_SendCMD(0x21);
    LL_mDelay(1);

    LCD_SendCMD(0x13);
    LL_mDelay(1);

    LCD_SendCMD(0x29);
    LL_mDelay(1);

    LCD_SendCMD(0x36);
#ifdef LCD_TURN
    LCD_SendData(0xC0);
#else
    LCD_SendData(0x00);
#endif
    LL_mDelay(1);
    LCD_Fill(0, 0, LCD_W, LCD_H, 0x00);
    LL_mDelay(1);
    LCD_BL_ON;
}

void LCD_Sleep(void)
{
    LCD_SendCMD(0x10);
    LL_mDelay(5);
}

void LCD_HWinit(void) // Настраиваем пины, SPI
{
    LL_SPI_InitTypeDef SPI_InitStruct;
    LL_GPIO_InitTypeDef GPIO_InitStruct;

    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SPI1);

    LCD_DC_DN;
    LCD_BL_OFF;
    GPIO_InitStruct.Pin = LL_GPIO_PIN_6; // DC
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_5 | LL_GPIO_PIN_7; // SCL, SDA
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_0 | LL_GPIO_PIN_1; // RST, BL
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    SPI_InitStruct.TransferDirection = LL_SPI_HALF_DUPLEX_TX;
    SPI_InitStruct.Mode = LL_SPI_MODE_MASTER;
    SPI_InitStruct.DataWidth = LL_SPI_DATAWIDTH_8BIT;
    SPI_InitStruct.ClockPolarity = LL_SPI_POLARITY_HIGH;
    SPI_InitStruct.ClockPhase = LL_SPI_PHASE_2EDGE;
    SPI_InitStruct.NSS = LL_SPI_NSS_SOFT;
    SPI_InitStruct.BaudRate = LL_SPI_BAUDRATEPRESCALER_DIV2;
    SPI_InitStruct.BitOrder = LL_SPI_MSB_FIRST;
    SPI_InitStruct.CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE;
    SPI_InitStruct.CRCPoly = 10;
    LL_SPI_Init(SPI1, &SPI_InitStruct);

    LL_SPI_Enable(SPI1);
}

void LCD_SendCMD(uint8_t val)
{
    while (!SPI1_TXE); //wait buffer empty
    LCD_DC_DN; // A0 = 0 - CMD
    SPI1->DR = val;
    while (SPI1_BSY); //wait finish sending
}

void LCD_SendData(uint8_t val)
{
    while (!SPI1_TXE); //wait buffer empty
    LCD_DC_UP; // A0 = 1 - DATA
    SPI1->DR = val & 0xFF;
    while (SPI1_BSY); //wait finish sending
}

void LCD_SendPixel(uint16_t data) // Шлем 2 байта (данные)
{
    while (!SPI1_TXE); //wait buffer empty
    SPI1->DR = (data >> 8);
    while (!SPI1_TXE); //wait buffer empty
    SPI1->DR = (data & 0xFF);
}

// Установка окна x, y - координаты начала; w, h - ширина, высота окна
void LCD_SetWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    uint16_t y_t = y;
    LCD_SendCMD(0x2A); // CASET
    LCD_SendData(0x00);
    LCD_SendData(x);
    LCD_SendData(0x00);
    LCD_SendData(x + w - 1);
    LCD_SendCMD(0x2B); // RASET
#if (LCD_H == 240)
#ifdef LCD_TURN
    y_t += 80;
    LCD_SendData(y_t >> 8);
    LCD_SendData(y_t & 0xFF);
    LCD_SendData((y_t + h - 1) >> 8);
    LCD_SendData((y_t + h - 1) & 0xFF);
#else
    LCD_SendData(0);
    LCD_SendData(y_t & 0xFF);
    LCD_SendData(0);
    LCD_SendData((y_t+h-1) & 0xFF);
#endif
#elif (LCD_H == 320)
    LCD_SendData(y_t >> 8);
    LCD_SendData(y_t & 0xFF);
    LCD_SendData((y_t+h-1) >> 8);
    LCD_SendData((y_t+h-1) & 0xFF);
#endif
    LCD_SendCMD(0x2C); // RAMWR
}

// Заполняем область цветом
void LCD_Fill(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    uint16_t count = w * h;
    LCD_SetWindow(x, y, w, h);
    LCD_DC_UP;
    while (count--)
        LCD_SendPixel(color);
    while (SPI1_BSY);
}

void LCD_putchar(char chr)
{
    LCD_SetWindow(Text_X, Text_Y, Font->char_w, Font->char_h);
    LCD_DC_UP;
    //volatile uint16_t temp;
    if (chr == '\n') // Если символ переноса
    {
#ifdef  SAVE_X_OFFSET
        Text_X = Text_X_start;
#else
        Text_X = 0;
#endif
        Text_Y += Font->char_h;
        chr = 0;
    }
    else
    {
        Text_X += Font->char_w;
        if (Text_X > (LCD_W - Font->char_w))
        {
#ifdef  SAVE_X_OFFSET
            Text_X = Text_X_start;
#else
            Text_X = 0;
#endif
            Text_Y += Font->char_h;
        }
    }
    if (Text_Y > (LCD_H - Font->char_h))
        Text_Y = 0;

    if (chr < Font->offset)
        chr = Font->offset;

    uint8_t * chardata = NULL;

    if (Font->type == 0)
    {
        if (chr < Font->offset)
            chr = Font->offset;
        chardata = (uint8_t *) &(Font->data[(Font->char_w * Font->char_h / 8) * (chr - Font->offset)]);
    }
    if (Font->type == 1)
    {
        if (chr < Font->offset)
            chr = Font->offset;
        chardata = (uint8_t *) &(Font->data[(Font->char_w - 1) * (chr - Font->offset)]);
    }
    LCD_SendChar(chardata);
    while (SPI1_BSY);
}

void LCD_SendChar(uint8_t * start)
{
    if (Font->type == 0)
    {
        uint8_t len = Font->char_w * Font->char_h / 8;
        for (uint8_t byte_cnt = 0; byte_cnt < len; byte_cnt++)
        {
            for (uint8_t bit_cnt = 0; bit_cnt < 8; bit_cnt++)
            {
                if ((start[byte_cnt] << bit_cnt) & 0x80)
                    LCD_SendPixel(TextColor);
                else
                    LCD_SendPixel(BGColor);
            }
        }
    }
    if (Font->type == 1)
    {
        for (uint8_t bit_cnt = 0; bit_cnt < 8; bit_cnt++)
        {
            for (uint8_t byte_cnt = 0; byte_cnt < (Font->char_w - 1); byte_cnt++)
            {
                if ((start[byte_cnt] >> bit_cnt) & 0x01)
                    LCD_SendPixel(TextColor);
                else
                    LCD_SendPixel(BGColor);
            }
            LCD_SendPixel(BGColor);
        }
    }
}

// Вывод монохромного изображения
void LCD_Image_mono(tImage img, uint16_t x, uint16_t y)
{
    LCD_SetWindow(x, y, img.width, img.height);
    LCD_DC_UP;
    for (uint16_t byte_cnt = 0; byte_cnt < img.width * img.width / 8; byte_cnt++)
    {
        for (uint8_t bit_cnt = 0; bit_cnt < 8; bit_cnt++)
        {
            if ((img.data[byte_cnt] << bit_cnt) & 0x80)
                LCD_SendPixel(TextColor);
            else
                LCD_SendPixel(BGColor);
        }
    }
    while (SPI1_BSY);
}

void LCD_print(char * str)
{
    //Text_X_start = Text_X;
    while (*str)
        LCD_putchar(*str++);
}

inline void LCD_out(char character, void* arg)
{
    LCD_putchar(character);
}

inline void LCD_SetTextColor(uint16_t color)
{
    TextColor = color;
}

inline void LCD_SetBGColor(uint16_t color)
{
    BGColor = color;
}

inline void LCD_SetTextPos(uint16_t x, uint16_t y)
{
    Text_X = x;
    Text_Y = y;
    Text_X_start = Text_X;
}

inline void LCD_SetFont(const tFont * fnt)
{
    Font = (tFont *) fnt;
}
