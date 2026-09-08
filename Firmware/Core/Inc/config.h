#ifndef _CONFIG_H
#define _CONFIG_H

#define ADC_REF_VOLT            (3.3f)
#define ADC_MAX_POINTS          (4095.0f)

#define LCD_INVERT_MODE	        1

#define LCD_SPI_NAME            SPI1

#define LCD_CLK_PIN				LL_GPIO_PIN_3//pb3
#define LCD_DAT_PIN				LL_GPIO_PIN_5//pb5
#define LCD_DATA_PORT 		    GPIOB

#define LCD_RS_PIN				LL_GPIO_PIN_15//pa15
#define LCD_RS_PORT 		    GPIOA

#define LCD_CS_PIN				LL_GPIO_PIN_12//pa12
#define LCD_CS_PORT 		    GPIOA

#define LCD_RST_PIN				LL_GPIO_PIN_11//pa11
#define LCD_RST_PORT 		    GPIOA

//Backlight
#define LCD_LED_PIN				LL_GPIO_PIN_0//pb0
#define LCD_LED_PORT 		    GPIOB


#define BATTERY_ADC_PIN		    LL_GPIO_PIN_1//pb1
#define BATTERY_ADC_PORT 	    GPIOB
#define BATTERY_ADC_NAME 	    ADC3
#define BATTERY_ADC_CH          LL_ADC_CHANNEL_1
// Resistor divider radio
#define BATTERY_ADC_DIV         (2.0f)

// **************************

//Memory buttons
#define RADIO_PLL_CLK_PIN       LL_GPIO_PIN_13 //pb13 - PLL pin 4
#define RADIO_PLL_DAT_PIN       LL_GPIO_PIN_14 //pb14 - PLL pin 5
#define RADIO_PLL_LAT_PIN       LL_GPIO_PIN_15 //pb15 - PLL pin 3
#define RADIO_PLL_PORT 		    GPIOB

// **************************

/// Number of memory buttons
#define MEM_BUTTONS_CNT         7

#define BUTTON_MEM1_PIN 	    LL_GPIO_PIN_7//pb7
#define BUTTON_MEM1_PORT 	    GPIOB

#define BUTTON_MEM2_PIN 	    LL_GPIO_PIN_8//pb8
#define BUTTON_MEM2_PORT 	    GPIOB

#define BUTTON_MEM3_PIN 	    LL_GPIO_PIN_9//pb9
#define BUTTON_MEM3_PORT 	    GPIOB

#define BUTTON_MEM4_PIN 	    LL_GPIO_PIN_14//pc14
#define BUTTON_MEM4_PORT 	    GPIOC

#define BUTTON_MEM5_PIN 	    LL_GPIO_PIN_15//pc15
#define BUTTON_MEM5_PORT 	    GPIOC

#define BUTTON_MEM6_PIN 	    LL_GPIO_PIN_0//pa0
#define BUTTON_MEM6_PORT 	    GPIOA

#define BUTTON_MEM7_PIN 	    LL_GPIO_PIN_1//pa1
#define BUTTON_MEM7_PORT 	    GPIOA

#define BUTTON_TUNE_LOW_PIN     LL_GPIO_PIN_13//pc13
#define BUTTON_TUNE_LOW_PORT    GPIOC

#define BUTTON_TUNE_HIGH_PIN    LL_GPIO_PIN_2//pa2
#define BUTTON_TUNE_HIGH_PORT   GPIOA

#define BUTTON_FRONT1_PIN       LL_GPIO_PIN_6//pb6
#define BUTTON_FRONT1_PORT      GPIOB

#define BUTTON_FRONT2_PIN       LL_GPIO_PIN_10//pa10
#define BUTTON_FRONT2_PORT      GPIOA

#define BUTTON_FRONT3_PIN       LL_GPIO_PIN_9//pa9
#define BUTTON_FRONT3_PORT      GPIOA

#define BUTTON_FRONT4_PIN       LL_GPIO_PIN_8//pa8
#define BUTTON_FRONT4_PORT      GPIOA

// *******************************************************

#define RADIO_PLL_FREQ_PIN      LL_GPIO_PIN_5//pa5
#define RADIO_PLL_FREQ_PORT     GPIOA
#define RADIO_PLL_FREQ_AF       LL_GPIO_AF_1


/// Timer that is counting external pulses
#define RADIO_COUNTING_TIM      TIM2

/// Timer that is generating timebase
#define RADIO_MASTER_TIM        TIM3


// *******************************

// Sound line
#define RADIO_ADC_PIN	        LL_GPIO_PIN_2//pb2
#define RADIO_ADC_PORT 	        GPIOB
#define RADIO_ADC_NAME 	        ADC2
#define RADIO_ADC_CH            LL_ADC_CHANNEL_12
#define RADIO_ADC_TIM           TIM1
#define RADIO_ADC_DMA           DMA2
#define RADIO_ADC_DMA_CH        LL_DMA_CHANNEL_1


#endif //_CONFIG_H
