#include "lut_types.hh"

LUT_ID& operator++ (LUT_ID & id)
{
    id = static_cast<LUT_ID>(static_cast<unsigned char>(id) + 1);
    return id;
}

LUT_ID operator++ (LUT_ID & id, int)
{
    LUT_ID tmp = id;
    id = static_cast<LUT_ID>(static_cast<unsigned char>(id) + 1);
    return tmp;
}