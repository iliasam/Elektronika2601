#ifndef __RADIO_CTRL_H
#define __RADIO_CTRL_H

#include <stdint.h>
#include "stm32f3xx.h"
#include "stdbool.h"
#include "config.h"

typedef struct 
{
    uint32_t frequency_hz;
    
    char *name;
} radio_stations_t;


void radio_ctrl_init(void);
void radio_ctrl_handling(void);

uint32_t radio_get_current_freq_hz(void);
bool radio_get_freq_lock_lost_state(void);

void radio_set_new_frequency(uint32_t new_freq_hz);

void radio_tune_step_up(void);
void radio_tune_step_down(void);

char* radio_ctrl_get_station_name(uint32_t frequency_hz);
char *radio_get_current_set_station_name(void);

#endif
