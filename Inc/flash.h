#ifndef FLASH_H_
#define FLASH_H_

#include "stm_lib.h"
#include <string.h>

void Flash_Read(uint32_t addr, void * const data, uint16_t len);
void Flash_Write(uint32_t addr, void * const data, uint16_t len);
void Flash_Erase(uint32_t pageAddress);
void Flash_Unlock(void);
void Flash_Lock(void);

#endif /* FLASH_H_ */
