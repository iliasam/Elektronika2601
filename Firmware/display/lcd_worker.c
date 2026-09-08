//Special framebuffer wrapper used for basic operations - text drawing
#include "lcd_worker.h"

uint16_t lcd_cursor_text_x = 0;
uint16_t lcd_cursor_text_y = 0;

void lcd_draw_char_size8(uint8_t chr, uint16_t x_start, uint16_t y_start, uint8_t flags);
void lcd_draw_char_size6(uint8_t chr, uint16_t x_start, uint16_t y_start, uint8_t flags);
void lcd_draw_char_size11(uint8_t chr, uint16_t x_start, uint16_t y_start, uint8_t flags);
void lcd_draw_char_size18(uint8_t chr, uint16_t x_start, uint16_t y_start, uint8_t flags);


uint8_t lcd_framebuffer[LCD_WIDTH*LCD_HEIGHT / 8];

void lcd_set_pixel(uint16_t x, uint16_t y)
{
  uint16_t loc_x = x + LCD_LEFT_OFFSET;
  if (loc_x >= LCD_WIDTH)
    return;
  if (loc_x >= LCD_RIGHT_OFFSET)
    return;
  
  uint16_t loc_y = y / 8;//8 - size of lcd line
  uint32_t byte_pos = loc_y * LCD_WIDTH + loc_x;
  uint8_t byte_val = 1 << ((y % 8));
  lcd_framebuffer[byte_pos]|= byte_val;
}

void lcd_reset_pixel(uint16_t x, uint16_t y)
{
  uint16_t loc_x = x + LCD_LEFT_OFFSET;
  if (loc_x >= LCD_WIDTH)
    return;
  if (loc_x >= LCD_RIGHT_OFFSET)
    return;
  
  uint16_t loc_y = y / 8;//8 - size of lcd line
  uint32_t byte_pos = loc_y * LCD_WIDTH + loc_x;
  uint8_t byte_val = 1 << ((y % 8));
  lcd_framebuffer[byte_pos]&= ~byte_val;
}

void lcd_clear_framebuffer(void)
{
  memset(lcd_framebuffer, 0, sizeof(lcd_framebuffer));
  lcd_cursor_text_x = 0;
  lcd_cursor_text_y = 0;
}

void lcd_full_clear(void)
{
  memset(lcd_framebuffer, 0, sizeof(lcd_framebuffer));
  lcd_clear();
  lcd_cursor_text_x = 0;
  lcd_cursor_text_y = 0;
}

void lcd_set_cursor_pos(uint16_t x, uint16_t y)
{
  lcd_cursor_text_x = x;
  lcd_cursor_text_y = y;
}

void lcd_update(void)
{
  lcd_send_full_framebuffer(lcd_framebuffer);
}

//x, y - in pixel
//return string width
//String end is 0x00 char
uint16_t lcd_draw_string(char *s, uint16_t x, uint16_t y, uint8_t font_size, uint8_t flags)
{
  uint16_t font_width = get_font_width(font_size);
  uint8_t chr_pos = 0;
  char chr = *s;
    
  if (flags & LCD_CENTER_X_FLAG)
  {
    uint8_t length =  strlen(s);
    uint16_t str_width = length * font_width;
    int16_t start_x = (int16_t)x - (str_width / 2);
    if (start_x < 0)
      start_x = 0;
    x = (uint16_t)start_x;
  }
  
  while (chr && (chr_pos < 50)) 
  {
    lcd_draw_char(chr, x + chr_pos * font_width, y, font_size, flags);
    chr_pos++;
    chr = s[chr_pos];
  }
  lcd_cursor_text_x = x + chr_pos * font_width;
  lcd_cursor_text_y = y;
  
  return chr_pos * font_width;
}

uint16_t lcd_draw_utf8_string(char *s, uint16_t x, uint16_t y, uint8_t font_size, uint8_t flags)
{
  char tmp_buf[64];
  memset(tmp_buf, 0, sizeof(tmp_buf));
  utf8_to_cp1251((uint8_t *)s, (uint8_t *)tmp_buf, 64, 64);
  return lcd_draw_string(tmp_buf, x, y, font_size, flags);
}

//Draw text at current cursor position
//String end is 0x00 char
uint16_t lcd_draw_string_cur(char *s, uint8_t font_size, uint8_t flags)
{
  uint16_t length = lcd_draw_string(s, lcd_cursor_text_x, lcd_cursor_text_y, font_size, flags);
  lcd_cursor_text_x+= length;
  if ((flags & LCD_NEW_LINE_FLAG) != 0)
  {
    lcd_cursor_text_y+= font_size;
    lcd_cursor_text_x = 0;
  }
    
  return length;
}

