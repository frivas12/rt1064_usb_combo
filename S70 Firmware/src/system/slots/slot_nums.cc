#ifdef __cplusplus

#include "slot_nums.h"
#include <cstdint>

// Allows the behavior to be linked to one source without having the chance of a call with the prefix operator.
static inline void increment(slot_nums& num)
{
    num = static_cast<slot_nums>(static_cast<uint16_t>(num) + 1);
}

slot_nums operator++(slot_nums& num)
{
    increment(num);
    return num;
}
slot_nums operator++(slot_nums& num, int)
{
    const slot_nums tmp = num;
    increment(num);
    return tmp;
}

#endif