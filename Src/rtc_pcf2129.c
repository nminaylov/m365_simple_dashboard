#include "rtc_pcf2129.h"

#define PCF2129_ADDRESS             0xA2

#define PCF2129_CONTROL_1           0x00
#define PCF2129_CONTROL_2           0x01
#define PCF2129_CONTROL_3           0x02
#define PCF2129_SECONDS             0x03
#define PCF2129_MINUTES             0x04
#define PCF2129_HOURS               0x05
#define PCF2129_DAYS                0x06
#define PCF2129_WEEKDAYS            0x07
#define PCF2129_MONTHS              0x08
#define PCF2129_YEARS               0x09
#define PCF2129_SECOND_ALARM        0x0A
#define PCF2129_MINUTE_ALARM        0x0B
#define PCF2129_HOUR_ALARM          0x0C
#define PCF2129_DAY_ALARM           0x0D
#define PCF2129_WEEKDAY_ALARM       0x0E
#define PCF2129_CLKOUT_CTRL         0x0F

#define PCF2129_WDG_CTL             0x10
#define PCF2129_WDG_VAL             0x11
#define PCF2129_TS_CTL              0x12
#define PCF2129_TS_SECOND           0x13
#define PCF2129_TS_MINUTE           0x14
#define PCF2129_TS_HOUR             0x15
#define PCF2129_TS_DAY              0x16
#define PCF2129_TS_MONTH            0x17
#define PCF2129_TS_YEAR             0x18
#define PCF2129_AGING_OFFSET        0x19
#define PCF2129_INT_REG_1           0x1A
#define PCF2129_INT_REG_2           0x1B

static void rtc_i2c_init(void);
static uint8_t rtc_write_reg(uint8_t addr, uint8_t reg, uint8_t data);
static uint8_t rtc_read_reg(uint8_t addr, uint8_t reg, uint8_t * val);

static uint8_t rtc_bcd2int(uint8_t data);
static uint8_t rtc_int2bcd(uint8_t data);
static void rtc_calc_weekday(rtc_date_t * d);

uint8_t rtc_init(void)
{
    rtc_time_t time_temp;
    rtc_date_t date_temp;
    rtc_i2c_init();
    uint8_t temp_reg = 0;
    LL_mDelay(1000);
    uint8_t state = rtc_read_reg(PCF2129_ADDRESS,PCF2129_CONTROL_1,&temp_reg);
    if (temp_reg != 0x00)
    {
        state &= rtc_write_reg(PCF2129_ADDRESS,PCF2129_CONTROL_1,0x00);
        state &= rtc_write_reg(PCF2129_ADDRESS,PCF2129_CONTROL_2,0x00);
        state &= rtc_write_reg(PCF2129_ADDRESS,PCF2129_CONTROL_3,0x00);
        time_temp.hour = 0;
        time_temp.min = 0;
        time_temp.sec = 0;
        state &= rtc_set_time(&time_temp);
        date_temp.day = 1;
        date_temp.month = 1;
        date_temp.year = 21;
        state &= rtc_set_date(&date_temp);
    }
    return state;
}

uint8_t rtc_get_time(rtc_time_t * t)
{
    uint8_t state = 0;
    uint8_t temp_reg = 0;
    state = rtc_read_reg(PCF2129_ADDRESS,PCF2129_SECONDS,&temp_reg);
    t->sec = rtc_bcd2int(temp_reg&0x7F);
    state &= rtc_read_reg(PCF2129_ADDRESS,PCF2129_MINUTES,&temp_reg);
    t->min = rtc_bcd2int(temp_reg&0x7F);
    state &= rtc_read_reg(PCF2129_ADDRESS,PCF2129_HOURS,&temp_reg);
    t->hour = rtc_bcd2int(temp_reg&0x3F);
    return state;
}

uint8_t rtc_set_time(rtc_time_t * t)
{
    uint8_t state = 0;
    uint8_t temp_reg = rtc_int2bcd(t->sec);
    state = rtc_write_reg(PCF2129_ADDRESS,PCF2129_SECONDS,(temp_reg));
    temp_reg = rtc_int2bcd(t->min);
    state &= rtc_write_reg(PCF2129_ADDRESS,PCF2129_MINUTES,(temp_reg));
    temp_reg = rtc_int2bcd(t->hour);
    state &= rtc_write_reg(PCF2129_ADDRESS,PCF2129_HOURS,(temp_reg));
    return state;
}