//x - size in pixel
//y - in pixel
//font_size - height
void lcd_draw_char(uint8_t chr, uint16_t x, uint16_t y, uint8_t font_size, uint8_t flags)
{
  switch (font_size)
  {
    case FONT_SIZE_8:
    {
      lcd_draw_char_size8(chr, x, y, flags);
      break;
    }
    case FONT_SIZE_6:
    {
      lcd_draw_char_size6(chr, x, y, flags);
      break;
    }
    case FONT_SIZE_11:
    {
      lcd_draw_char_size11(chr, x, y, flags);
      break;
    }
    case FONT_SIZE_18:
    {
      lcd_draw_char_size18(chr, x, y, flags);
      break;
    }
  }
}

//x, y - size in pixel
void lcd_draw_char_size8(uint8_t chr, uint16_t x_start, uint16_t y_start, uint8_t flags)
{
  uint16_t x_pos, y_pos;
  
  //decoding symbol
  if (chr >= 32 && chr <= '~')
  {
    chr = chr - 32;
  } 
  else
  {
    if (chr >= 192)
      chr = chr - 97;
    else
    {
        /*
      if (chr == SYMB_MICRO_CODE)
        chr = FONT8_TABLE_LENGTH - 1;
      else if (chr == SYMB_PI_CODE)
        chr = FONT8_TABLE_LENGTH - 2;
        return;
        */
        return;
    }
      
  }
  
  for (x_pos = 0; x_pos < (FONT_SIZE_8_WIDTH); x_pos++)
  {
    for (y_pos = 0; y_pos < FONT_SIZE_8; y_pos++)
    {
      uint8_t pixel = lcd_font_size8[chr][x_pos] & (1<<y_pos);
      if (x_pos == (FONT_SIZE_8_WIDTH-1))
        pixel = 0;
      
      if (flags & LCD_INVERTED_FLAG) 
        if (pixel) pixel = 0; else pixel = 1;
      
      if (pixel) 
        lcd_set_pixel(x_start + x_pos, y_start + y_pos);
      else
        lcd_reset_pixel(x_start + x_pos, y_start + y_pos);
    }
  }
}

void lcd_draw_char_size6(uint8_t chr, uint16_t x_start, uint16_t y_start, uint8_t flags)
{
  uint16_t x_pos, y_pos;
  
  for (x_pos = 0; x_pos < (FONT_SIZE_6_WIDTH); x_pos++)
  {
    for (y_pos = 0; y_pos < FONT_SIZE_6; y_pos++)
    {
      uint8_t pixel = lcd_font_size6[chr][y_pos] & (1<<(3-x_pos));
      
      if (flags & LCD_INVERTED_FLAG) 
        if (pixel) pixel = 0; else pixel = 1;
      
      if (pixel) 
        lcd_set_pixel(x_start + x_pos, y_start + y_pos);
      else
        lcd_reset_pixel(x_start + x_pos, y_start + y_pos);
    }
  }
}

//x, y - size in pixel
void lcd_draw_char_size11(uint8_t chr, uint16_t x_start, uint16_t y_start, uint8_t flags)
{
  uint16_t x_pos, y_pos;
  
  //decoding symbol
  if (chr >= 32 && chr <= 128)
  {
    chr = chr - 32;
  } 
  
  for (x_pos = 0; x_pos < (FONT_SIZE_11_WIDTH - 1); x_pos++)
  {
    for (y_pos = 0; y_pos < (FONT_SIZE_11-1); y_pos++)
    {
      if (lcd_font_size11[chr][y_pos] & (1<<(x_pos))) 
        lcd_set_pixel(x_start + x_pos, y_start + y_pos);
      else
        lcd_reset_pixel(x_start + x_pos, y_start + y_pos);
    }
  }
}

//x, y - size in pixel
void lcd_draw_char_size18(uint8_t chr, uint16_t x_start, uint16_t y_start, uint8_t flags)
{
  uint16_t x_pos, y_pos;
  
  //decoding symbol
  if (chr >= 32 && chr <= 128)
  {
    chr = chr - 32;
  }
  else
  {
    return;
  }
  
  uint16_t start = chr * FONT_SIZE_18 * 2;
  
  for (y_pos = 0; y_pos < (FONT_SIZE_18); y_pos++)
  {
    uint16_t line_num = start + y_pos*2;
    uint16_t hor_line = (uint16_t)lcd_font_size18[line_num] | ((uint16_t)lcd_font_size18[line_num + 1] << 8);
    //uint16_t hor_line = ((uint16_t*)display_font_size22)[start + y_pos];
    for (x_pos = 0; x_pos < (FONT_SIZE_18_WIDTH); x_pos++)
    {
      if (hor_line & (1 << (x_pos)))
        lcd_set_pixel(x_start + x_pos, y_start + y_pos);
      else
        lcd_reset_pixel(x_start + x_pos, y_start + y_pos);
    }
  }
}

