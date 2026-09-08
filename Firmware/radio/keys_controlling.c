/* Includes ------------------------------------------------------------------*/
#include "keys_controlling.h"
#include "stm32f3xx_ll_gpio.h"
#include "main.h"
#include "string.h"
#include "radio_ctrl.h"
#include "display_handling.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
//Time in ms
#define KEY_HOLD_TIME            900

//Time in ms
#define KEY_PRESSED_TIME         50

//Time in ms
#define KEY_RELEASE_TIME         50

//Time in ms
#define KEYS_STARTUP_DELAY      500

#define BUTTONS_COUNT           (7 + 4 + 2)

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
key_item_t key_down;
key_item_t key_up;


key_item_t key_items[BUTTONS_COUNT] = {0};



uint32_t keys_startup_timer = 0;
uint8_t keys_startup_lock_flag = 1;

extern volatile uint32_t ms_tick;

/* Private function prototypes -----------------------------------------------*/
void keys_tune_pressed(uint8_t index);
void keys_backlight_pressed(uint8_t index);

/* Private functions ---------------------------------------------------------*/

void keys_init(void)
{
    key_items[0].pin_name  =    BUTTON_MEM1_PIN;
    key_items[0].gpio_name =    BUTTON_MEM1_PORT;
    
    key_items[1].pin_name  =    BUTTON_MEM2_PIN;
    key_items[1].gpio_name =    BUTTON_MEM2_PORT;
    
    key_items[2].pin_name  =    BUTTON_MEM3_PIN;
    key_items[2].gpio_name =    BUTTON_MEM3_PORT;

    key_items[3].pin_name  =    BUTTON_MEM4_PIN;
    key_items[3].gpio_name =    BUTTON_MEM4_PORT;

    key_items[4].pin_name  =    BUTTON_MEM5_PIN;
    key_items[4].gpio_name =    BUTTON_MEM5_PORT;

    key_items[5].pin_name  =    BUTTON_MEM6_PIN;
    key_items[5].gpio_name =    BUTTON_MEM6_PORT;

    key_items[6].pin_name  =    BUTTON_MEM7_PIN;
    key_items[6].gpio_name =    BUTTON_MEM7_PORT;

    key_items[7].pin_name  =    BUTTON_TUNE_LOW_PIN;
    key_items[7].gpio_name =    BUTTON_TUNE_LOW_PORT;
    key_items[7].pressed_event_callback = keys_tune_pressed;
    
    key_items[8].pin_name  =    BUTTON_TUNE_HIGH_PIN;
    key_items[8].gpio_name =    BUTTON_TUNE_HIGH_PORT;
    key_items[8].pressed_event_callback = keys_tune_pressed;
    
    key_items[9].pin_name   =   BUTTON_FRONT1_PIN;
    key_items[9].gpio_name  =   BUTTON_FRONT1_PORT;
    key_items[9].pressed_event_callback = keys_backlight_pressed;

    key_items[10].pin_name  =   BUTTON_FRONT2_PIN;
    key_items[10].gpio_name =   BUTTON_FRONT2_PORT;

    key_items[11].pin_name  =   BUTTON_FRONT3_PIN;
    key_items[11].gpio_name =   BUTTON_FRONT3_PORT;

    key_items[12].pin_name  =   BUTTON_FRONT4_PIN;
    key_items[12].gpio_name =   BUTTON_FRONT4_PORT;  
    
    
    for (int i = 0; i < BUTTONS_COUNT; i++)
    {
        key_items[i].key_index = i;
        keys_functons_init_hardware(&key_items[i]);
    }

    START_TIMER(keys_startup_timer, KEYS_STARTUP_DELAY);
}



void key_handling(void)
{
    for (int i = 0; i < BUTTONS_COUNT; i++)
    {
        keys_functons_update_key_state(&key_items[i]);
    }
    
    if (TIMER_ELAPSED(keys_startup_timer) == 0)
        return; //delay before startup
    else
    {
        keys_startup_lock_flag = 0;
    }
    
    /*
  
  if ((key_down.prev_state == KEY_PRESSED) && 
      (key_down.state == KEY_WAIT_FOR_RELEASE))
  {

  }
  
  if ((key_up.prev_state == KEY_PRESSED) && 
      (key_up.state == KEY_WAIT_FOR_RELEASE))
  {

  }
  
  if ((key_up.prev_state == KEY_PRESSED) && 
      (key_up.state == KEY_HOLD))
  {

  }
  
  */

}

//*****************************************************************************

// Initialize single key pin
void keys_functons_init_hardware(key_item_t* key_item)
{
    if (key_item == NULL)
        return;

    LL_GPIO_InitTypeDef GPIO_InitStructure = {0};

    GPIO_InitStructure.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStructure.Mode = LL_GPIO_MODE_INPUT;
    GPIO_InitStructure.Pull = LL_GPIO_PULL_UP;

    GPIO_InitStructure.Pin = key_item->pin_name;
    LL_GPIO_Init(key_item->gpio_name, &GPIO_InitStructure);

    key_item->state = KEY_RELEASED;
}

void keys_functons_update_key_state(key_item_t* key_item)
{
  key_item->prev_state = key_item->state;
  
  if ((key_item->gpio_name->IDR & key_item->pin_name) == 0) //pressed
    key_item->current_state = 1;
  else
    key_item->current_state = 0;
  
  if ((key_item->state == KEY_RELEASED) && (key_item->current_state != 0))
  {
    //key presed now
    key_item->state = KEY_PRESSED_WAIT;
    key_item->key_timestamp = ms_tick;
    return;
  }
  
  if (key_item->state == KEY_PRESSED_WAIT)
  {
    uint32_t delta_time = ms_tick - key_item->key_timestamp;
    if (delta_time > KEY_PRESSED_TIME)
    {
      if (key_item->current_state != 0)
      {
          key_item->state = KEY_PRESSED;
          if (key_item->pressed_event_callback != NULL)
              key_item->pressed_event_callback(key_item->key_index);
      }
      else
        key_item->state = KEY_RELEASED;
    }
    return;
  }
  
  if ((key_item->state == KEY_PRESSED) || (key_item->state == KEY_HOLD))
  {
    // key not pressed
    if (key_item->current_state == 0)
    {
      key_item->state = KEY_WAIT_FOR_RELEASE;// key is locked here
      key_item->key_timestamp = ms_tick;
      return;
    }
  }
  
  if (key_item->state == KEY_WAIT_FOR_RELEASE)
  {
    uint32_t delta_time = ms_tick - key_item->key_timestamp;
    if (delta_time > KEY_RELEASE_TIME)
    {
      key_item->state = KEY_RELEASED;
      return;
    }
  }
  
  if ((key_item->state == KEY_PRESSED) && (key_item->current_state != 0))
  {
    //key still presed now
    uint32_t delta_time = ms_tick - key_item->key_timestamp;
    if (delta_time > KEY_HOLD_TIME)
    {
      key_item->state = KEY_HOLD;
      return;
    }
  }
}

//Return state of a certain key, (1 is pressed)
int keys_get_current_state(int index)
{
    if (index >= BUTTONS_COUNT)
    return 0;
    
    return (int)key_items[index].current_state;
}

// *************************************************

void keys_tune_pressed(uint8_t index)
{
    if (index == 7)
        radio_tune_step_down();
    else if (index == 8)
        radio_tune_step_up();
}

void keys_backlight_pressed(uint8_t index)
{
    static bool backlight_state = true;
    
    backlight_state = !backlight_state;
    display_backlight_switch(backlight_state);
}
