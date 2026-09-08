#include "power_ctrl.h"
#include "config.h"
#include "stm32f3xx_ll_adc.h"
#include "stm32f3xx_ll_rcc.h"
#include "stm32f3xx_ll_bus.h"
#include "stm32f3xx_ll_utils.h"
#include "stm32f3xx_ll_gpio.h"


// In volts
float power_ctrl_bat_adc_v = 1.0f;

void power_ctrl_init_adc(void);
void power_ctrl_measure_batt_volatge(void);
void power_ctrl_measure_batt_volatge(void);

//*****************************************


void power_ctrl_init(void)
{
    power_ctrl_init_adc();
}

void power_ctrl_update(void)
{
    power_ctrl_measure_batt_volatge();
}


void power_ctrl_measure_batt_volatge(void)
{
    LL_ADC_Enable(ADC3);
    while (LL_ADC_IsActiveFlag_ADRDY(ADC3) == 0);
    
    LL_ADC_REG_StartConversion(ADC3);
    while (LL_ADC_IsActiveFlag_EOC(ADC3) == 0);
    uint16_t adc_value = LL_ADC_REG_ReadConversionData12(ADC3);
    
    float adc_points = (float)adc_value * BATTERY_ADC_DIV;
    power_ctrl_bat_adc_v = adc_points * ADC_REF_VOLT / ADC_MAX_POINTS;
    
    if (LL_ADC_REG_IsConversionOngoing(ADC3) != 0)
    {
        LL_ADC_REG_StopConversion(ADC3);
        while (LL_ADC_REG_IsStopConversionOngoing(ADC3) != 0);
    }
    
    LL_ADC_ClearFlag_ADRDY(ADC3);
    LL_ADC_ClearFlag_EOC(ADC3);
    
    LL_ADC_Disable(ADC3);
    while (LL_ADC_IsEnabled(ADC3) != 0);
}

float power_ctrl_get_batt_voltage(void)
{
    return power_ctrl_bat_adc_v;
}

void power_ctrl_init_adc(void)
{
    LL_ADC_InitTypeDef ADC_InitStruct = {0};
    LL_ADC_REG_InitTypeDef ADC_REG_InitStruct = {0};

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_ADC34);

    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);

    GPIO_InitStruct.Pin = BATTERY_ADC_PIN;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(BATTERY_ADC_PORT, &GPIO_InitStruct);


    ADC_InitStruct.Resolution = LL_ADC_RESOLUTION_12B;
    ADC_InitStruct.DataAlignment = LL_ADC_DATA_ALIGN_RIGHT;
    ADC_InitStruct.LowPowerMode = LL_ADC_LP_MODE_NONE;
    LL_ADC_Init(ADC2, &ADC_InitStruct);
    ADC_REG_InitStruct.TriggerSource = LL_ADC_REG_TRIG_SOFTWARE;
    ADC_REG_InitStruct.SequencerLength = LL_ADC_REG_SEQ_SCAN_DISABLE;
    ADC_REG_InitStruct.SequencerDiscont = LL_ADC_REG_SEQ_DISCONT_DISABLE;
    ADC_REG_InitStruct.ContinuousMode = LL_ADC_REG_CONV_SINGLE;
    ADC_REG_InitStruct.DMATransfer = LL_ADC_REG_DMA_TRANSFER_LIMITED;
    ADC_REG_InitStruct.Overrun = LL_ADC_REG_OVR_DATA_OVERWRITTEN;
    LL_ADC_REG_Init(BATTERY_ADC_NAME, &ADC_REG_InitStruct);
    LL_ADC_SetCommonClock(__LL_ADC_COMMON_INSTANCE(BATTERY_ADC_NAME), LL_ADC_CLOCK_SYNC_PCLK_DIV4);

    LL_ADC_EnableInternalRegulator(BATTERY_ADC_NAME);

    LL_mDelay(1);

    LL_ADC_REG_SetSequencerRanks(BATTERY_ADC_NAME, LL_ADC_REG_RANK_1, BATTERY_ADC_CH);
    LL_ADC_SetChannelSamplingTime(BATTERY_ADC_NAME, BATTERY_ADC_CH, LL_ADC_SAMPLINGTIME_181CYCLES_5);
    LL_ADC_SetChannelSingleDiff(BATTERY_ADC_NAME, BATTERY_ADC_CH, LL_ADC_SINGLE_ENDED);

    if (LL_ADC_IsEnabled(BATTERY_ADC_NAME) == 0)
    {
        LL_ADC_StartCalibration(BATTERY_ADC_NAME, LL_ADC_SINGLE_ENDED);
        while (LL_ADC_IsCalibrationOnGoing(BATTERY_ADC_NAME) != 0);
    }
}
