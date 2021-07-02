#include "m365_uart.h"
#include <string.h>

static m365_data_t m365_data;

#define DATA_LEN_MAX 250
#define BUF_NB 5

typedef struct
{
    uint8_t len;
    uint8_t addr;
    uint8_t cmd;
    uint8_t arg;
    uint8_t data[DATA_LEN_MAX];
    uint16_t csum;
} m365_cmd_t;


static uint8_t m365_cmd_tx_buf[64];
static m365_cmd_t cmd[BUF_NB];
static uint8_t cmd_rx_last = 0;
static uint8_t cmd_rx_cnt = 0;
static uint8_t data_len = 0;

static uint16_t bytes_skip = 0;
static uint8_t update_flag = 0;

static void m365_uart_handle_esc(m365_cmd_t * cmd);
static void m365_uart_handle_bms(m365_cmd_t * cmd);
static void m365_request_regs(uint8_t addr, uint8_t start, uint8_t nb);
static void m365_uart_tx(uint8_t * data, uint16_t len);

void m365_uart_handler(void)
{
    if (cmd_rx_cnt > 0)
    {
        if (cmd_rx_cnt > (BUF_NB-1))
            cmd_rx_cnt = BUF_NB-1;
        int8_t cmd_rx = cmd_rx_last+1-cmd_rx_cnt;
        if (cmd_rx < 0)
            cmd_rx += BUF_NB;
        cmd_rx_cnt--;

        uint16_t csum_temp = cmd[cmd_rx].len + cmd[cmd_rx].addr + cmd[cmd_rx].cmd + cmd[cmd_rx].arg;
        for (uint8_t i = 0; i < cmd[cmd_rx].len-2; i++)
        {
            csum_temp += cmd[cmd_rx].data[i];
        }
        csum_temp = 0xFFFF ^ csum_temp;

        if (csum_temp == cmd[cmd_rx].csum)
        {
            switch (cmd[cmd_rx].addr)
            {
            case 0x23:
                m365_uart_handle_esc(&cmd[cmd_rx]);
                break;
            case 0x25:
                m365_uart_handle_bms(&cmd[cmd_rx]);
                break;

            default:
                break;
            }
        }
    }

    if (update_flag == 1)
    {
        update_flag = 0;
        m365_request_regs(0x20, 0x48, 1);
    }
}

static void m365_uart_handle_esc(m365_cmd_t * cmd)
{
    if (cmd->cmd == 0x01)
    {
        for (uint8_t reg = 0; reg < ((cmd->len-2)/2); reg++)
        {
            switch (cmd->arg + reg)
            {
            case 0x26:
                memcpy(&m365_data.speed, &cmd->data[reg*2], 2);
                m365_data.update_flag = 1;
                break;
            case 0x29:
                memcpy(&m365_data.odo, &cmd->data[reg*2], 4);
                m365_data.update_flag = 1;
                break;
            case 0x2F:
                memcpy(&m365_data.trip, &cmd->data[reg*2], 2);
                m365_data.update_flag = 1;
                break;
            case 0xBB:
                memcpy(&m365_data.esc_temp, &cmd->data[reg*2], 2);
                m365_data.update_flag = 1;
                break;
            case 0x48:
                memcpy(&m365_data.voltage, &cmd->data[reg*2], 2);
                m365_data.update_flag = 1;
                break;
            case 0x50:
                memcpy(&m365_data.current, &cmd->data[reg*2], 2);
                m365_data.update_flag = 1;
                break;
            default:
                break;
            }
        }
    }
}

static void m365_uart_handle_bms(m365_cmd_t * cmd)
{
    if (cmd->cmd == 0x01)
    {
        for (uint8_t reg = 0; reg < ((cmd->len-2)/2); reg++)
        {
            switch (cmd->arg + reg)
            {
            case 0x31:
                memcpy(&m365_data.bms_mah, &cmd->data[reg*2], 2);
                m365_data.update_flag = 1;
                break;
            case 0x32:
                memcpy(&m365_data.bms_percent, &cmd->data[reg*2], 2);
                m365_data.update_flag = 1;
                break;
            case 0x35:
                memcpy(m365_data.bms_temp, &cmd->data[reg*2], 2);
                m365_data.update_flag = 1;
                break;
            default:
                break;
            }
        }
    }
}

