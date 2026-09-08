#include "radio_menu.h"
#include "radio_ctrl.h"
#include "display_handling.h"
#include "stdio.h"
#include "config.h"
#include "nvram.h"

#define TUNE_DOWN_KEY_IDX   7
#define TUNE_UP_KEY_IDX     8

#define SAVE_TO_MEM_MSG_TIMEOUT_MS      4000
#define READ_MEM_MSG_TIMEOUT_MS         800

typedef enum
{
    RADIO_MENU_MAIN = 0,
    RADIO_MENU_MESSAGE,
} radio_menu_mode_t;


typedef struct
{
    void (*yes_callback)(void);
    void (*no_callback)(void);
    
    //0 - no timeout
    uint32_t timeout_timer_ms;
} radio_menu_msg_t;

radio_menu_msg_t radio_menu_msg_obj = {0};

radio_menu_mode_t radio_menu_mode = RADIO_MENU_MAIN;

/// Temporary value, used fo saving index of the pressed memory button, starts from 1
uint8_t radio_menu_mem_saving_index = 0;


void radio_menu_activate_message(void);
void radio_menu_close_message(void);
void radio_menu_save_memory_yes(void);
void radio_menu_display_warning(uint8_t index);

//*****************************************


void radio_menu_handling(void)
{
    if (radio_menu_mode == RADIO_MENU_MESSAGE)
    {
        //Leaving message mode
        if (radio_menu_msg_obj.timeout_timer_ms != 0)
        {
            if (TIMER_ELAPSED(radio_menu_msg_obj.timeout_timer_ms))
            {
                radio_menu_close_message();
            }
        }
    }
}


/// Called from keys event callback
void radio_menu_memory_hold(uint8_t index)
{
    char tmp_str[64];
    radio_menu_mem_saving_index = index + 1;
    float freq_mgz = (float)radio_get_current_freq_hz() / 1000000.0f;
    sprintf(tmp_str, "SAVE %.1f > CH%d?", freq_mgz, radio_menu_mem_saving_index);
    
    radio_menu_msg_obj.yes_callback = radio_menu_save_memory_yes;
    radio_menu_msg_obj.no_callback = NULL;
    START_TIMER(radio_menu_msg_obj.timeout_timer_ms, SAVE_TO_MEM_MSG_TIMEOUT_MS);
    
    display_show_message(tmp_str, "YES", "NO");
    radio_menu_activate_message();
}

/// Called when button YES is pressed in Save To MEM menu
void radio_menu_save_memory_yes(void)
{
    nvram_save_key_memory_freq(radio_get_current_freq_hz(), radio_menu_mem_saving_index);
}

void radio_menu_activate_message(void)
{
    radio_menu_mode = RADIO_MENU_MESSAGE;
}

void radio_menu_close_message(void)
{
    radio_menu_mode = RADIO_MENU_MAIN;
    display_close_message();
    radio_menu_msg_obj.timeout_timer_ms = 0;
}

//**********************************************************

/// Called from keys event callback
void radio_menu_tune_pressed(uint8_t index)
{
    if (index == TUNE_DOWN_KEY_IDX)
        radio_tune_step_down();
    else if (index == TUNE_UP_KEY_IDX)
        radio_tune_step_up();
}

// Read station from NVRAM
void radio_menu_memory_presed(uint8_t index)
{
    uint32_t new_freq_hz = nvram_read_key_memory_freq(index + 1);
    if ((new_freq_hz < 50e6) || (new_freq_hz > 120e6))
    {
        radio_menu_display_warning(index);
        return;
    }
    
    uint32_t cur_freq_hz = radio_get_current_freq_hz();
    if (new_freq_hz != cur_freq_hz)
        radio_set_new_frequency(new_freq_hz);
}

void radio_menu_display_warning(uint8_t index)
{
    char tmp_str[64];
    uint8_t tmp_index = index + 1;
    sprintf(tmp_str, "Bad freq. for CH%d!", tmp_index);
    
    radio_menu_msg_obj.yes_callback = NULL;
    radio_menu_msg_obj.no_callback = NULL;
    START_TIMER(radio_menu_msg_obj.timeout_timer_ms, READ_MEM_MSG_TIMEOUT_MS);
    
    display_show_message(tmp_str, NULL, NULL);
    radio_menu_activate_message();
}

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

