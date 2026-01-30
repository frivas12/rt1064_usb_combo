#include "lut-builder/workers/stepper_builder.hh"
#include "lut-builder/workers/common.hh"
#include "structure_id.hh"
#include <stdint.h>
#include <string>

static void parse_drive_kval(tinyxml2::XMLElement * const p_elem, Stepper_drive_params& rt);
static void parse_drive_slope(tinyxml2::XMLElement * const p_elem, Stepper_drive_params& rt);


Stepper_Save_Configs builders::stepper::build_config_list (tinyxml2::XMLElement * p_data_node)
{
    Stepper_Save_Configs cfgs;

    tinyxml2::XMLElement * itr = p_data_node->FirstChildElement();

    while (itr != nullptr)
    {
        const std::string name = itr->Name();

        if (name == "Signature")
        {
            const config_signature_t signature = builders::parse_config_signature(itr);


            switch(static_cast<Struct_ID>(signature.struct_id))
            {
            case Struct_ID::ENCODER:
                cfgs.enc_save = signature;
            break;
            case Struct_ID::LIMIT:
                cfgs.limits_save = signature;
            break;
            case Struct_ID::STEPPER_CONFIG:
                cfgs.stepper_config = signature;
            break;
            case Struct_ID::STEPPER_DRIVE:
                cfgs.stepper_drive_params = signature;
            break;
            case Struct_ID::STEPPER_FLAGS:
                cfgs.flags_save = signature;
            break;
            case Struct_ID::STEPPER_HOME:
                cfgs.home_params = signature;
            break;
            case Struct_ID::STEPPER_JOG:
                cfgs.jog_params = signature;
            break;
            case Struct_ID::STEPPER_PID:
                cfgs.pid_save = signature;
            break;
            case Struct_ID::STEPPER_STORE:
                cfgs.stepper_store = signature;
            break;
            default:
            break;
            }
        }

        itr = itr->NextSiblingElement();
    }

    return cfgs;
}


Stepper_Config builders::stepper::build_struct_config (tinyxml2::XMLElement * p_data_node)
{
    Stepper_Config rt;

    tinyxml2::XMLElement * itr = p_data_node->FirstChildElement();
    while (itr != nullptr)
    {
        std::string name = std::string(itr->Name());

        if (name == "AxisSerial") {
            rt.axis_serial_no = itr->FindAttribute("value")->UnsignedValue();
        } else if (name == "CountsPerUnit") {
            rt.counts_per_unit = itr->FindAttribute("value")->FloatValue();
        } else if (name == "Position") {
            rt.min_pos = itr->FindAttribute("min")->UnsignedValue();
            rt.max_pos = itr->FindAttribute("max")->UnsignedValue();
        } else if (name == "CollisionThreshold") {
            rt.collision_threshold = itr->FindAttribute("value")->UnsignedValue();
        }

        itr = itr->NextSiblingElement();
    }

    return rt;
}


Stepper_drive_params builders::stepper::build_struct_drive (tinyxml2::XMLElement * p_data_node)
{
    Stepper_drive_params rt;

    tinyxml2::XMLElement * itr = p_data_node->FirstChildElement();

    while (itr != nullptr)
    {
        std::string name = std::string(itr->Name());

        if (name == "KVal")
        {
            parse_drive_kval(itr, rt);
        } else if (name == "Slope") {
            parse_drive_slope(itr, rt);
        } else if (name == "Acceleration") {
            rt.acc = itr->FindAttribute("max")->UnsignedValue();
            rt.dec = itr->FindAttribute("min")->UnsignedValue();
        } else if (name == "Speed") {
            rt.max_speed = itr->FindAttribute("max")->UnsignedValue();
            rt.min_speed = itr->FindAttribute("min")->UnsignedValue();
        } else if (name == "FullStepSpeed") {
            rt.fs_spd = itr->FindAttribute("value")->UnsignedValue();
        } else if (name == "IntersectSpeed") {
            rt.int_speed = itr->FindAttribute("value")->UnsignedValue();
        } else if (name == "StallThreshold") {
            rt.stall_th = itr->FindAttribute("value")->UnsignedValue();
        } else if (name == "OCDThreshold") {
            rt.ocd_th = itr->FindAttribute("value")->UnsignedValue();
        } else if (name == "StepMode") {
            rt.step_mode = itr->FindAttribute("value")->UnsignedValue();
        } else if (name == "ICConfig") {
            rt.config = itr->FindAttribute("value")->UnsignedValue();
        } else if (name == "GateDriverConfig") {
            uint32_t val = itr->FindAttribute("value")->UnsignedValue();
            rt.gatecfg1 = (uint16_t)(val);
            rt.gatecfg2 = (uint16_t)(val >> 16);
        } else if (name == "ApproachVelocity") {
            rt.approach_vel = itr->FindAttribute("value")->UnsignedValue();
        } else if (name == "Deadband") {
            rt.deadband = itr->FindAttribute("value")->UnsignedValue();
        } else if (name == "Backlash") {
            rt.backlash = itr->FindAttribute("value")->UnsignedValue();
        } else if (name == "KickoutTime") {
            rt.kickout_time = itr->FindAttribute("value")->UnsignedValue();
        }

        itr = itr->NextSiblingElement();
    }

    return rt;
}


