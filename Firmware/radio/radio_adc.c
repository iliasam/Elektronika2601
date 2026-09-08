#include "radio_adc.h"
#include "config.h"
#include "main.h"
#include "stdlib.h"
#include "math.h"
#include "stm32f3xx_ll_rcc.h"
#include "stm32f3xx_ll_bus.h"
#include "stm32f3xx_ll_utils.h"
#include "stm32f3xx_ll_gpio.h"
#include "stm32f3xx_ll_adc.h"
#include "stm32f3xx_ll_tim.h"

#define RADIO_ADC_BUFFER_SIZE 1024

#define RADIO_ADC_MEAS_PERIOD_MS        200//200ms


#define RADIO_ADC_SAMPLE_RATE_HZ        (125000)

#define RADIO_ADC_PILOT_TONE_HZ         (19100)

// Samplerate = 125kHz (72M / 125k)
#define RADIO_ADC_TIMER_PERIOD          (576 - 1)

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

uint32_t radio_adc_last_meas_timestamp_ms = 0;
uint16_t adc_buffer[RADIO_ADC_BUFFER_SIZE];

volatile float test_amp = 1.0f;

void radio_adc_timer_init(void);
void radio_adc_start_capture(void);
void radio_adc_init_hw(void);
void radio_adc_init_dma(void);

float goertzel_amplitude(void);

void radio_adc_init(void)
{
    LL_GPIO_SetPinMode(RADIO_ADC_PORT, RADIO_ADC_PIN, LL_GPIO_MODE_ANALOG);
    LL_GPIO_SetPinPull(RADIO_ADC_PORT, RADIO_ADC_PIN, LL_GPIO_PULL_NO);
    
    radio_adc_timer_init();
    radio_adc_init_hw();
    radio_adc_init_dma();
}


void radio_adc_timer_init(void) 
{
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM1);
    
    LL_TIM_SetPrescaler(RADIO_ADC_TIM, 0);
    LL_TIM_SetAutoReload(RADIO_ADC_TIM, RADIO_ADC_TIMER_PERIOD);
    LL_TIM_SetCounterMode(RADIO_ADC_TIM, LL_TIM_COUNTERMODE_UP);
    
    LL_TIM_SetTriggerOutput(RADIO_ADC_TIM, LL_TIM_TRGO_UPDATE);
}

void radio_adc_init_hw(void) 
{
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_ADC12);
    LL_ADC_SetCommonClock(__LL_ADC_COMMON_INSTANCE(RADIO_ADC_NAME), LL_ADC_CLOCK_SYNC_PCLK_DIV1);
    
    if (LL_ADC_IsEnabled(RADIO_ADC_NAME)) 
    {
        LL_ADC_Disable(RADIO_ADC_NAME);
    }
    
    //LL_ADC_DisableDeepPowerDown(RADIO_ADC_NAME);
    LL_ADC_EnableInternalRegulator(RADIO_ADC_NAME);
    
    // Программная задержка для стабилизации регулятора (~10 мкс)
    volatile uint32_t delay = 720;
    while(delay--);
    
    if (LL_ADC_IsEnabled(RADIO_ADC_NAME) == 0)
    {
        LL_ADC_StartCalibration(RADIO_ADC_NAME, LL_ADC_SINGLE_ENDED);
        while (LL_ADC_IsCalibrationOnGoing(RADIO_ADC_NAME) != 0);
    }
    
    LL_ADC_REG_InitTypeDef ADC_REG_InitStruct = {0};
    ADC_REG_InitStruct.TriggerSource    = LL_ADC_REG_TRIG_EXT_TIM1_TRGO; // Триггер RADIO_ADC_TIM TRGO
    ADC_REG_InitStruct.SequencerLength  = LL_ADC_REG_SEQ_SCAN_DISABLE;   // 1 канал в регулярной группе
    ADC_REG_InitStruct.SequencerDiscont = LL_ADC_REG_SEQ_DISCONT_DISABLE;
    ADC_REG_InitStruct.ContinuousMode   = LL_ADC_REG_CONV_SINGLE;        // Преобразование по триггеру (не Continuous)
    ADC_REG_InitStruct.DMATransfer      = LL_ADC_REG_DMA_TRANSFER_LIMITED; // Одиночный буфер DMA
    LL_ADC_REG_Init(RADIO_ADC_NAME, &ADC_REG_InitStruct);
    
    LL_ADC_REG_SetTriggerEdge(RADIO_ADC_NAME, LL_ADC_REG_TRIG_EXT_RISING); // Запуск по фронту TRGO
    
    LL_ADC_REG_SetSequencerRanks(RADIO_ADC_NAME, LL_ADC_REG_RANK_1, LL_ADC_CHANNEL_12);
    LL_ADC_SetChannelSamplingTime(RADIO_ADC_NAME, LL_ADC_CHANNEL_12, LL_ADC_SAMPLINGTIME_4CYCLES_5);
    
    LL_ADC_Enable(RADIO_ADC_NAME);
    while (!LL_ADC_IsActiveFlag_ADRDY(RADIO_ADC_NAME)); // Ожидание готовности АЦП
}

