using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace SystemXMLParser
{
    abstract class APT
    {
        public const ushort MGMSG_MOT_SET_DCPIDPARAMS                   = 0x04A0;

        public const ushort MGMSG_SET_CARD_TYPE                         = 0x4004;
        public const ushort MGMSG_MOD_SET_JOYSTICK_MAP_IN               = 0x4014;
        public const ushort MGMSG_MOD_SET_JOYSTICK_MAP_OUT              = 0x4017;

        public const ushort MGMSG_SET_STORE_POSITION_DEADBAND           = 0x401D;
        public const ushort MGMSG_SET_STORE_POSITION                    = 0x4021;
        public const ushort MGMSG_MCM_SET_HOMEPARAMS                    = 0x403E;
        public const ushort MGMSG_MCM_SET_STAGEPARAMS                   = 0x4041;
        public const ushort MGMSG_MCM_MOT_SET_LIMSWITCHPARAMS           = 0x4047;

        public const ushort MGMSG_LUT_SET_INQUIRY                       = 0x40F8;
        public const ushort MGMSG_LUT_INQUIRE                           = 0x40FC;
        public const ushort MGMSG_LUT_SET_PROGRAMMING                   = 0x40FE;
        public const ushort MGMSG_LUT_PROGRAM                           = 0x4102;
        public const ushort MGMSG_LUT_FINISH_PROGRAMMING                = 0x4104;
        public const ushort MGMSG_LUT_REQ_PROGRAMMING_SIZE              = 0x4106;

        public const ushort MGMSG_MCM_SET_ALLOWED_DEVICES               = 0x40F2;

        public const ushort MGMSG_MCM_SET_DEVICE_DETECTION              = 0x40F5;

        // See src/system/drivers/save_constructor/structure_id.hh.
        public enum StructIDs
        {
            // Encoder Structures
            ENCODER = 0,

            // Limit Structures
            LIMIT = 1,

            // Stepper Structures
            STEPPER_CONFIG = 2,
            STEPPER_DRIVE = 3,
            STEPPER_FLAGS = 4,
            STEPPER_HOME = 5,
            STEPPER_JOG = 6,
            STEPPER_PID = 7,
            STEPPER_STORE = 8,

            // Slider IO Structures
            SLIDER_IO_PARAMS = 9,

            // Shutter 4 Structures
            SHUTTER_4_PARAMS = 10,

            // Servo Structures
            SERVO_PARAMS = 11,
            SERVO_SHUTTER_PARAMS = 12,
            SERVO_STORE = 13,

            // Piezo Structures,
            PIEZO_PARAMS = 14,

            INVALID = 0xFFFF
        }
    }

}
