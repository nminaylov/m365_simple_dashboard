#ifndef M365_UART_H_
#define M365_UART_H_

#include "stm_lib.h"

typedef struct
{
    int16_t speed;
    uint16_t voltage;
    int16_t current;

    uint16_t trip;
    uint32_t odo;
    int16_t esc_temp;

    uint16_t bms_flags;
    uint16_t bms_percent;
    uint16_t bms_mah;
    uint16_t bms_voltage;
    int16_t bms_current;
    uint8_t bms_temp[2];
    uint16_t bms_balance_flags;
    uint16_t bms_cell_voltage[10];

    uint8_t trottle_pos;
    uint8_t brake_pos;

    uint8_t update_flag;
} m365_data_t;

m365_data_t * m365_uart_init(void);
void m365_uart_set_req_mode(uint8_t req_mode);
void m365_uart_handler(void);


#endif /* M365_UART_H_ */