void radio_adc_init_dma(void) 
{
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA2);
    
    LL_DMA_ConfigTransfer(RADIO_ADC_DMA, RADIO_ADC_DMA_CH,
                          LL_DMA_DIRECTION_PERIPH_TO_MEMORY |
                          LL_DMA_MODE_NORMAL           |
                          LL_DMA_PERIPH_NOINCREMENT    |
                          LL_DMA_MEMORY_INCREMENT      |
                          LL_DMA_PDATAALIGN_HALFWORD   | 
                          LL_DMA_MDATAALIGN_HALFWORD   |
                          LL_DMA_PRIORITY_HIGH);
    
    LL_DMA_ConfigAddresses(RADIO_ADC_DMA, RADIO_ADC_DMA_CH,
                           LL_ADC_DMA_GetRegAddr(RADIO_ADC_NAME, LL_ADC_DMA_REG_REGULAR_DATA),
                           (uint32_t)adc_buffer,
                           LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    
    LL_DMA_SetDataLength(RADIO_ADC_DMA, RADIO_ADC_DMA_CH, RADIO_ADC_BUFFER_SIZE);
    
    // LL_DMA_EnableIT_TC(RADIO_ADC_DMA, RADIO_ADC_DMA_CH);
    // NVIC_EnableIRQ(DMA2_Channel1_IRQn);
}

void radio_adc_handling(void)
{
    static bool adc_dma_is_done = false;
    
    uint32_t adc_time_diff_ms = ms_tick - radio_adc_last_meas_timestamp_ms;
    if ((adc_time_diff_ms > RADIO_ADC_MEAS_PERIOD_MS) && adc_dma_is_done)
    {
        radio_adc_start_capture();
        radio_adc_last_meas_timestamp_ms = ms_tick;
    }
    else if (LL_DMA_IsActiveFlag_TC1(RADIO_ADC_DMA))
    {
        LL_DMA_ClearFlag_TC1(RADIO_ADC_DMA);
        adc_dma_is_done = true;
        test_amp = goertzel_amplitude();
    }
}

void radio_adc_start_capture(void) 
{
    LL_TIM_DisableCounter(RADIO_ADC_TIM);
    
    LL_DMA_DisableChannel(RADIO_ADC_DMA, RADIO_ADC_DMA_CH);
    while (LL_DMA_IsEnabledChannel(RADIO_ADC_DMA, RADIO_ADC_DMA_CH));
    LL_DMA_SetDataLength(RADIO_ADC_DMA, RADIO_ADC_DMA_CH, RADIO_ADC_BUFFER_SIZE);
    
    LL_DMA_ClearFlag_TC1(RADIO_ADC_DMA);
    LL_DMA_ClearFlag_TE1(RADIO_ADC_DMA);
    
    LL_DMA_EnableChannel(RADIO_ADC_DMA, RADIO_ADC_DMA_CH);
    LL_ADC_REG_StartConversion(RADIO_ADC_NAME);
    LL_TIM_SetCounter(RADIO_ADC_TIM, 0);
    LL_TIM_EnableCounter(RADIO_ADC_TIM);
}


float goertzel_amplitude(void)
{
    static float noise_level = 0.0f;
    static float ratio_filter = 0.0f;
    
    
    float k = (RADIO_ADC_BUFFER_SIZE * RADIO_ADC_PILOT_TONE_HZ) / RADIO_ADC_SAMPLE_RATE_HZ; 
    float omega = (2.0f * M_PI * k) / RADIO_ADC_BUFFER_SIZE;
    float coeff = 2.0f * cosf(omega);
    
    float k2 = (RADIO_ADC_BUFFER_SIZE * (RADIO_ADC_PILOT_TONE_HZ + 1500)) / RADIO_ADC_SAMPLE_RATE_HZ; 
    float omega2 = (2.0f * M_PI * k2) / RADIO_ADC_BUFFER_SIZE;
    float coeff2 = 2.0f * cosf(omega2);
    

    // Переменные состояния (задержки)
    float s_prev = 0.0f;
    float s_prev2 = 0.0f;
    
    float s_prev_2 = 0.0f;
    float s_prev2_2 = 0.0f;

    for (uint16_t i = 0; i < RADIO_ADC_BUFFER_SIZE; i++) 
    {
        float sample = (float)adc_buffer[i] - (ADC_MAX_POINTS / 2.0f);
        //float sample = (float)adc_buffer[i];

        float s = sample + coeff * s_prev - s_prev2;
        s_prev2 = s_prev;
        s_prev = s;
        
        float s_2 = sample + coeff2 * s_prev_2 - s_prev2_2;
        s_prev2_2 = s_prev_2;
        s_prev_2 = s_2;
    }

    float power = (s_prev * s_prev) + (s_prev2 * s_prev2) - (coeff * s_prev * s_prev2);
    float power2 = (s_prev_2 * s_prev_2) + (s_prev2_2 * s_prev2_2) - (coeff2 * s_prev_2 * s_prev2_2);

    float magnitude = (2.0f * sqrtf(power)) / RADIO_ADC_BUFFER_SIZE;
    float magnitude2 = (2.0f * sqrtf(power2)) / RADIO_ADC_BUFFER_SIZE;
    
    noise_level = 0.05f * (magnitude2 - noise_level) + noise_level;
    
    //return magnitude;
    
    if (noise_level == 0.0f)
        return 0.0f;
    
    float ratio = magnitude / noise_level;
    ratio_filter = 0.8f * (ratio - ratio_filter) + ratio_filter;
    
    return ratio_filter;
}

float radio_adc_get_level(void)
{
    float value_db = 20.0f * log10f(test_amp);
    if (value_db < 0.0f)
        return 0.0f; //for better look
    return value_db;
}

