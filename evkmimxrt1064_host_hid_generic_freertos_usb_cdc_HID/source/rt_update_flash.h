#ifndef RT_UPDATE_FLASH_H_
#define RT_UPDATE_FLASH_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool RT_UpdateFlashEnter(void);
bool RT_UpdateFlashEraseSector(uint32_t address);
bool RT_UpdateFlashProgramPage(uint32_t address, const uint8_t *data, size_t length);
bool RT_UpdateFlashRead(uint32_t address, uint8_t *data, size_t length);
bool RT_UpdateFlashFinalize(void);

#endif /* RT_UPDATE_FLASH_H_ */