//Draw black bar
void draw_caption_bar(uint8_t height)
{
  uint16_t x_pos, y_pos;
  for (x_pos = 0; x_pos < LCD_RIGHT_OFFSET; x_pos++)
  {
    for (y_pos = 0; y_pos < height; y_pos++)
    {
        lcd_set_pixel(x_pos, y_pos);
    }
  }
}


//Horizontal line
void display_draw_line(uint16_t y)
{
  uint16_t x_pos;
  for (x_pos = 0; x_pos <= LCD_RIGHT_OFFSET; x_pos++)
  {
    lcd_set_pixel(x_pos, y);
  }
}


uint16_t get_font_width(uint8_t font)
{
  switch (font)
  {
    case FONT_SIZE_6:  return FONT_SIZE_6_WIDTH;
    case FONT_SIZE_8:  return FONT_SIZE_8_WIDTH;
    case FONT_SIZE_11: return FONT_SIZE_11_WIDTH;
    case FONT_SIZE_18: return (FONT_SIZE_18_WIDTH + 2);
    default: return 5;
  }
}

void display_draw_vertical_line(uint16_t x, uint16_t y1, uint16_t y2)
{
  //y1 must be less than y2
  if (y1 > y2)
  {
    uint16_t tmp = y1;
    y1 = y2;
    y2 = tmp;
  }
  
  for (uint16_t y = y1; y <= y2; y++)
  {
    lcd_set_pixel(x, y);
  }
}

void display_draw_horizontal_line(uint16_t x1, uint16_t x2, uint16_t y)
{
  if (x1 > x2)
  {
    uint16_t tmp = x1;
    x1 = x2;
    x2 = tmp;
  }
  
  for (uint16_t x = x1; x <= x2; x++)
  {
    lcd_set_pixel(x, y);
  }
}

void display_clear_horizontal_line(uint16_t x1, uint16_t x2, uint16_t y)
{
  if (x1 > x2)
  {
    uint16_t tmp = x1;
    x1 = x2;
    x2 = tmp;
  }
  
  for (uint16_t x = x1; x <= x2; x++)
  {
    lcd_reset_pixel(x, y);
  }
}

void display_draw_rectangle(int x, int y, int width, int height) 
{
    //Top 
    display_draw_horizontal_line(x, x + width - 1, y);

    // Bottom
    display_draw_horizontal_line(x, x + width, y + height - 1);

    // Left 
    display_draw_vertical_line(x, y, y + height - 1);

    // Right
    display_draw_vertical_line(x + width, y, y + height - 1);
}

void display_draw_emplty_rectangle(int x, int y, int width, int height) 
{
    for (uint32_t cur_y = y; cur_y < (y+height); cur_y++)
    {
        display_clear_horizontal_line(x, x + width, cur_y);
    }
    
    display_draw_rectangle(x, y, width, height);
}

void utf8_to_cp1251(uint8_t *in_str, uint8_t *out_str, uint8_t max_out_len, uint16_t len)
{
  uint16_t i = 0;
  uint16_t j = 0;
  uint8_t char1 = 0;
  uint8_t char2 = 0;
          
	while(in_str[i] && (j <= max_out_len) && (in_str[i] > 0))
	{
		if ((i + 1) < len)
		{
			char1 = in_str[i] & 0xFF;
			char2 = in_str[i+1] & 0xFF;
			if ((char1 == 0xD0) && (char2 == 0x81))
			{
				out_str[j] = 168;
				i++;
			}
			else if ((char1 == 0xD1) && (char2 == 0x91))
			{
				out_str[j] = 184;
				i++;
			}
			else if ((char1 == 0xD0) && (char2 >= 0x90) && (char2 <= 0xBF))
			{
				out_str[j] = char2 + 48;
				i++;
			}
			else if ((char1 == 0xD1) && (char2 >= 0x80) && (char2 <= 0x8F))
			{
				out_str[j] = char2 + 112;
				i++;
			}
			else 
      {
        out_str[j] = in_str[i];
      }
		}
		else
    {
      out_str[j] = in_str[i];
    }
    i++;
    j++;
	}
}
