#include "radio_ctrl.h"
#include "config.h"
#include "main.h"
#include "stdlib.h"
#include "stm32f3xx_ll_rcc.h"
#include "stm32f3xx_ll_bus.h"
#include "stm32f3xx_ll_utils.h"
#include "stm32f3xx_ll_gpio.h"
#include "stm32f3xx_ll_tim.h"
#include "radio_adc.h"

#define RADIO_IF_FREQ_HZ        10700000 //10.7MHz
#define RADIO_TUNE_STEP_100KHZ  100000 //100kHz
#define RADIO_TUNE_STEP_1MHZ    1000000 //1MHz

/// Duration of frequency measurement
#define RADIO_FREQ_MEAS_TIMEBASE_US     1000 //1ms

#define RADIO_FREQ_MEAS_PERIOD_MS       100 //100ms

// Threshold level
#define RADIO_FREQ_CHECK_THRESHOLD_HZ   2000000 //2MHz



/// Current radio RX frequency
uint32_t radio_current_set_freq_hz = 100.0e6;

// Time when frequency was measured
uint32_t radio_last_freq_meas_timestamp_ms = 0;

char *radio_current_set_station_name = NULL;

bool radio_freq_measurement_running = false;
bool radio_freq_lock_lost_flag = false;

//Measured with 1MHz steps
volatile uint32_t radio_measured_frequency_hz = 0;

radio_tune_mode_t radio_tune_mode = RADIO_TUNE_MODE_100K;

void radio_send_pll_val(uint8_t counter_n, uint8_t counter_a);
void radio_ctrl_init_timers(void);
void radio_start_measure_freq(void);

int radio_ctrl_get_closest_station_index(uint32_t frequency_hz);

//https://noginsk-service.ru/page.php?575
const radio_stations_t radio_stations_array[] = 
{
    {87500000, "Бизнес FM"},
    {87900000, "Радио Like FM"},
    {88300000, "Радио Ретро FM"},
    {88700000, "Радио Юмор FM"},
    {89100000, "Радио JAZZ"},
    {89500000, "Калина Красная FM"},
    {89900000, "Радио Record"},
    {90300000, "Авторадио"},
    {90800000, "Радио Relax FM"},
    {91200000, "Спутник"},
    {91600000, "Радио Культура"},
    {92000000, "Радио Москва FM"},
    {92400000, "Радио Дача"},
    {92800000, "Радио РБК"},
    {93200000, "STUDIO 21"},
    {93600000, "Коммерсантъ FM"},
    {94000000, "Радио Восток FM"},
    {94400000, "Первое Спорт. радио"},
    {94800000, "Говорит Москва"},
    {95200000, "Rock FM"},
    {95600000, "Звезда FM"},
    {96000000, "Дорожное радио"},
    {96400000, "Радио Такси FM"},
    {96800000, "Детское радио"},
    {97200000, "Комсомольская Правда"},
    {97600000, "Вести FM"},
    {98000000, "Шоколад"},
    {98400000, "Новое радио"},
    {98800000, "Радио Романтика"},
    {99200000, "Радио Орфей"},
    {99600000, "Радио Русский Хит"},
    {100100000, "Серебряный дождь"},
    {100500000, "Радио Жара FM"},
    {100900000, "Радио Вера"},
    {101200000, "DFM"},
    {101500000, "Радио России"},
    {101800000, "Наше радио"},
    {102100000, "Монте Карло"},
    {102500000, "Comedy Radio"},
    {103000000, "Радио Шансон"},
    {103400000, "Маяк"},
    {103700000, "Радио Максимум"},
    {104200000, "Радио NRJ"},
    {104700000, "Радио 7"},
    {105000000, "Радио Гордость"},
    {105300000, "Радио Москвы"},
    {105700000, "Русское Радио"},
    {106200000, "Европа Плюс"}
};

const int radio_stations_count = sizeof(radio_stations_array) / sizeof(radio_stations_t);



