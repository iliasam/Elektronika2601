//----------------------------------------------------------------------
//EASTRISING TECHNOLOGY CO,.LTD.
// Encoding  : UTF8
// Module    : ERC12864-1 Series
// Create    : Alex_EXE ( http://alex-exe.ru ) + iliasam
// Source    : JAVEN
// Date      : 2014-08-17
// Drive IC  : ST7565R
// INTERFACE : SPI
// VDD		 : 3.3V
//----------------------------------------------------------------------
#include "ST7565R.h"
#include "stm32f3xx_ll_spi.h"

#define LCD_HW_X_OFFSET     0
#define LCD_HW_WIDTH        128
#define LCD_HW_PAGES        4 // 32 / 8


unsigned char Contrast_level = 25;

void lcd_init_pins(void) 
{
  LL_GPIO_InitTypeDef GPIO_InitStructure = {0};

  GPIO_InitStructure.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStructure.Mode = LL_GPIO_MODE_OUTPUT;
  
  //GPIO_InitStructure.Pin = LCD_CLK_PIN | LCD_DAT_PIN;
  //LL_GPIO_Init(LCD_DATA_PORT, &GPIO_InitStructure);
  //LCD_SPI_NAME is initialized at the startup in MX_SPI1_Init()
  
  GPIO_InitStructure.Pin = LCD_RS_PIN;
  LL_GPIO_Init(LCD_RS_PORT, &GPIO_InitStructure);
  
  GPIO_InitStructure.Pin = LCD_CS_PIN;
  LL_GPIO_Init(LCD_CS_PORT, &GPIO_InitStructure);
  
  GPIO_InitStructure.Pin = LCD_RST_PIN;
  LL_GPIO_Init(LCD_RST_PORT, &GPIO_InitStructure);

  lcd_enable_power();
}

void lcd_enable_power(void)
{

}

void lcd_disable_power(void)
{

}


void lcd_delay(unsigned long p) 
{
  unsigned long i;
  for (i = 0; i < p; i++)
    ;
}

// Send data to the LCD
void lcd_write_data(unsigned char dat) 
{
  CS1_0;
  RS_1;//data
  lcd_delay(2);
    
  while(LL_SPI_IsActiveFlag_TXE(LCD_SPI_NAME) == 0) {}
  LL_SPI_TransmitData8(LCD_SPI_NAME, dat);
  while(LL_SPI_IsActiveFlag_BSY(LCD_SPI_NAME)) {}
  CS1_1;
}

// Send command to the LCD
void lcd_write_cmd(unsigned char cmd) 
{
  CS1_0;
  RS_0;//command
  lcd_delay(2);
  
  while(LL_SPI_IsActiveFlag_TXE(LCD_SPI_NAME) == 0) {}
  LL_SPI_TransmitData8(LCD_SPI_NAME, cmd);
  while(LL_SPI_IsActiveFlag_BSY(LCD_SPI_NAME)) {}
      
  CS1_1;
}

// Адрес первой строки дисплея
// Specify DDRAM line for COM0 0~63
void lcd_Initial_Dispay_Line(unsigned char line) 
{
	line |= 0x40;
	lcd_write_cmd(line);
}

//	Установка строки (заменена lcd_gotoxy)
// Set page address 0~15
void lcd_Set_Page_Address(unsigned char add) 
{
	add = ST7565R_SET_PAGE | add;
	lcd_write_cmd(add);
}

//	Устрановка столбца (заменена lcd_gotoxy)
void lcd_Set_Column_Address(unsigned char add) 
{
  lcd_write_cmd((0x10 | (add >> 4)));
  lcd_write_cmd((0x0f & add));
}

//	Управление питанием
//Power_Control   4 (internal converte ON) + 2 (internal regulor ON) + 1 (internal follower ON)
void lcd_Power_Control(unsigned char vol) {
  //16
  lcd_write_cmd((0x28 | vol));
}

