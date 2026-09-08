#include "radio_menu.h"
#include "radio_ctrl.h"
#include "display_handling.h"
#include "stdio.h"
#include "config.h"

#define TUNE_DOWN_KEY_IDX   7
#define TUNE_UP_KEY_IDX     8

typedef enum
{
    RADIO_MENU_MAIN = 0,
    RADIO_MENU_MESSAGE,
} radio_menu_mode_t;


typedef struct
{
    void (*yes_callback)(void);
    void (*no_callback)(void);
} radio_menu_msg_t;

radio_menu_msg_t radio_menu_msg_obj = {0};

radio_menu_mode_t radio_menu_mode = RADIO_MENU_MAIN;


void radio_menu_activate_message(void);
void radio_menu_close_message(void);
void radio_menu_save_memory_yes(void);

//*****************************************

/// Called from keys event callback
void radio_menu_tune_pressed(uint8_t index)
{
    if (index == TUNE_DOWN_KEY_IDX)
        radio_tune_step_down();
    else if (index == TUNE_UP_KEY_IDX)
        radio_tune_step_up();
}

/// Called from keys event callback
void radio_menu_memory_hold(uint8_t index)
{
    char tmp_str[64];
    uint8_t mem_channel = index + 1;
    float freq_mgz = (float)radio_get_current_freq_hz() / 1000000.0f;
    sprintf(tmp_str, "SAVE %.1f > CH%d?", freq_mgz, mem_channel);
    
    radio_menu_msg_obj.yes_callback = radio_menu_save_memory_yes;
    radio_menu_msg_obj.no_callback = NULL;
    
    display_show_message(tmp_str, "YES", "NO");
    radio_menu_activate_message();
}

/// Called when button YES is pressed in Save To MEM menu
void radio_menu_save_memory_yes(void)
{

}

void radio_menu_activate_message(void)
{
    radio_menu_mode = RADIO_MENU_MESSAGE;
}

void radio_menu_close_message(void)
{
    radio_menu_mode = RADIO_MENU_MAIN;
    display_close_message();
}

//**********************************************************

void radio_menu_front2_pressed(uint8_t index)
{
    if (radio_menu_mode == RADIO_MENU_MESSAGE)
    {
        //YES button
        if (radio_menu_msg_obj.yes_callback != NULL)
            radio_menu_msg_obj.yes_callback();
        radio_menu_close_message();
    }
}

void radio_menu_front4_pressed(uint8_t index)
{
    if (radio_menu_mode == RADIO_MENU_MESSAGE)
    {
        //NO button
        if (radio_menu_msg_obj.no_callback != NULL)
            radio_menu_msg_obj.no_callback();
        radio_menu_close_message();
    }
}