void radio_ctrl_init(void)
{
    LL_GPIO_InitTypeDef GPIO_InitStructure = {0};

    GPIO_InitStructure.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStructure.Mode = LL_GPIO_MODE_OUTPUT;

    GPIO_InitStructure.Pin = RADIO_PLL_CLK_PIN | RADIO_PLL_DAT_PIN | RADIO_PLL_LAT_PIN;
    LL_GPIO_Init(RADIO_PLL_PORT, &GPIO_InitStructure);
    
    radio_ctrl_init_timers();
    radio_adc_init();
    
    //radio_set_new_frequency(88.3e6);
    radio_set_new_frequency(104.7e6);
    LL_mDelay(300);
}

void radio_ctrl_handling(void)
{
    radio_adc_handling();
        
    uint32_t freq_time_diff_ms = ms_tick - radio_last_freq_meas_timestamp_ms;
    if ((freq_time_diff_ms > RADIO_FREQ_MEAS_PERIOD_MS) && 
        (radio_freq_measurement_running == false))
    {
        radio_start_measure_freq();
    }
    
    if (radio_freq_measurement_running)
    {
        if ((LL_TIM_IsEnabledCounter(RADIO_MASTER_TIM)) == false)
        {
            uint32_t pulses = LL_TIM_OC_GetCompareCH1(RADIO_COUNTING_TIM); 
            radio_measured_frequency_hz = pulses * 1000 * 40 - RADIO_IF_FREQ_HZ;
            radio_freq_measurement_running = false;
            radio_last_freq_meas_timestamp_ms = ms_tick;
            
            uint32_t freq_diff_hz = abs(radio_measured_frequency_hz - radio_current_set_freq_hz);
            radio_freq_lock_lost_flag = 
                (freq_diff_hz > RADIO_FREQ_CHECK_THRESHOLD_HZ) ? true : false;
        }
    }
}

void radio_set_new_frequency(uint32_t new_freq_hz)
{
    radio_current_set_freq_hz = new_freq_hz;
    new_freq_hz += RADIO_IF_FREQ_HZ;
    
    uint32_t mhz_integer = (uint32_t)(new_freq_hz / 1000000);
    uint32_t mhz_dec = (uint32_t)(new_freq_hz / 100000) % 10;
    
    radio_send_pll_val(mhz_integer, mhz_dec);
    radio_freq_lock_lost_flag = false;
    
    radio_current_set_station_name = radio_ctrl_get_station_name(radio_current_set_freq_hz);
}

// Send 17-bit packet
void radio_send_pll_val(uint8_t counter_n, uint8_t counter_a)
{
    uint32_t k_dpkd = ((uint32_t)counter_n << 4) | (counter_a & 0x0F);

    uint32_t packet = 0;

    packet |= (1UL << 16); 

    //(ЧМ-канал)
    //packet |= (1UL << 15); 

    packet |= (k_dpkd & 0x0FFF);


    LL_GPIO_ResetOutputPin(RADIO_PLL_PORT, RADIO_PLL_LAT_PIN);
    LL_mDelay(2); 
    LL_GPIO_SetOutputPin(RADIO_PLL_PORT, RADIO_PLL_LAT_PIN);
    LL_mDelay(2);

    for (int i = 16; i >= 0; i--) 
    {
        bool bit_val = (packet >> i) & 1;

        if (bit_val)
            LL_GPIO_SetOutputPin(RADIO_PLL_PORT, RADIO_PLL_DAT_PIN);
        else
            LL_GPIO_ResetOutputPin(RADIO_PLL_PORT, RADIO_PLL_DAT_PIN);
        LL_mDelay(1);

        LL_GPIO_SetOutputPin(RADIO_PLL_PORT, RADIO_PLL_CLK_PIN);
        LL_mDelay(1); 

        LL_GPIO_ResetOutputPin(RADIO_PLL_PORT, RADIO_PLL_CLK_PIN);
        LL_mDelay(1); 
    }
}

