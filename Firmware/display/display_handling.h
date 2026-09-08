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

void display_show_message(char *text, char *text_yes, char *text_no);
void display_close_message(void);

#endif

