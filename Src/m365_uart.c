#include "m365_uart.h"
#include <string.h>

static m365_data_t m365_data;

#define DATA_LEN_MAX 250
#define BUF_NB 5
#define DELAY_SINCE_RX_MIN 2
#define DELAY_SINCE_RX_MAX 7

typedef struct
{
    uint8_t len;
    uint8_t addr;
    uint8_t cmd;
    uint8_t arg;
    uint8_t data[DATA_LEN_MAX];
    uint16_t csum;
} m365_cmd_t;

typedef struct
{
    uint8_t addr;
    uint8_t reg_start;
    uint8_t reg_nb;
    uint16_t period;
    uint16_t tick_last;
} m365_req_t;

static m365_req_t req_list_run[6] =
{
    {0x20, 0x26, 0x01, 100, 0}, // ESC - Speed
    {0x20, 0x48, 0x09, 100, 0}, // ESC - Voltage, current

    {0x20, 0x29, 0x07, 1000, 0}, // ESC - Distance
    {0x20, 0xBB, 0x01, 1000, 0}, // ESC - Temperature

    {0x22, 0x30, 0x06, 1000, 0}, // BMS

    {0xFF, 0x00, 0x00,    0, 0},
};

static m365_req_t req_list_charge[2] =
{
    {0x22, 0x30, 0x20, 1000, 0}, // BMS

    {0xFF, 0x00, 0x00,    0, 0},
};

static uint8_t req_cur_nb = 0;
static m365_req_t * req_list_cur = NULL;

static uint16_t tick_cur = 0;
static int16_t  tick_rx_idle = -1;

static uint8_t m365_cmd_tx_buf[64];
static m365_cmd_t cmd[BUF_NB];
static uint8_t cmd_rx_last = 0;
static uint8_t cmd_rx_cnt = 0;
static uint8_t data_len = 0;

static uint16_t rx_bytes_skip = 0;

static void m365_uart_handle_esc(m365_cmd_t * cmd);
static void m365_uart_handle_bms(m365_cmd_t * cmd);
static void m365_uart_handle_inputs(m365_cmd_t * cmd);
static void m365_request_regs(uint8_t addr, uint8_t start, uint8_t nb);
static uint16_t m365_uart_get_ticks_since(uint16_t ticks_last);

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
            case 0x20:
                m365_uart_handle_inputs(&cmd[cmd_rx]);
                break;
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

    if (req_list_cur != NULL)
    {
    	if (m365_uart_get_ticks_since(req_list_cur[req_cur_nb].tick_last) > req_list_cur[req_cur_nb].period)
		{
			if ((tick_rx_idle > DELAY_SINCE_RX_MIN) && (tick_rx_idle < DELAY_SINCE_RX_MAX))
			{
				tick_rx_idle = -1;
				m365_request_regs(req_list_cur[req_cur_nb].addr, req_list_cur[req_cur_nb].reg_start, req_list_cur[req_cur_nb].reg_nb);
				req_list_cur[req_cur_nb].tick_last = tick_cur;
			}
		}
		req_cur_nb++;
		if (req_list_cur[req_cur_nb].addr == 0xFF)
			req_cur_nb = 0;
    }
}

void m365_uart_set_req_mode(uint8_t req_mode)
{
	switch (req_mode)
	{
	case 0:
		req_list_cur = NULL;
		break;
	case 1:
		req_list_cur = req_list_run;
		break;
	case 2:
		req_list_cur = req_list_charge;
		break;
	}
	req_cur_nb = 0;
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
            case 0x30:
                memcpy(&m365_data.bms_flags, &cmd->data[reg*2], 2);
                m365_data.update_flag = 1;
                break;
            case 0x31:
                memcpy(&m365_data.bms_mah, &cmd->data[reg*2], 2);
                m365_data.update_flag = 1;
                break;
            case 0x32:
                memcpy(&m365_data.bms_percent, &cmd->data[reg*2], 2);
                m365_data.update_flag = 1;
                break;
            case 0x33:
                memcpy(&m365_data.bms_current, &cmd->data[reg*2], 2);
                m365_data.update_flag = 1;
                break;
            case 0x34:
                memcpy(&m365_data.bms_voltage, &cmd->data[reg*2], 2);
                m365_data.update_flag = 1;
                break;
            case 0x35:
                memcpy(m365_data.bms_temp, &cmd->data[reg*2], 2);
                m365_data.update_flag = 1;
                break;
            case 0x36:
                memcpy(&m365_data.bms_balance_flags, &cmd->data[reg*2], 2);
                m365_data.update_flag = 1;
                break;
            case 0x40:
            case 0x41:
            case 0x42:
            case 0x43:
            case 0x44:
            case 0x45:
            case 0x46:
            case 0x47:
            case 0x48:
            case 0x49:
            	memcpy(&m365_data.bms_cell_voltage[cmd->arg+reg-0x40], &cmd->data[reg*2], 2);
            	m365_data.update_flag = 1;
            	break;
            default:
                break;
            }
        }
    }
}

