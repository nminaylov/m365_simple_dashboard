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
}tFont;

extern const tFont clock_digits_32x50;
extern const tFont clock_minus_24x5;
extern const tFont clock_digits;
extern const tFont clock_symbols;
extern const tFont symbols_m365;
extern const tFont t_12x24_full;
extern const tFont page_markers;
extern const tFont t_8x14_rus;
extern const tFont t_16x22_digits;
extern const tFont symbols;

#endif
