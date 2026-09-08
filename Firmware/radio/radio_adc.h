#ifndef __RADIO_ADC_H
#define __RADIO_ADC_H

#include <stdint.h>
#include "stm32f3xx.h"
#include "stdbool.h"
#include "config.h"

void radio_adc_init(void);
void radio_adc_start_capture(void);
void radio_adc_handling(void);
float radio_adc_get_level(void);


#endif