uint8_t rtc_get_date(rtc_date_t * d)
{
    uint8_t state = 0;
    uint8_t temp_reg = 0;
    state = rtc_read_reg(PCF2129_ADDRESS,PCF2129_DAYS,&temp_reg);
    d->day = rtc_bcd2int(temp_reg&0x3F);
    state &= rtc_read_reg(PCF2129_ADDRESS,PCF2129_WEEKDAYS,&temp_reg);
    d->wday = rtc_bcd2int(temp_reg&0x07);
    state &= rtc_read_reg(PCF2129_ADDRESS,PCF2129_MONTHS,&temp_reg);
    d->month = rtc_bcd2int(temp_reg&0x1F);
    state &= rtc_read_reg(PCF2129_ADDRESS,PCF2129_YEARS,&temp_reg);
    d->year = rtc_bcd2int(temp_reg);
    return state;
}

uint8_t rtc_set_date(rtc_date_t * d)
{
    uint8_t state = 0;
    rtc_calc_weekday(d);
    uint8_t temp_reg = rtc_int2bcd(d->day);
    state = rtc_write_reg(PCF2129_ADDRESS,PCF2129_DAYS,(temp_reg));
    temp_reg = rtc_int2bcd(d->wday);
    state &= rtc_write_reg(PCF2129_ADDRESS,PCF2129_WEEKDAYS,(temp_reg));
    temp_reg = rtc_int2bcd(d->month);
    state &= rtc_write_reg(PCF2129_ADDRESS,PCF2129_MONTHS,(temp_reg));
    temp_reg = rtc_int2bcd(d->year);
    state &= rtc_write_reg(PCF2129_ADDRESS,PCF2129_YEARS,(temp_reg));
    return state;
}

static inline uint8_t rtc_bcd2int(uint8_t data)
{
    return (data & 0x0F) + ((data & 0xF0)>>4)*10;
}

static inline uint8_t rtc_int2bcd(uint8_t data)
{
    return (((data / 10) % 10) << 4) | (data % 10);
}

static void rtc_calc_weekday(rtc_date_t * d) // Считаем день недели
{
    uint8_t a = (14 - d->month) / 12; // ХЗ как это работает
    uint8_t y = d->year - a;
    uint8_t m = d->month + 12 * a - 2;
    d->wday = (d->day + y + (y / 4) - (y / 100) + (y / 400) + (31 * m) / 12) % 7;
}

static void rtc_i2c_init(void)
{
    LL_I2C_InitTypeDef I2C_InitStruct;
    LL_GPIO_InitTypeDef GPIO_InitStruct;

    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_9 | LL_GPIO_PIN_10;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_4;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    I2C_InitStruct.PeripheralMode = LL_I2C_MODE_I2C;
    I2C_InitStruct.Timing = __LL_I2C_CONVERT_TIMINGS(0x0, 0x9, 0x0, 0x18, 0x50); // 400kHz (на самом деле 70)
    I2C_InitStruct.TypeAcknowledge = LL_I2C_ACK;
    I2C_InitStruct.OwnAddrSize = LL_I2C_OWNADDRESS1_7BIT;
    I2C_InitStruct.OwnAddress1 = 0;
    I2C_InitStruct.DigitalFilter = 0;
    I2C_InitStruct.AnalogFilter = LL_I2C_ANALOGFILTER_ENABLE;
    LL_I2C_Init(I2C1,&I2C_InitStruct);

    LL_I2C_Enable(I2C1);
}

static uint8_t rtc_write_reg(uint8_t addr, uint8_t reg, uint8_t data)
{
    LL_I2C_HandleTransfer(I2C1, addr, LL_I2C_ADDRSLAVE_7BIT, 2, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_WRITE);

    while(!(LL_I2C_IsActiveFlag_TXE(I2C1))); // TODO: таймауты
    LL_I2C_TransmitData8(I2C1, reg);
    while(!(LL_I2C_IsActiveFlag_TXE(I2C1)));
    LL_I2C_TransmitData8(I2C1, data);

    while(!(LL_I2C_IsActiveFlag_STOP(I2C1)));

    return 1;
}

static uint8_t rtc_read_reg(uint8_t addr, uint8_t reg, uint8_t * val)
{
    LL_I2C_ClearFlag_STOP(I2C1);
    LL_I2C_HandleTransfer(I2C1, addr, LL_I2C_ADDRSLAVE_7BIT, 1, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_WRITE);

    while(!(LL_I2C_IsActiveFlag_TXE(I2C1)));
    LL_I2C_TransmitData8(I2C1, reg);

    //while(!(LL_I2C_IsActiveFlag_TC(I2C1)));

    while(!(LL_I2C_IsActiveFlag_STOP(I2C1)));

    LL_I2C_ClearFlag_STOP(I2C1);

    //while((LL_I2C_IsActiveFlag_BUSY(I2C1)));

    LL_I2C_HandleTransfer(I2C1, addr, LL_I2C_ADDRSLAVE_7BIT, 1, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_READ);

    while(!(LL_I2C_IsActiveFlag_RXNE(I2C1)));
    *val = LL_I2C_ReceiveData8(I2C1);

    while(!(LL_I2C_IsActiveFlag_STOP(I2C1)));

    return 1;
}
