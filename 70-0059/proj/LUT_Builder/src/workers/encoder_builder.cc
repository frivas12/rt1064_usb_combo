#include "lut-builder/workers/encoder_builder.hh"
#include <stdint.h>
#include <string>

Encoder_Save builders::encoder::build_struct_encoder(tinyxml2::XMLElement * p_data_node)
{
    Encoder_Save rt;

    tinyxml2::XMLElement * itr = p_data_node->FirstChildElement();

    while (itr != nullptr)
    {
        std::string name = std::string(itr->Name());

        if (name == "EncoderType") {
            rt.encoder_type = itr->FindAttribute("value")->UnsignedValue();
        } else if (name == "IndexDelta") {
            rt.index_delta_min = itr->FindAttribute("min")->UnsignedValue();
            rt.index_delta_step = itr->FindAttribute("step")->UnsignedValue();
        } else if (name == "NmPerCount") {
            rt.nm_per_count = itr->FindAttribute("value")->FloatValue();
        }

        itr = itr->NextSiblingElement();
    }

    return rt;
}