// Init timers for frequency measurement
void radio_ctrl_init_timers(void)
{
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM2);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM3);

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = RADIO_PLL_FREQ_PIN;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate = RADIO_PLL_FREQ_AF;
    LL_GPIO_Init(RADIO_PLL_FREQ_PORT, &GPIO_InitStruct);

    LL_TIM_SetPrescaler(RADIO_COUNTING_TIM, 0);
    LL_TIM_SetAutoReload(RADIO_COUNTING_TIM, 0xFFFFFFFF);
    LL_TIM_SetClockSource(RADIO_COUNTING_TIM, LL_TIM_CLOCKSOURCE_EXT_MODE2);

    // Use ETR (External Clock Mode 2)
    LL_TIM_ConfigETR(
        RADIO_COUNTING_TIM, LL_TIM_ETR_POLARITY_NONINVERTED, LL_TIM_ETR_PRESCALER_DIV1, LL_TIM_ETR_FILTER_FDIV1);

    LL_TIM_SetTriggerInput(RADIO_COUNTING_TIM, LL_TIM_TS_ITR2);
    
    // Sounting when RADIO_MASTER_TIM is high
    LL_TIM_SetSlaveMode(RADIO_COUNTING_TIM, LL_TIM_SLAVEMODE_GATED);
    
    // Use CH1 to capture slave tim value
    LL_TIM_IC_SetActiveInput(RADIO_COUNTING_TIM, LL_TIM_CHANNEL_CH1, LL_TIM_ACTIVEINPUT_TRC);
    LL_TIM_IC_SetPolarity(RADIO_COUNTING_TIM, LL_TIM_CHANNEL_CH1, LL_TIM_IC_POLARITY_FALLING);
    LL_TIM_CC_EnableChannel(RADIO_COUNTING_TIM, LL_TIM_CHANNEL_CH1);

    LL_TIM_EnableCounter(RADIO_COUNTING_TIM);

    //master
    LL_TIM_SetPrescaler(RADIO_MASTER_TIM, 72 - 1); //1mhz
    
    LL_TIM_SetAutoReload(RADIO_MASTER_TIM, RADIO_FREQ_MEAS_TIMEBASE_US); 
    LL_TIM_OC_SetCompareCH1(RADIO_MASTER_TIM, RADIO_FREQ_MEAS_TIMEBASE_US);

    LL_TIM_OC_SetMode(RADIO_MASTER_TIM, LL_TIM_CHANNEL_CH1, LL_TIM_OCMODE_PWM1);
    
    LL_TIM_SetOnePulseMode(RADIO_MASTER_TIM, LL_TIM_ONEPULSEMODE_SINGLE);
    LL_TIM_SetTriggerOutput(RADIO_MASTER_TIM, LL_TIM_TRGO_OC1REF);
}

void radio_start_measure_freq(void)
{
    if (LL_TIM_IsEnabledCounter(RADIO_MASTER_TIM))
        return;
    
    LL_TIM_SetCounter(RADIO_COUNTING_TIM, 0);
    LL_TIM_SetCounter(RADIO_MASTER_TIM, 0);
    LL_TIM_EnableCounter(RADIO_MASTER_TIM);//master
    
    radio_freq_measurement_running = true;
}

/// Get RX frequeny in Hz, that was set
uint32_t radio_get_current_freq_hz(void)
{
    return radio_current_set_freq_hz;
}

char *radio_get_current_set_station_name(void)
{
    return radio_current_set_station_name;
}

bool radio_get_freq_lock_lost_state(void)
{
    return radio_freq_lock_lost_flag;
}

