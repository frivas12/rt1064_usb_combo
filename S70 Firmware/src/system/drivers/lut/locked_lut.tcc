// locked_lut.tcc

/**************************************************************************//**
 * \file locked_lut.tcc
 * \author Sean Benish
 * \brief Wrapper for a LUT that uses a FreeRTOS lock.
 *****************************************************************************/

#ifndef _SRC_SYSTEM_DRIVERS_LUT_LOCKED_LUT_TCC
#define _SRC_SYSTEM_DRIVERS_LUT_LOCKED_LUT_TCC

#include "lut.tcc"

/*****************************************************************************
 * Constants
 *****************************************************************************/

/*****************************************************************************
 * Macros
 *****************************************************************************/

/*****************************************************************************
 * Data Types
 *****************************************************************************/

template<typename STRUCTURE_KEY, typename DISCRIMINATOR_KEY>
class LockedLUT
{
private:
    SemaphoreHandle_t _lock;

    LUT<STRUCTURE_KEY, DISCRIMINATOR_KEY> _lut;    

public:
    LockedLUT(const LUT_ID id, uint32_t (*payload_size_func) (const STRUCTURE_KEY));

#ifdef LUT_DESTRUCTORS
    ~LockedLUT();
#endif

    inline LUT_ERROR load_data (const efs::handle& r_handle, const STRUCTURE_KEY str_key, const DISCRIMINATOR_KEY disc_key,
        char * const p_payload)
    {
        LUT_ERROR rt;

        xSemaphoreTake(_lock, portMAX_DELAY);
        rt = _lut.load_data(r_handle, str_key, disc_key, p_payload);
        xSemaphoreGive(_lock);

        return rt;
    }

    inline LUT<STRUCTURE_KEY, DISCRIMINATOR_KEY> * _get_lut()
    {
        return &_lut;
    }

};

/*****************************************************************************
 * Private Function Prototypes
 *****************************************************************************/

/*****************************************************************************
 * Static Data
 *****************************************************************************/

/******************************************************************************
 * Interrupt Handlers
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/

template<typename STRUCTURE_KEY, typename DISCRIMINATOR_KEY>
LockedLUT<STRUCTURE_KEY, DISCRIMINATOR_KEY>::LockedLUT(const LUT_ID id, uint32_t (*payload_size_func) (const STRUCTURE_KEY))
    : _lut(LUT<STRUCTURE_KEY, DISCRIMINATOR_KEY>(id, payload_size_func))
{
    _lock = xSemaphoreCreateMutex();
    if (_lock == nullptr)
    {
        /// \todo Crash maybe?
    }
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/

#endif

// EOF
