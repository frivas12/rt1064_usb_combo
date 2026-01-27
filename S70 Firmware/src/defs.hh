// def.hh

/**************************************************************************//**
 * \file defs.hh
 * \author Sean Benish
 * \brief Extends defs.h with c++ related operations.
 *****************************************************************************/

#ifndef SRC_DEFS_HH_
#define SRC_DEFS_HH_

#include "defs.h"

// Allows for external programs to remove the cpp memory lib.
#ifndef _DESKTOP_BUILD
#include "cppmem.hh"
#endif

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

template<typename T>
T integral_increment(T& t)
{
    t = static_cast<T>(static_cast<uint32_t>(t) + 1);
    return t;
}

slot_types operator++ (slot_types& base);
slot_types operator-- (slot_types& base);
slot_types operator+= (slot_types& base, const slot_types offset);
slot_types operator-= (slot_types& base, const slot_types offset);

bool operator== (config_signature_t&, config_signature_t&);

constexpr bool operator== (const device_signature_t& l, const device_signature_t& r) noexcept {
    return l.slot_type == r.slot_type
        && l.device_id == r.device_id;
}

#endif /* SRC_DEFS_H_ */
//EOF
