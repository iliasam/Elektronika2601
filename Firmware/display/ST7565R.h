#ifndef __ST7565R_H
#define __ST7565R_H

#include <stdint.h>
#include "stm32f3xx.h"
#include "config.h"
//#include <stm32f30x_rcc.h>
#include "stm32f3xx_ll_gpio.h"


#define RS_0					LL_GPIO_ResetOutputPin(LCD_RS_PORT, LCD_RS_PIN);
#define RS_1					LL_GPIO_SetOutputPin(LCD_RS_PORT, LCD_RS_PIN);

#define CS1_0					LL_GPIO_ResetOutputPin(LCD_CS_PORT, LCD_CS_PIN);
#define CS1_1					LL_GPIO_SetOutputPin(LCD_CS_PORT, LCD_CS_PIN);

#define RST_0					LL_GPIO_ResetOutputPin(LCD_RST_PORT, LCD_RST_PIN);
#define RST_1					LL_GPIO_SetOutputPin(LCD_RST_PORT, LCD_RST_PIN);


#define ST7565R_SET_PAGE        0xB0

//	инициализация выводов МК для работы с дисплеем
void lcd_init_pins(void);
//	задержка
void lcd_delay(unsigned long p);	//	задержка
#define DELAY_NOP				lcd_delay(10);

//	отправить данные на дисплей
void lcd_write_data(unsigned char dat);
//	отправить команду на дисплей
void lcd_write_cmd(unsigned char cmd);
//	Адрес первой строки дисплея
void lcd_Initial_Dispay_Line(unsigned char line);
//	Установка строки (заменена lcd_gotoxy)
void lcd_Set_Page_Address(unsigned char add);
//	Устрановка столбца (заменена lcd_gotoxy)
void lcd_Set_Column_Address(unsigned char add);
//	Управление питанием
void lcd_Power_Control(unsigned char vol);
//	Установка контрастности дисплея
//	Input : mod - контрастность от 0 до 63
void lcd_Set_Contrast_Control_Register(unsigned char mod);
void lcd_Regulor_Resistor_Select(unsigned char r);

void lcd_enable_power(void);
void lcd_disable_power(void);

//	инициализация дисплея
void lcd_init(void);

//	очистка дисплея
void lcd_clear(void);
//	Установка курсора
// 	Input : x,y - координаты символа
void lcd_gotoxy(unsigned char x,unsigned char y);



void lcd_send_full_framebuffer(uint8_t* data);

#endif
