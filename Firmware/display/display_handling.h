#ifndef __DISPLAY_HANDLING_H
#define __DISPLAY_HANDLING_H

#include "stdint.h"
#include "string.h"
#include "ST7565R.h"
#include "fonts.h"
#include "stdbool.h"

void display_handling_init(void);
void display_handling_update(void);

void display_backlight_switch(bool new_state);

#endif

