#ifndef __MAIN_H__
#define __MAIN_H__

#include "lcd.h"
#include "stm_lib.h"

//#define _DEBUG_PRINTF_ENABLED

#define LOWPASS(xold, xnew, pow2) \
   ((((1<<pow2) - 1) * xold +  xnew) >> pow2) // Простой БИХ-фильтр


#endif /* __MAIN_H__ */
