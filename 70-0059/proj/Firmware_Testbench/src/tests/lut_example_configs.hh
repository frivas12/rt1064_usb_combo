#pragma once

#include <stepper_saves.h>
#include <defs.hh>

constexpr int NUM_STEPPER_STEPPER_DEVICES = 13;
constexpr int NUM_HC_STEPPER_STEPPER_DEVICES = 13;
constexpr int NUM_HC_STEPPER_HC_STEPPER_DEVICES = 13;

namespace example
{
    extern Stepper_Config stepper_config;
    extern Stepper_drive_params stepper_drive_params;
    extern Flags_save flags_save;
    extern Limits_save limits_save;
    extern Home_Params home_params;
    extern Jog_Params jog_params;
    extern Encoder_Save enc_save;
    extern Stepper_Pid_Save stepper_pid_save;
    extern Stepper_Store stepper_store;
}