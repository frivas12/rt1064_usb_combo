#pragma once

#include "tinyxml2.h"

#include <stepper_saves.h>

#define ALLOW_STORE_PARSING
namespace builders
{
    namespace stepper
    {
        Stepper_Save_Configs build_config_list (tinyxml2::XMLElement * p_data_node);

        Stepper_Config build_struct_config (tinyxml2::XMLElement * p_data_node);
        Stepper_drive_params build_struct_drive (tinyxml2::XMLElement * p_data_node);
        Flags_save build_struct_flags (tinyxml2::XMLElement * p_data_node);
        Home_Params build_struct_home (tinyxml2::XMLElement * p_data_node);
        Jog_Params build_struct_jog (tinyxml2::XMLElement * p_data_node);
        Stepper_Pid_Save build_struct_pid (tinyxml2::XMLElement * p_data_node);
        Stepper_Store build_struct_store (tinyxml2::XMLElement * p_data_node);
    }
}
