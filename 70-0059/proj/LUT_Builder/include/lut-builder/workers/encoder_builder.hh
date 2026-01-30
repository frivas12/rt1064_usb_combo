#pragma once

#include "tinyxml2.h"

#include "encoder.h"

#define ALLOW_STORE_PARSING
namespace builders
{
    namespace encoder
    {
        Encoder_Save build_struct_encoder (tinyxml2::XMLElement * p_data_node);
    }

}
