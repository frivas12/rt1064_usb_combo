#include "lut-builder/workers/limits_builder.hh"
#include <stdint.h>
#include <string>

Limits_save builders::limits::build_struct_limits(tinyxml2::XMLElement * p_data_node)
{
    Limits_save rt;

    tinyxml2::XMLElement * itr = p_data_node->FirstChildElement();

    while (itr != nullptr)
    {
        std::string name = std::string(itr->Name());

        if (name == "HardLimit") {
            rt.cw_hard_limit = itr->FindAttribute("cw")->UnsignedValue();
            rt.ccw_hard_limit = itr->FindAttribute("ccw")->UnsignedValue();
        } else if (name == "SoftLimit") {
            rt.cw_soft_limit = itr->FindAttribute("cw")->IntValue();
            rt.ccw_soft_limit = itr->FindAttribute("ccw")->IntValue();
        } else if (name == "AbsLimit") {
            rt.abs_high_limit = itr->FindAttribute("high")->IntValue();
            rt.abs_low_limit = itr->FindAttribute("low")->IntValue();
        } else if (name == "Mode") {
            rt.limit_mode = itr->FindAttribute("value")->UnsignedValue();
        }

        itr = itr->NextSiblingElement();
    }

    return rt;
}
