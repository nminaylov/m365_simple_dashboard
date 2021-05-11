#ifndef __LCD_H
#define __LCD_H

#include "main.h"
#include "stm_lib.h"
#include "lcd_image.h"
#include "lcd_font.h"

#ifndef DEBUG_BOARD
#define LCD_W (240)
#define LCD_H (240)

#define LCD_TURN
#else
#define LCD_W (240)
#define LCD_H (240)

//#define LCD_TURN
#endif

#define LCD_MAX_STRING_SIZE 100 // максимальная длина строки для printf
#define SAVE_X_OFFSET

//#define LCD_BL_OFF 	    LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_1)
//#define LCD_BL_ON	 	LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_1)
#define LCD_DC_DN 		LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_6)
#define LCD_DC_UP 		LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_6)
#define LCD_RST_DN      LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_1)
#define LCD_RST_UP      LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_1)
#define SPI1_BSY 		(SPI1->SR & LL_SPI_SR_BSY)
#define SPI1_TXE 		(SPI1->SR & LL_SPI_SR_TXE)

#define BLACK           0x0000      /*   0,   0,   0 */
#define NAVY            0x000F      /*   0,   0, 128 */
#define DGREEN          0x03E0      /*   0, 128,   0 */
#define DCYAN           0x03EF      /*   0, 128, 128 */
#define MAROON          0x7800      /* 128,   0,   0 */
#define PURPLE          0x780F      /* 128,   0, 128 */
#define OLIVE           0x7BE0      /* 128, 128,   0 */
#define LGRAY           0xC618      /* 192, 192, 192 */
#define DGRAY           0x7BEF      /* 128, 128, 128 */
#define BLUE            0x001F      /*   0,   0, 255 */
#define GREEN           0x07E0      /*   0, 255,   0 */
#define CYAN            0x07FF      /*   0, 255, 255 */
#define RED             0xF800      /* 255,   0,   0 */
#define MAGENTA         0xF81F      /* 255,   0, 255 */
#define YELLOW          0xFFE0      /* 255, 255,   0 */
#define WHITE           0xFFFF      /* 255, 255, 255 */
#define ORANGE          0xFD20      /* 255, 165,   0 */
#define GREENYELLOW     0xAFE5      /* 173, 255,  47 */
#define BROWN           0XBC40
#define BRRED           0XFC07

#define LCD_printf(...) fctprintf(&LCD_out, NULL, __VA_ARGS__)

void LCD_init(void);
void LCD_HWinit(void);
void LCD_SendCMD(uint8_t val);
void LCD_SendData(uint8_t val);
void LCD_SendPixel(uint16_t data);
void LCD_Sleep(void);
void LCD_SetWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void LCD_Fill(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void LCD_putchar(char chr);
void LCD_SendChar(uint8_t * start);
void LCD_Image_mono(tImage img, uint16_t x, uint16_t y);
void LCD_set_pixel(uint16_t x, uint16_t y, uint16_t color);
void LCD_draw_circle(int16_t x0, int16_t y0, int16_t radius);
void LCD_draw_line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t thick);
void LCD_print(char * str);
void LCD_out(char character, void* arg);
//void LCD_printf(const char *pFormat, ...);
void LCD_SetTextColor(uint16_t color);
void LCD_SetBGColor(uint16_t color);
void LCD_SetTextPos(uint16_t x, uint16_t y);
void LCD_SetFont(const tFont * fnt);

#endif