void radio_tune_step_up(void)
{
    if (radio_tune_mode == RADIO_TUNE_MODE_100K)
        radio_current_set_freq_hz += RADIO_TUNE_STEP_100KHZ;
    else if (radio_tune_mode == RADIO_TUNE_MODE_1M)
        radio_current_set_freq_hz += RADIO_TUNE_STEP_1MHZ;
    else if (radio_tune_mode == RADIO_TUNE_MODE_STATIONS)
    {
        int index = radio_ctrl_get_closest_station_index(radio_current_set_freq_hz);
        index++;
        if (index < radio_stations_count)
        {
            radio_set_new_frequency(radio_stations_array[index].frequency_hz);
            return;
        }
    }
    radio_set_new_frequency(radio_current_set_freq_hz);
}

void radio_tune_step_down(void)
{
    if (radio_tune_mode == RADIO_TUNE_MODE_100K)
        radio_current_set_freq_hz -= RADIO_TUNE_STEP_100KHZ;
    else if (radio_tune_mode == RADIO_TUNE_MODE_1M)
        radio_current_set_freq_hz -= RADIO_TUNE_STEP_1MHZ;
    else if (radio_tune_mode == RADIO_TUNE_MODE_STATIONS)
    {
        int index = radio_ctrl_get_closest_station_index(radio_current_set_freq_hz);
        if (index > 0)
        {
            index--;
            radio_set_new_frequency(radio_stations_array[index].frequency_hz);
            return;
        }
    }
    radio_set_new_frequency(radio_current_set_freq_hz);
}

void radio_tune_switch_mode(void)
{
    radio_tune_mode++;
    if (radio_tune_mode >= RADIO_TUNE_MODE_LAST)
        radio_tune_mode = RADIO_TUNE_MODE_100K;
}

radio_tune_mode_t radio_tune_get_mode(void)
{
    return radio_tune_mode;
}

char* radio_ctrl_get_station_name(uint32_t frequency_hz)
{
    size_t left = 0;
    size_t right = radio_stations_count;

    while (left < right) 
    {
        size_t mid = left + (right - left) / 2;
        uint32_t mid_freq = radio_stations_array[mid].frequency_hz;

        if (mid_freq == frequency_hz) 
        {
            return radio_stations_array[mid].name;
        } 
        else if (mid_freq < frequency_hz)
        {
            left = mid + 1;
        } 
        else 
        {
            right = mid;
        }
    }

    // Not found
    return NULL;
}

// Return index of he station from radio_stations_array[]
int radio_ctrl_get_closest_station_index(uint32_t frequency_hz)
{
    size_t left = 0;
    size_t right = radio_stations_count;

    // Binary search (lower bound)
    while (left < right) 
    {
        size_t mid = left + (right - left) / 2;
        uint32_t mid_freq = radio_stations_array[mid].frequency_hz;

        if (mid_freq < frequency_hz)
            left = mid + 1;
        else 
            right = mid;
    }

    if (left == 0)
        return 0;
    
    if (left == radio_stations_count)
    {
        return (int)(left - 1);
    }

    uint32_t diff_current = radio_stations_array[left].frequency_hz - frequency_hz;
    uint32_t diff_previous = frequency_hz - radio_stations_array[left - 1].frequency_hz;

    if (diff_current < diff_previous)
    {
        return (int)left;
    }
    else
    {
        return (int)(left - 1);
    }
}

/*
void Measure_Frequency(void)
{
    LL_TIM_SetCounter(RADIO_COUNTING_TIM, 0);
    LL_TIM_SetCounter(RADIO_MASTER_TIM, 0);
    LL_TIM_EnableCounter(RADIO_MASTER_TIM);//master

    while (LL_TIM_IsEnabledCounter(RADIO_MASTER_TIM)) {}
    
    LL_mDelay(5);

    //uint32_t pulses = LL_TIM_GetCounter(RADIO_COUNTING_TIM);
    uint32_t pulses = LL_TIM_OC_GetCompareCH1(TIM2); 
    measured_frequency = pulses * 1000 * 40 - RADIO_IF_FREQ_HZ;
}
*/
