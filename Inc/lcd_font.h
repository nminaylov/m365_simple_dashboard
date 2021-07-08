#ifndef __LCD_FONT_H
#define __LCD_FONT_H

#include "stm_lib.h"

typedef struct
{
	const uint8_t *data;
	uint8_t char_w;
	uint8_t char_h;
	uint8_t type;
	uint8_t offset;
} lcd_font_t;

extern const lcd_font_t clock_digits_32x50;
extern const lcd_font_t clock_minus_24x5;
extern const lcd_font_t clock_symbols;
extern const lcd_font_t symbols_m365;
extern const lcd_font_t t_12x24_full;

#endif
