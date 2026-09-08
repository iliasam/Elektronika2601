#ifndef __RADIO_MENU_H
#define __RADIO_MENU_H

#include <stdint.h>
#include "stm32f3xx.h"
#include "config.h"

void radio_menu_tune_pressed(uint8_t index);

void radio_menu_memory_hold(uint8_t index);
void radio_menu_memory_presed(uint8_t index);

void radio_menu_front2_pressed(uint8_t index);
void radio_menu_front4_pressed(uint8_t index);

#endif //__RADIO_MENU_H
