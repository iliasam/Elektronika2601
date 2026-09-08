/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __NVRAM_H
#define __NVRAM_H

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "config.h"

/* Exported types ------------------------------------------------------------*/
typedef struct
{
    uint16_t flash_ok_flag;
    
    uint32_t key_memory_freq_hz[MEM_BUTTONS_CNT];
  
} nvram_data_t;

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void nvram_use_dafault_settings(void);
void nvram_save_current_settings(void);
void nvram_read_data(void);
void nvram_save_key_memory_freq(uint32_t freq_hz, uint8_t index);
uint32_t nvram_read_key_memory_freq(uint8_t index);

#endif /* __NVRAM_H */