static void m365_uart_handle_inputs(m365_cmd_t * cmd)
{
    if (cmd->cmd == 0x65) // Update head inputs
    {
    	m365_data.trottle_pos = cmd->data[1];
    	m365_data.brake_pos = cmd->data[2];
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

    rx_bytes_skip = m365_cmd_tx_buf[2]+6; // „тобы не обрабатывать свои же данные

    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_2, m365_cmd_tx_buf[2]+6);
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_2);
}

m365_data_t * m365_uart_init(void)
{
    LL_USART_InitTypeDef USART_InitStruct;
    LL_GPIO_InitTypeDef GPIO_InitStruct;
    LL_DMA_InitTypeDef DMA_InitStruct;
    LL_TIM_InitTypeDef TIM_InitStruct;

    LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_USART1);
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_2;// | LL_GPIO_PIN_3; // TX RX
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
    LL_USART_EnableDMAReq_TX(USART1);

    NVIC_SetPriority(USART1_IRQn, 0);
    NVIC_EnableIRQ(USART1_IRQn);

    LL_USART_Enable(USART1);

    // DMA
    LL_DMA_DeInit(DMA1, LL_DMA_CHANNEL_2); // Tx
    DMA_InitStruct.PeriphOrM2MSrcAddress = LL_USART_DMA_GetRegAddr(USART1,LL_USART_DMA_REG_DATA_TRANSMIT);
    DMA_InitStruct.MemoryOrM2MDstAddress = (uint32_t)m365_cmd_tx_buf;
    DMA_InitStruct.Direction = LL_DMA_DIRECTION_MEMORY_TO_PERIPH;
    DMA_InitStruct.NbData = 0;
    DMA_InitStruct.PeriphOrM2MSrcIncMode = LL_DMA_PERIPH_NOINCREMENT;
    DMA_InitStruct.MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT;
    DMA_InitStruct.PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_BYTE;
    DMA_InitStruct.MemoryOrM2MDstDataSize = LL_DMA_PDATAALIGN_HALFWORD;
    DMA_InitStruct.Mode = LL_DMA_MODE_NORMAL;
    DMA_InitStruct.Priority = LL_DMA_PRIORITY_VERYHIGH;
    LL_DMA_Init(DMA1, LL_DMA_CHANNEL_2, &DMA_InitStruct);

    LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_2);

    NVIC_SetPriority(DMA1_Channel2_3_IRQn, 1);
    NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);


    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM14);

    TIM_InitStruct.Prescaler = 48-1; // 48 000 000/48/1000 = 1000
    TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
    TIM_InitStruct.Autoreload = 1000;
    TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
    TIM_InitStruct.RepetitionCounter = 0;
    LL_TIM_Init(TIM14, &TIM_InitStruct);

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

    tick_rx_idle = -1;

    if (LL_USART_IsActiveFlag_ORE(USART1))
    {
        LL_USART_ClearFlag_ORE(USART1);
    }

    if (LL_USART_IsActiveFlag_RXNE(USART1))
    {
        uint16_t rx_data = LL_USART_ReceiveData8(USART1);

        if (rx_bytes_skip > 0)
        {
            rx_bytes_skip--;
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
                if (((cmd[buf_nb].addr == 0x20) && (cmd[buf_nb].cmd == 0x65)) ||
                    ((cmd[buf_nb].addr == 0x21) && (cmd[buf_nb].cmd == 0x64)))
                    tick_rx_idle = 0;
                cmd_rx_cnt++;
                cmd_rx_last = buf_nb;
                buf_nb++;
                if (buf_nb >= BUF_NB)
                    buf_nb = 0;
                state = 0;
                break;
            }
        }
    }
}

void DMA1_Channel2_3_IRQHandler(void)
{
    if (LL_DMA_IsActiveFlag_TC2(DMA1)) // Tx end
    {
        LL_DMA_ClearFlag_TC2(DMA1);
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_2);
    }
}

void TIM14_IRQHandler(void)
{
    LL_TIM_ClearFlag_UPDATE(TIM14);
    tick_cur++;
    if (tick_rx_idle >= 0)
        tick_rx_idle++;
}

static uint16_t m365_uart_get_ticks_since(uint16_t ticks_last)
{
    int16_t t = tick_cur - ticks_last;
    if (t < 0)
        t += 0xFFFF;
    return t;
}
