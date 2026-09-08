#ifndef __POWER_CTRL_H
#define __POWER_CTRL_H

#include <stdint.h>
#include "stm32f3xx.h"
#include "config.h"

void power_ctrl_init(void);
void power_ctrl_update(void);
float power_ctrl_get_batt_voltage(void);

#endif
