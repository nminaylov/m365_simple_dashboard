#include "flash.h"

void Flash_Read(uint32_t addr, void * const data, uint16_t len)
{
	memcpy(data,(void *)(addr),len);
}

void Flash_Write(uint32_t addr, void * const data, uint16_t len)
{
	while (FLASH->SR & FLASH_SR_BSY);
	if (FLASH->SR & FLASH_SR_EOP)
	{
		FLASH->SR = FLASH_SR_EOP;
	}

	FLASH->CR |= FLASH_CR_PG;

	for (uint16_t i = 0; i < len; i += 2)
	{
		*(uint16_t*)(addr + i) = ((*((uint8_t*)(data+i+1))) << 8) | ((*((uint8_t*)(data+i))));
		while (!(FLASH->SR & FLASH_SR_EOP));
		FLASH->SR = FLASH_SR_EOP;
	}

	FLASH->CR &= ~(FLASH_CR_PG);
}

void Flash_Erase(uint32_t pageAddress)
{
	while (FLASH->SR & FLASH_SR_BSY);
	if (FLASH->SR & FLASH_SR_EOP)
	{
		FLASH->SR = FLASH_SR_EOP;
	}

	FLASH->CR |= FLASH_CR_PER;
	FLASH->AR = pageAddress;
	FLASH->CR |= FLASH_CR_STRT;
	while (!(FLASH->SR & FLASH_SR_EOP));
	FLASH->SR = FLASH_SR_EOP;
	FLASH->CR &= ~FLASH_CR_PER;
}

void Flash_Unlock(void)
{
	if (FLASH->CR & FLASH_CR_LOCK)
	{
		FLASH->KEYR = FLASH_KEY1;
		FLASH->KEYR = FLASH_KEY2;
	}
}

void Flash_Lock(void)
{
	FLASH->CR |= FLASH_CR_LOCK;
}
