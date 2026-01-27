#include "rt_update_flash.h"

#if defined(__GNUC__)
#define RT_UPDATE_WEAK __attribute__((weak))
#else
#define RT_UPDATE_WEAK
#endif

RT_UPDATE_WEAK bool RT_UpdateFlashEnter(void)
{
    return false;
}

RT_UPDATE_WEAK bool RT_UpdateFlashEraseSector(uint32_t address)
{
    (void)address;
    return false;
}

RT_UPDATE_WEAK bool RT_UpdateFlashProgramPage(uint32_t address, const uint8_t *data, size_t length)
{
    (void)address;
    (void)data;
    (void)length;
    return false;
}

RT_UPDATE_WEAK bool RT_UpdateFlashRead(uint32_t address, uint8_t *data, size_t length)
{
    (void)address;
    (void)data;
    (void)length;
    return false;
}

RT_UPDATE_WEAK bool RT_UpdateFlashFinalize(void)
{
    return false;
}
