// save_util.hh

/**************************************************************************//**
 * \file save_util.hh
 * \author Sean Benish
 * \brief Common utility functions used by slot cards.
 *****************************************************************************/

#ifndef SRC_SYSTEM_DRIVERS_SAVE_CONSTRUCTOR_SAVE_UTIL_HH_
#define SRC_SYSTEM_DRIVERS_SAVE_CONSTRUCTOR_SAVE_UTIL_HH_

#include "slot_nums.h"
#include "config_save.tcc"
#include "gate.h"
#include "lock_guard.hh"

namespace save_util
{
/*****************************************************************************
 * Defines
 *****************************************************************************/



/*****************************************************************************
 * Data Types
 *****************************************************************************/



/*****************************************************************************
 * Constants
 *****************************************************************************/



/*****************************************************************************
 * Public Functions
 *****************************************************************************/

    /**
     * Update the config save's device signature if a device is currently connected,
     * then it saves it to stroage.
     * \tparam T Saved parameters struture.
     * \param[in]       card_slot
     * \param[in]       base_address
     * \param[out]      r_config
     */
    template<typename T>
    void update_save(const slot_nums card_slot, const uint32_t base_address, ConfigSave<T>& r_config)
    {
        if (xGatePass(slots[card_slot].device.connection_gate, 0) == pdPASS)
        {
            lock_guard lg(slots[card_slot].xSlot_Mutex);
            r_config.signature = slots[card_slot].device.info.signature;
        }

        r_config.write_eeprom(base_address);
    }
}

#endif /* SRC_SYSTEM_DRIVERS_SAVE_CONSTRUCTOR_SAVE_UTIL_HH_ */
//EOF
