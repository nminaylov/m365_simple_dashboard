#ifndef M365_UART_H_
#define M365_UART_H_

#include "stm_lib.h"

typedef struct
{
    uint16_t speed;
    uint16_t voltage;
    int16_t current;

    uint16_t trip;
    uint32_t odo;
    uint16_t esc_temp;

    uint16_t bms_percent;
    uint16_t bms_mah;
    uint8_t bms_temp[2];
} m365_data_t;

m365_data_t * m365_uart_init(void);
void m365_uart_handler(void);


#endif /* M365_UART_H_ */