static void m365_request_regs(uint8_t addr, uint8_t start, uint8_t nb)
{
    m365_cmd_tx_buf[0] = 0x55;
    m365_cmd_tx_buf[1] = 0xAA;

    m365_cmd_tx_buf[2] = 3;     // len
    m365_cmd_tx_buf[3] = addr;  // addr
    m365_cmd_tx_buf[4] = 0x01;  // cmd
    m365_cmd_tx_buf[5] = start; // arg

    m365_cmd_tx_buf[6] = nb*2;  // data[0]

    uint16_t csum_temp = 0;
    for (uint8_t i = 2; i < m365_cmd_tx_buf[2]+2+2; i++)
    {
        csum_temp += m365_cmd_tx_buf[i];
    }
    csum_temp = 0xFFFF ^ csum_temp;
    m365_cmd_tx_buf[m365_cmd_tx_buf[2]+4] = csum_temp & 0xFF;
    m365_cmd_tx_buf[m365_cmd_tx_buf[2]+5] = (csum_temp >> 8) & 0xFF;

    m365_uart_tx(m365_cmd_tx_buf, m365_cmd_tx_buf[2]+6);
}

static void m365_uart_tx(uint8_t * data, uint16_t len)
{
    bytes_skip = len;
    for (uint16_t i = 0; i < len; i++)
    {
        while (!LL_USART_IsActiveFlag_TXE(USART1));
        LL_USART_TransmitData8(USART1, data[i]);
    }

}

m365_data_t * m365_uart_init(void)
{
    LL_USART_InitTypeDef USART_InitStruct;
    LL_GPIO_InitTypeDef GPIO_InitStruct;
    LL_TIM_InitTypeDef TIM_InitStruct;

    LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_USART1);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_2; // TX
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_1;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    USART_InitStruct.BaudRate = 115200;
    USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
    USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
    USART_InitStruct.Parity = LL_USART_PARITY_NONE;
    USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
    USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
    USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
    LL_USART_Init(USART1, &USART_InitStruct);
    LL_USART_ConfigAsyncMode(USART1);

    LL_USART_ConfigHalfDuplexMode(USART1);

    LL_USART_EnableIT_RXNE(USART1);

    NVIC_SetPriority(USART1_IRQn, 1);
    NVIC_EnableIRQ(USART1_IRQn);

    LL_USART_Enable(USART1);


    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM14);

    TIM_InitStruct.Prescaler = 48-1; // 48 000 000/48/1000 = 1000
    TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
    TIM_InitStruct.Autoreload = 5000;
    TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
    TIM_InitStruct.RepetitionCounter = 0;
    LL_TIM_Init(TIM14, &TIM_InitStruct);
    LL_TIM_SetOnePulseMode(TIM14, LL_TIM_ONEPULSEMODE_SINGLE);

    LL_TIM_ClearFlag_UPDATE(TIM14);
    LL_TIM_EnableIT_UPDATE(TIM14);

    NVIC_SetPriority(TIM14_IRQn, 2);
    NVIC_EnableIRQ(TIM14_IRQn);

    LL_TIM_EnableCounter(TIM14);

    return &m365_data;
}

void USART1_IRQHandler(void)
{
    static uint8_t state = 0;
    static uint8_t buf_nb = 0;

    if (LL_USART_IsActiveFlag_ORE(USART1))
    {
        LL_USART_ClearFlag_ORE(USART1);
    }

    if (LL_USART_IsActiveFlag_RXNE(USART1))
    {
        LL_TIM_SetCounter(TIM14, 0);
        uint16_t rx_data = LL_USART_ReceiveData8(USART1);

        if (bytes_skip > 0)
        {
            bytes_skip--;
        }
        else
        {
            switch (state)
            {
            case 0:
                if (rx_data == 0x55)
                    state = 1;
                break;
            case 1:
                if (rx_data == 0xAA)
                    state = 2;
                else
                    state = 0;
                break;
            case 2: //len
                cmd[buf_nb].len = rx_data;
                if (cmd[buf_nb].len < (DATA_LEN_MAX+2))
                    state = 3;
                else
                    state = 0;
                break;
            case 3: //addr
                cmd[buf_nb].addr = rx_data;
                state = 4;
                break;
            case 4: //cmd
                cmd[buf_nb].cmd = rx_data;
                state = 5;
                break;
            case 5: //arg
                cmd[buf_nb].arg = rx_data;
                state = 6;
                data_len = 0;
                break;
            case 6: //data
                cmd[buf_nb].data[data_len++] = rx_data;
                if (data_len >= cmd[buf_nb].len-2)
                    state = 7;
                break;
            case 7: //csum - 1
                cmd[buf_nb].csum = rx_data;
                state = 8;
                break;
            case 8: //csum - 2
                cmd[buf_nb].csum |= rx_data << 8;
                cmd_rx_cnt++;
                cmd_rx_last = buf_nb;
                buf_nb++;
                if (buf_nb >= BUF_NB)
                    buf_nb = 0;
                state = 0;
                LL_TIM_EnableCounter(TIM14);
                break;
            }
        }
    }
}

void TIM14_IRQHandler(void)
{
    LL_TIM_ClearFlag_UPDATE(TIM14);
    if (LL_USART_IsActiveFlag_IDLE(USART1))
        update_flag = 1;
}

