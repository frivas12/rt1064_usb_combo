#include "25lc1024.h"
#include <stdio.h>
#include <stdlib.h>
// #include "slots.h"
#include <cstring>
#include "emulation_help.hh"
#include "lut_types.hh"
#include "user_spi.h"

#define LUT_SIZE                        (EFS_EEPROM_END - EFS_EEPROM_START)
#define ADDRESS_TO_EMULATION(addr)      (addr-EFS_EEPROM_START)
#define ERASE_VALUE                     0xFF

char* emulated_flash = nullptr;

uint8_t lock_state = 2;

SemaphoreHandle_t xSPI_Semaphore;

void eeprom_init(void) {
    if (!emulated_flash)
    {
        emulated_flash = static_cast<char*>(malloc(LUT_SIZE));
    }
    for (uint32_t addr = 0; addr < LUT_SIZE; ++addr) {
        emulated_flash[addr] = 0xFF;
    }
    if (xSPI_Semaphore == nullptr)
    {
        xSPI_Semaphore = xSemaphoreCreateMutex();
    }
}

void eeprom_25LC1024_clear_sector(uint32_t sector) {
    if (lock_state != 0) {
        printf("ERROR: Tried to clear sector when system was not unlocked.\n");
        exit(0);
    }
    for (uint32_t address = EEPROM_25LC1024_SECTOR_SIZE*sector; address < (sector + 1)*EEPROM_25LC1024_SECTOR_SIZE; ++address) {
        emulated_flash[ADDRESS_TO_EMULATION(address)] = ERASE_VALUE;
    }
}

void eeprom_25LC1024_set_write_protect(uint8_t protection) {
    lock_state = (protection >> 2) & 0x03;
}
void eeprom_25LC1024_read(uint32_t startAdr, uint32_t len, uint8_t* rx_data) {
    memcpy(rx_data, emulated_flash + ADDRESS_TO_EMULATION(startAdr), len);
}
uint8_t eeprom_25LC1024_write(uint32_t startAdr, uint32_t len, const uint8_t *data) {
    memcpy(emulated_flash + ADDRESS_TO_EMULATION(startAdr), data, len);
    return 1;
}

LUT_EEPROM_CRC_t CRC_split(const char* p_data, uint32_t length, uint8_t crc)
{
    static const uint8_t POLY = 0x31;

    for(uint32_t i = 0; i < length; ++i)
    {
        crc ^= p_data[i];
        for (uint8_t j = 0; j < 8; ++j)
        {
            if ((crc & 0x80) == 0)
            {
                crc <<= 1;
            } else {
                crc = (uint8_t)((crc << 1) ^ POLY);
            }
        }
    }

    return crc;
}

/**
 * Calculates the CRC8 for a given data stream.
 * \param p_data Input data stream.
 * \param length Length of the data stream (in bytes).
 * \return The data stream's CRC8.
 */
LUT_EEPROM_CRC_t CRC(const char* p_data, uint32_t length)
{
    //Invert bits and return
    return CRC_split(p_data, length, 0);
}

void cycler(char* p_data, uint32_t count)
{
    static const unsigned char FILL_IN[] = {0xDE, 0xAD, 0xBE, 0xEF};
    uint8_t index = 0;
    for (uint32_t counter = 0; counter < count; ++counter) {
        p_data[counter] = FILL_IN[index];
        if (++index == 4) { index = 0; }
    }
}
