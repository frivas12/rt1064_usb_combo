#pragma once

#include "tinyxml2.h"

#include <usr_limits.h>

namespace builders
{
    namespace limits
    {
        Limits_save build_struct_limits(tinyxml2::XMLElement * p_data_node);
    }
}
