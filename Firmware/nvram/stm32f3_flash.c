/* Includes ------------------------------------------------------------------*/
#include "stm32f3_flash.h"
#include "main.h"
#include "string.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define FLASH_FLAG_ALL     FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_FLAG_PGERR
#define FLASH_SECTOR_ADDRESS_MSK        (FLASH_SECTOR_SIZE - 1)

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

void flash_erase_sector(uint8_t sector_idx)
{
  uint32_t int_state;
  
  // 1. Разблокировка FLASH (FLASH_Unlock)
  if (FLASH->CR & FLASH_CR_LOCK)
  {
    FLASH->KEYR = 0x45670123U; // FLASH_KEY1
    FLASH->KEYR = 0xCDEF89ABU; // FLASH_KEY2
  }

  ENTER_CRITICAL(int_state);

  // 2. Сброс флагов ошибок и окончания операции (FLASH_ClearFlag)
  FLASH->SR = FLASH_SR_EOP | FLASH_SR_WRPERR | FLASH_SR_PGERR;
  
  // Вычисление адреса страницы
  uint32_t address_dst = FLASH_BASE + FLASH_SECTOR_SIZE * sector_idx;

  // 3. Очистка страницы (FLASH_ErasePage)
  // Активируем режим стирания страницы (Page Erase)
  FLASH->CR |= FLASH_CR_PER;
  // Записываем адрес страницы в регистр AR
  FLASH->AR = address_dst;
  // Запуск операции стирания
  FLASH->CR |= FLASH_CR_STRT;

  // Ожидание завершения операции (Busy флаг)
  while (FLASH->SR & FLASH_SR_BSY)
  {
    // Здесь можно добавить таймаут, если необходима защита от зависания
  }

  // Деактивируем режим стирания страницы
  FLASH->CR &= ~FLASH_CR_PER;

  LEAVE_CRITICAL(int_state);

  // 4. Блокировка FLASH (FLASH_Lock)
  FLASH->CR |= FLASH_CR_LOCK;
}

// Записать побайтово массив, начиная с указанного адреса
// address_dst - адрес начала записываемых данных
// buf - указатель на записываемый массив
// size - размер буфера с данными
// Данные можно писать в один и тот же сектор несколько раз
void flash_write(uint32_t address_dst, uint8_t *buf, uint16_t size)
{
  uint32_t int_state;
  
  // 1. Разблокировка FLASH
  if (FLASH->CR & FLASH_CR_LOCK)
  {
    FLASH->KEYR = 0x45670123U; // FLASH_KEY1
    FLASH->KEYR = 0xCDEF89ABU; // FLASH_KEY2
  }

  // 2. Сброс флагов ошибок и окончания предыдущих операций
  FLASH->SR = FLASH_SR_EOP | FLASH_SR_WRPERR | FLASH_SR_PGERR;
  
  // Приведение буфера к 16-битному типу для итерации
  uint16_t *data_ptr = (uint16_t *)buf;
  uint16_t halfwords_count = size / sizeof(uint16_t);

  // Запись по 2 байта
  for (uint16_t i = 0; i < halfwords_count; i++)
  {
    ENTER_CRITICAL(int_state);
    
    // Включаем режим программирования (записи)
    FLASH->CR |= FLASH_CR_PG;
    
    // Выполняем запись полуслова по целевому адресу
    *(volatile uint16_t*)address_dst = data_ptr[i];
    
    // Ожидание завершения операции (Busy флаг)
    while (FLASH->SR & FLASH_SR_BSY)
    {
      // Здесь можно добавить таймаут
    }
    
    // Проверяем наличие ошибок после записи полуслова
    uint32_t status = FLASH->SR;
    
    // Отключаем режим программирования
    FLASH->CR &= ~FLASH_CR_PG;
    
    LEAVE_CRITICAL(int_state);
    
    // Если обнаружена ошибка записи или защиты от записи — прерываем цикл
    if (status & (FLASH_SR_WRPERR | FLASH_SR_PGERR))
    {
      break;
    }
    
    // Переходим к следующему адресу (2 байта)
    address_dst += sizeof(uint16_t);
  }

  // 3. Блокировка FLASH
  FLASH->CR |= FLASH_CR_LOCK;
}


// src_addr - адрес начала чтения
// buf - указатель на массив, куда будут считаны данные
// size - размер считываемых данных
void flash_read(uint32_t src_addr, uint8_t *buf, uint16_t size)
{
    memcpy((void *)buf, (void *)src_addr, size);
}
