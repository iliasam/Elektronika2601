#include "lcd_worker.h"

#include "display_handling.h"
#include "stm32f3xx_ll_utils.h"
#include "string.h"
#include "stdio.h"
#include "stdbool.h"
#include "power_ctrl.h"
#include "keys_controlling.h"
#include "radio_ctrl.h"
#include "radio_adc.h"
#include "main.h"

typedef struct
{
    bool is_active;
} display_msg_t;

display_msg_t display_msg_obj = {0};

void display_handling_draw_big_freq(uint8_t x, uint8_t y, uint32_t freq_value_hz);
void display_backlight_init(void);
void display_handling_draw_keys_test(void);

void display_handling_draw_main_menu(void);

// ***********************************************

void display_show_message(char *text, char *text_yes, char *text_no)
{
    display_msg_obj.is_active = true;
    
    lcd_clear_framebuffer();
    
    const uint32_t frame_height = 32-7;
    const uint32_t frame_width = 128 - 6;
    display_draw_emplty_rectangle(3, 4, frame_width, frame_height);
    display_draw_horizontal_line(4, 4 + frame_width, 4 + frame_height);
    display_draw_vertical_line(4 + frame_width, 4 + 1, 4 + frame_height);
    
    lcd_draw_utf8_string(text, 7, 7, FONT_SIZE_8, 0);
    
    lcd_draw_utf8_string(text_yes, 53, 21, FONT_SIZE_6, LCD_CENTER_X_FLAG);
    lcd_draw_utf8_string(text_no, 110, 21, FONT_SIZE_6, LCD_CENTER_X_FLAG);
    
    lcd_update();
}

void display_close_message(void)
{
    display_msg_obj.is_active = false;
}

void display_handling_init(void)
{
    lcd_init_pins();
    display_backlight_init();
    LL_mDelay(10);
    lcd_init();
    lcd_clear();
    
    //lcd_draw_string("1234", 3, 0, FONT_SIZE_18, 0);
    
    //display_handling_draw_big_freq(4, 3, 103.3f * 1e6);
    
    lcd_draw_string("RETRO FM", 4, FONT_SIZE_18 + 5, FONT_SIZE_8, 0);

    //display_draw_line(0);
    //display_draw_line(LCD_HEIGHT - 1);
    
    
    lcd_update();
}

void display_handling_update(void)
{
    if (display_msg_obj.is_active)
        return;
    
    display_handling_draw_main_menu();
}

void display_handling_draw_main_menu(void)
{
    static uint32_t disp_lock_timer = 0;
    static bool disp_lock_visible_flag = false;
    
    char tmp_str[16];
    lcd_clear_framebuffer();
    display_handling_draw_big_freq(4, 3, radio_get_current_freq_hz());
    char *station_name_p = radio_get_current_set_station_name();
    if (station_name_p != NULL)
        lcd_draw_utf8_string(station_name_p, 4, FONT_SIZE_18 + 5, FONT_SIZE_8, 0);
    
    //Draw battry voltage
    float batt_volt = power_ctrl_get_batt_voltage();
    sprintf(tmp_str, "%.1fV", batt_volt);
    lcd_draw_string(tmp_str, LCD_RIGHT_OFFSET - 16, 5, FONT_SIZE_6, 0);
    
    float rx_level = radio_adc_get_level();
    sprintf(tmp_str, "%d", (int16_t)rx_level);
    lcd_draw_string("SNR", LCD_RIGHT_OFFSET - 16, 16, FONT_SIZE_6, 0);
    lcd_draw_string(tmp_str, LCD_RIGHT_OFFSET - 15, FONT_SIZE_18 + 5, FONT_SIZE_8, 0);
    
    if (radio_get_freq_lock_lost_state())
    {
        if (TIMER_ELAPSED(disp_lock_timer))
        {
            START_TIMER(disp_lock_timer, 500);
            disp_lock_visible_flag = !disp_lock_visible_flag;
        }
        
        if (disp_lock_visible_flag)
            lcd_draw_string("NO LOCK!", 74, 4, FONT_SIZE_6, 0);      
    }
    else
    {
        disp_lock_visible_flag = false;
        START_TIMER(disp_lock_timer, 500);
    }
    
    lcd_update();
}

// Draw big frequency value
void display_handling_draw_big_freq(uint8_t x, uint8_t y, uint32_t freq_value_hz)
{
    uint32_t mhz_integer = (uint32_t)(freq_value_hz / 1000000);
    uint32_t mhz_dec = (uint32_t)(freq_value_hz / 100000) % 10;
    
    char tmp_str[16];
    uint32_t integer_part_len = sprintf(tmp_str, "%d", mhz_integer);
    lcd_draw_string(tmp_str, x, y, FONT_SIZE_18, 0);
    uint32_t new_x_pos = x + get_font_width(FONT_SIZE_18) * integer_part_len + 3;
    
    //Comma
    uint32_t comma_x_pos = new_x_pos - 4;
    display_draw_vertical_line(comma_x_pos + 1, y + FONT_SIZE_18 - 1, y + FONT_SIZE_18 + 1);
    display_draw_vertical_line(comma_x_pos + 2, y + FONT_SIZE_18 - 2, y + FONT_SIZE_18 + 2);
    display_draw_vertical_line(comma_x_pos + 3, y + FONT_SIZE_18 - 1, y + FONT_SIZE_18 + 1);
    
    memset(tmp_str, 0, sizeof(tmp_str));
    sprintf(tmp_str, "%d", mhz_dec);
    lcd_draw_string(tmp_str, new_x_pos, y, FONT_SIZE_18, 0);
    
    lcd_draw_string("MHz", new_x_pos + get_font_width(FONT_SIZE_18) + 1, y + 7, FONT_SIZE_11, 0);
    
}

void display_backlight_init(void)
{
  LL_GPIO_InitTypeDef GPIO_InitStructure = {0};

  GPIO_InitStructure.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStructure.Mode = LL_GPIO_MODE_OUTPUT;

  GPIO_InitStructure.Pin = LCD_LED_PIN;
  LL_GPIO_Init(LCD_LED_PORT, &GPIO_InitStructure);
  
  LL_GPIO_SetOutputPin(LCD_LED_PORT, LCD_LED_PIN);
}

void display_backlight_switch(bool new_state)
{
    if (new_state)
        LL_GPIO_SetOutputPin(LCD_LED_PORT, LCD_LED_PIN);
    else
        LL_GPIO_ResetOutputPin(LCD_LED_PORT, LCD_LED_PIN);
}


void display_handling_draw_keys_test(void)
{
    char tmp_str[16];
    for (int i = 0; i < 9; i++)
    {
        sprintf(tmp_str, "%d", i + 1);
        lcd_draw_string(tmp_str, i*FONT_SIZE_8_WIDTH*2, 5, FONT_SIZE_8, 0);
        
        memset(tmp_str, 0, sizeof(tmp_str));
        int state = keys_get_current_state(i);
        sprintf(tmp_str, "%d", (uint16_t)state);
        lcd_draw_string(tmp_str, i*FONT_SIZE_8_WIDTH*2, 5+9, FONT_SIZE_8, 0);
    }
}