Flags_save builders::stepper::build_struct_flags (tinyxml2::XMLElement * p_data_node)
{
    Flags_save rt;

    tinyxml2::XMLElement * itr = p_data_node->FirstChildElement();

    while (itr != nullptr)
    {
        std::string name = std::string(itr->Name());

        if (name == "Flags") {
            rt.flags = itr->FindAttribute("value")->UnsignedValue();   
        }

        itr = itr->NextSiblingElement();
    }

    return rt;
}


Home_Params builders::stepper::build_struct_home (tinyxml2::XMLElement * p_data_node)
{
    Home_Params rt;

    tinyxml2::XMLElement * itr = p_data_node->FirstChildElement();

    while (itr != nullptr)
    {
        std::string name = std::string(itr->Name());

        if (name == "Mode") {
            rt.home_mode = itr->FindAttribute("value")->UnsignedValue();
        } else if (name == "Direction") {
            rt.home_dir = itr->FindAttribute("value")->UnsignedValue();
        } else if (name == "LimitSwitch") {
            rt.limit_switch = itr->FindAttribute("value")->UnsignedValue();
        } else if (name == "Velocity") {
            rt.home_velocity = itr->FindAttribute("value")->UnsignedValue();
        } else if (name == "OffsetDistance") {
            rt.offset_distance = itr->FindAttribute("value")->IntValue();
        }

        itr = itr->NextSiblingElement();
    }

    return rt;
}


Jog_Params builders::stepper::build_struct_jog (tinyxml2::XMLElement * p_data_node)
{
    Jog_Params rt;

    tinyxml2::XMLElement * itr = p_data_node->FirstChildElement();

    while (itr != nullptr)
    {
        const std::string name = itr->Name();

        if (name == "Mode") {
            rt.jog_mode = itr->FindAttribute("value")->UnsignedValue();
        } else if (name == "StepSize") {
            rt.step_size = itr->FindAttribute("value")->IntValue();
        } else if (name == "Velocity") {
            rt.min_vel = itr->FindAttribute("min")->IntValue();
            rt.max_vel = itr->FindAttribute("max")->IntValue();
        } else if (name == "Accel") {
            rt.acc = itr->FindAttribute("value")->IntValue();
        } else if (name == "StopMode") {
            rt.stop_mode = itr->FindAttribute("value")->UnsignedValue();
        }

        itr = itr->NextSiblingElement();
    }

    return rt;
}


Stepper_Pid_Save builders::stepper::build_struct_pid (tinyxml2::XMLElement * p_data_node)
{
    Stepper_Pid_Save rt;

    tinyxml2::XMLElement * itr = p_data_node->FirstChildElement();

    while (itr != nullptr)
    {
        std::string name = std::string(itr->Name());

        if (name == "Kp") {
            rt.Kp = itr->FindAttribute("value")->UnsignedValue();
        } else if (name == "Ki") {
            rt.Ki = itr->FindAttribute("value")->UnsignedValue();
        } else if (name == "Kd") {
            rt.Kd = itr->FindAttribute("value")->UnsignedValue();
        } else if (name == "IMax") {
            rt.imax = itr->FindAttribute("value")->UnsignedValue();
        } else if (name == "FilterControl") {
            rt.FilterControl = itr->FindAttribute("value")->UnsignedValue();
        }

        itr = itr->NextSiblingElement();
    }

    return rt;
}


Stepper_Store builders::stepper::build_struct_store (tinyxml2::XMLElement * p_data_node)
{
    Stepper_Store rt;

    tinyxml2::XMLElement * itr = p_data_node->FirstChildElement();

    while (itr != nullptr)
    {
        std::string name = std::string(itr->Name());

        if (name == "Position") {
            const int index = itr->FindAttribute("index")->IntValue();
            rt.stored_pos[index - 1] = itr->FindAttribute("value")->IntValue();
        } else if (name == "Deadband") {
            rt.stored_pos_deadband = itr->FindAttribute("value")->UnsignedValue();
        }

        itr = itr->NextSiblingElement();
    }

    return rt;
}




static void parse_drive_kval(tinyxml2::XMLElement * const p_elem, Stepper_drive_params& rt)
{
    tinyxml2::XMLElement * itr = p_elem->FirstChildElement();

    while (itr != nullptr)
    {
        std::string name = std::string(itr->Name());

        if (name == "Hold") {
            rt.kval_hold = itr->FindAttribute("value")->UnsignedValue();
        } else if (name == "Run") {
            rt.kval_run = itr->FindAttribute("value")->UnsignedValue();
        } else if (name == "Accel") {
            rt.kval_acc = itr->FindAttribute("value")->UnsignedValue();
        } else if (name == "Decel") {
            rt.kval_dec = itr->FindAttribute("value")->UnsignedValue();
        }

        itr = itr->NextSiblingElement();
    }
}


static void parse_drive_slope(tinyxml2::XMLElement * const p_elem, Stepper_drive_params& rt)
{
    tinyxml2::XMLElement * itr = p_elem->FirstChildElement();

    while (itr != nullptr)
    {
        std::string name = std::string(itr->Name());

        if (name == "Start") {
            rt.st_slp = itr->FindAttribute("value")->UnsignedValue();
        } else if (name == "FinalAccel") {
            rt.fn_slp_acc = itr->FindAttribute("value")->UnsignedValue();
        } else if (name == "FinalDecel") {
            rt.fn_slp_dec = itr->FindAttribute("value")->UnsignedValue();
        }

        itr = itr->NextSiblingElement();
    }
}

//EOF