//
void lcd_Regulor_Resistor_Select(unsigned char r) {
  //  Regulor resistor select
  //            1+Rb/Ra  Vo=(1+Rb/Ra)Vev    Vev=(1-(63-a)/162)Vref   2.1v
  //            0  3.0       4  5.0(default)
  //            1  3.5       5  5.5
  //            2  4         6  6
  //            3  4.5       7  6.4
  lcd_write_cmd((0x20 | r));
}

//	Установка контрастности дисплея
//	Input : mod - контрастность от 0 до 63
void lcd_Set_Contrast_Control_Register(unsigned char mod) {
  //a(0-63) 32default   Vev=(1-(63-a)/162)Vref   2.1v
  lcd_write_cmd(0x81);
  lcd_write_cmd(mod);
}

//	инициализация дисплея
void lcd_init(void)
{
  LL_SPI_Enable(LCD_SPI_NAME);  
  
  lcd_write_cmd(0xe2); //RESET();
  lcd_delay(100);

  CS1_0;
  lcd_write_cmd(0xe2);//RESET();
  RST_1;
  lcd_delay(100000);
  RST_0;
  lcd_delay(100000);
  RST_1;
    
  lcd_delay(100000);
#if (LCD_INVERT_MODE == 1)
  lcd_write_cmd(0x0a0);	 /* ADC set to reverse */
  lcd_write_cmd(0x0c8);	 /* common output mode */
#else
  lcd_write_cmd(0xa1); /* ADC set to reverse */
  lcd_write_cmd(0xc0); /* common output mode */
#endif


  lcd_write_cmd(0xa6); /* display normal, bit val 0: LCD pixel off. */
  lcd_write_cmd(0xa3); /* LCD bias 1/7 */
  
  lcd_write_cmd(0x028|4); /* all power  control circuits on */
  lcd_delay(1000);
  lcd_write_cmd(0x028|6); /* all power  control circuits on */
  lcd_delay(1000);
  lcd_write_cmd(0x028|7); /* all power  control circuits on */
  lcd_delay(1000);

  lcd_Initial_Dispay_Line(0);
  lcd_Regulor_Resistor_Select(3);
  lcd_Set_Contrast_Control_Register(Contrast_level);
  
  //Column and page are set in lcd_send_full_framebuffer() !!
  
  lcd_write_cmd(0xaf); //  Display on - ok
  
  lcd_delay(100000);
  lcd_write_cmd(0xa6); //  Инвертировать все точки - выкл - нормальный режим
  //lcd_write_cmd(0xa7);   //  Инвертировать все точки - вкл
  lcd_write_cmd(0xa4); //  Зажечь все точки - выкл - норальный режим
  //lcd_write_cmd(0xa5);   //  Зажечь все точки - вкл
  
}

//	очистка дисплея
void lcd_clear(void)
{
  unsigned char i, j;
  
  for (i = 0; i < 9; i++) //clear page 0~8
  {
    lcd_write_cmd(ST7565R_SET_PAGE + i); //set page
    lcd_write_cmd(0X00); //set column
    lcd_write_cmd(0X10); //set column
    for (j = 0; j < 132; j++) //clear all columns upto 130
    {
      lcd_write_data(0x00);
    }
  }
}


//Send data from framebuffer to LCD
void lcd_send_full_framebuffer(uint8_t* data)
{
  unsigned char i, j;
  
  for (i = 0; i < LCD_HW_PAGES; i++) //page 0~3
  {
    lcd_write_cmd(ST7565R_SET_PAGE + i); //set page
    lcd_write_cmd(0x10 + 0); //set column
    lcd_write_cmd(0x00 + LCD_HW_X_OFFSET); //set column
    
    //Start send page
    CS1_0;
    RS_1;//data
    lcd_delay(2);
      
    for (j = 0; j < LCD_HW_WIDTH; j++)
    {
      while(LL_SPI_IsActiveFlag_TXE(LCD_SPI_NAME) == 0) {}
      LL_SPI_TransmitData8(LCD_SPI_NAME, (uint8_t)*data);
      data++;
    }
    
    while(LL_SPI_IsActiveFlag_BSY(LCD_SPI_NAME)) {}
    CS1_1;
  }
}
