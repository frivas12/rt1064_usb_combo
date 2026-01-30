#ifndef APT_MOTION_CONTROLLERS_H
#define APT_MOTION_CONTROLLERS_H
/****************************************************************************
 * APT Region - Motion Controllers (0x4000 - 0x41FF)                        *
 ****************************************************************************/
/****************************************************************************
 * System (0x4000 - 0x403C)                                                 *
 ****************************************************************************/
#define MGMSG_MCM_HW_REQ_INFO						0x4000
#define MGMSG_MCM_HW_GET_INFO						0x4001
#define MGMSG_CPLD_UPDATE							0x4002
#define MGMSG_SET_HW_REV							0x4003
#define MGMSG_SET_CARD_TYPE							0x4004
#define MGMSG_SET_DEVICE							0x4005
#define MGMSG_REQ_DEVICE							0x4006
#define MGMSG_GET_DEVICE							0x4007
#define MGMSG_SET_DEVICE_BOARD						0x4008
#define MGMSG_REQ_DEVICE_BOARD						0x4009
#define MGMSG_GET_DEVICE_BOARD						0x400A
#define MGMSG_RESTART_PROCESSOR						0x400B
#define MGMSG_ERASE_EEPROM							0x400C
#define MGMSG_REQ_CPLD_WR							0x400D
#define MGMSG_GET_CPLD_WR							0x400E
#define MGMSG_TASK_CONTROL							0x400F
#define MGMSG_BOARD_REQ_STATUSUPDATE				0x4010
#define MGMSG_BOARD_GET_STATUSUPDATE				0x4011
#define MGMSG_MOD_REQ_JOYSTICK_INFO					0x4012
#define MGMSG_MOD_GET_JOYSTICK_INFO					0x4013
#define MGMSG_MOD_SET_JOYSTICK_MAP_IN				0x4014
#define MGMSG_MOD_REQ_JOYSTICK_MAP_IN				0x4015
#define MGMSG_MOD_GET_JOYSTICK_MAP_IN				0x4016
#define MGMSG_MOD_SET_JOYSTICK_MAP_OUT				0x4017
#define MGMSG_MOD_REQ_JOYSTICK_MAP_OUT				0x4018
#define MGMSG_MOD_GET_JOYSTICK_MAP_OUT				0x4019
#define MGMSG_MOD_SET_SYSTEM_DIM					0x401A
#define MGMSG_MOD_REQ_SYSTEM_DIM					0x401B
#define MGMSG_MOD_GET_SYSTEM_DIM					0x401C
#define MGMSG_SET_STORE_POSITION_DEADBAND			0x401D
#define MGMSG_REQ_STORE_POSITION_DEADBAND			0x401E
#define MGMSG_GET_STORE_POSITION_DEADBAND			0x401F
#define MGMSG_MCM_ERASE_DEVICE_CONFIGURATION		0x4020
#define MGMSG_SET_STORE_POSITION					0x4021
#define MGMSG_REQ_STORE_POSITION					0x4022
#define MGMSG_GET_STORE_POSITION					0x4023
#define MGMSG_SET_GOTO_STORE_POSITION				0x4024
#define MGMSG_MCM_START_LOG							0x4025
#define MGMSG_MCM_POST_LOG							0x4026 // asynchronous send to only USB PC port
#define MGMSG_MCM_SET_ENABLE_LOG					0x4027
#define MGMSG_MCM_REQ_ENABLE_LOG					0x4028
#define MGMSG_MCM_GET_ENABLE_LOG					0x4029
#define MGMSG_MCM_REQ_JOYSTICK_DATA					0x402A
#define MGMSG_MCM_GET_JOYSTICK_DATA					0x402B
#define MGMSG_MCM_SET_SLOT_TITLE					0x402C
#define MGMSG_MCM_REQ_SLOT_TITLE					0x402D
#define MGMSG_MCM_GET_SLOT_TITLE					0x402E
#define MGMSG_MOD_REQ_JOYSTICK_CONTROL				0x402F
#define MGMSG_MOD_GET_JOYSTICK_CONTROL				0x4030
// reserve space for future

/****************************************************************************
 * Stepper (0x403D - 0x405F)                                                *
 ****************************************************************************/
#define MGMSG_MCM_SET_SOFT_LIMITS					0x403D
#define MGMSG_MCM_SET_HOMEPARAMS					0x403E
#define MGMSG_MCM_REQ_HOMEPARAMS					0x403F
#define MGMSG_MCM_GET_HOMEPARAMS					0x4040
#define MGMSG_MCM_SET_STAGEPARAMS					0x4041
#define MGMSG_MCM_REQ_STAGEPARAMS					0x4042
#define MGMSG_MCM_GET_STAGEPARAMS					0x4043
#define MGMSG_MCM_REQ_STATUSUPDATE					0x4044
#define MGMSG_MCM_GET_STATUSUPDATE					0x4045
#define MGMSG_MCM_SET_ABS_LIMITS					0x4046
#define MGMSG_MCM_MOT_SET_LIMSWITCHPARAMS			0x4047
#define MGMSG_MCM_MOT_REQ_LIMSWITCHPARAMS			0x4048
#define MGMSG_MCM_MOT_GET_LIMSWITCHPARAMS			0x4049
#define MGMSG_MCM_MOT_MOVE_BY						0x4050	// Added for Texas TIDE autofocus
#define MGMSG_MCM_REQ_STEPPER_LOG					0x4051
#define MGMSG_MCM_GET_STEPPER_LOG					0x4052
#define MGMSG_MCM_MOVE_VELOCITY					    0x4053
#define MGMSG_MCM_MOT_SET_VELOCITY					MGMSG_MCM_MOVE_VELOCITY // This has been deprecated.
#define MGMSG_MCM_SET_SPEED_LIMIT                   0x4054
#define MGMSG_MCM_REQ_SPEED_LIMIT                   0x4055
#define MGMSG_MCM_GET_SPEED_LIMIT                   0x4056




/****************************************************************************
 * Shutter (0x4060 - 0x407F)                                                *
 ****************************************************************************/
#define MGMSG_MCM_SET_SHUTTERPARAMS					0x4064
#define MGMSG_MCM_REQ_SHUTTERPARAMS					0x4065
#define MGMSG_MCM_GET_SHUTTERPARAMS					0x4066
#define MGMSG_MCM_REQ_INTERLOCK_STATE				0x4067
#define MGMSG_MCM_GET_INTERLOCK_STATE				0x4068
#define MGMSG_MCM_SET_PWM_PERIOD                    0x4069
#define MGMSG_MCM_REQ_PWM_PERIOD                    0x406A
#define MGMSG_MCM_GET_PWM_PERIOD                    0x406B
#define MGMSG_MCM_SET_SHUTTERTRIG                   0x406C
#define MGMSG_MCM_REQ_SHUTTERTRIG                   0x406D
#define MGMSG_MCM_GET_SHUTTERTRIG                   0x406E

/****************************************************************************
 * Slider IO (0x4080 - 0x409F)                                              *
 ****************************************************************************/
#define MGMSG_MCM_SET_MIRROR_STATE					0x4087
#define MGMSG_MCM_REQ_MIRROR_STATE					0x4088
#define MGMSG_MCM_GET_MIRROR_STATE					0x4089
#define MGMSG_MCM_SET_MIRROR_PARAMS					0x408A
#define MGMSG_MCM_REQ_MIRROR_PARAMS					0x408B
#define MGMSG_MCM_GET_MIRROR_PARAMS					0x408C
#define MGMSG_MCM_SET_MIRROR_PWM_DUTY				0x408D
#define MGMSG_MCM_REQ_MIRROR_PWM_DUTY				0x408E
#define MGMSG_MCM_GET_MIRROR_PWM_DUTY				0x408F

/****************************************************************************
 * Synchronized Motion (0x40A0 - 0x40BF)                                    *
 ****************************************************************************/
#define MGMSG_MCM_SET_HEX_POSE						0x40A0
#define MGMSG_MCM_REQ_HEX_POSE						0x40A1
#define MGMSG_MCM_GET_HEX_POSE						0x40A2
#define MGMSG_MCM_SET_SYNC_MOTION_PARAM				0x40A3
#define MGMSG_MCM_REQ_SYNC_MOTION_PARAM				0x40A4
#define MGMSG_MCM_GET_SYNC_MOTION_PARAM				0x40A5
#define MGMSG_MCM_SET_SYNC_MOTION_POINT				0x40A6
#define MGMSG_MCM_SET_HEX_STRUT_ENDS				0x40A7
#define MGMSG_MCM_REQ_HEX_STRUT_ENDS				0x40A8
#define MGMSG_MCM_GET_HEX_STRUT_ENDS				0x40A9
#define MGMSG_MCM_SET_HEX_POSE_LIMITS 				0x40AC
#define MGMSG_MCM_REQ_HEX_POSE_LIMITS				0x40AD
#define MGMSG_MCM_GET_HEX_POSE_LIMITS				0x40AE


#define MGMSG_LA_GET_JOYSTICKS_MAP					#error("depreciated") //0x085B
#define MGMSG_LA_GET_JOYSTICKS_INFO					#error("depreciated") //0x0858
      
/****************************************************************************
 * Piezo (0x40D0 - 0x40EB)                                                  *
 ****************************************************************************/
#define MGMSG_MCM_PIEZO_SET_PRAMS					0x40DC
#define MGMSG_MCM_PIEZO_REQ_PRAMS					0x40DD
#define MGMSG_MCM_PIEZO_GET_PRAMS					0x40DE
#define MGMSG_MCM_PIEZO_REQ_VALUES					0x40DF
#define MGMSG_MCM_PIEZO_GET_VALUES					0x40E0
#define MGMSG_MCM_PIEZO_REQ_LOG						0x40E1
#define MGMSG_MCM_PIEZO_GET_LOG						0x40E2
#define MGMSG_MCM_PIEZO_SET_ENABLE_PLOT				0x40E3
#define MGMSG_MCM_PIEZO_SET_DAC_VOLTS				0x40E4
#define MGMSG_MCM_PIEZO_SET_PID_PARMS				0x40E5
#define MGMSG_MCM_PIEZO_REQ_PID_PARMS				0x40E6
#define MGMSG_MCM_PIEZO_GET_PID_PARMS				0x40E7
#define MGMSG_MCM_PIEZO_SET_MOVE_BY					0x40E8
#define MGMSG_MCM_PIEZO_SET_MODE					0x40E9
#define MGMSG_MCM_PIEZO_REQ_MODE					0x40EA
#define MGMSG_MCM_PIEZO_GET_MODE					0x40EB

/****************************************************************************
 * One-Wire (0x40EC - 0x40F1)                                               *
 ****************************************************************************/
#define MGMSG_OW_SET_PROGRAMMING					0x40EC
#define MGMSG_OW_REQ_PROGRAMMING					0x40ED
#define MGMSG_OW_GET_PROGRAMMING					0x40EE
#define MGMSG_OW_PROGRAM							0x40EF
#define MGMSG_OW_REQ_PROGRAMMING_SIZE				0x40F0
#define MGMSG_OW_GET_PROGRAMMING_SIZE				0x40F1


/****************************************************************************
 * Device Detection (0x40F2 - 0x40F7)                                       *
 ****************************************************************************/
#define MGMSG_MCM_SET_ALLOWED_DEVICES				0x40F2
#define MGMSG_MCM_REQ_ALLOWED_DEVICES				0x40F3
#define MGMSG_MCM_GET_ALLOWED_DEVICES				0x40F4
#define MGMSG_MCM_SET_DEVICE_DETECTION				0x40F5
#define MGMSG_MCM_REQ_DEVICE_DETECTION				0x40F6
#define MGMSG_MCM_GET_DEVICE_DETECTION				0x40F7

/****************************************************************************
 * Embedded File System (0x40F8 - 0x40FF)                                   *
 ****************************************************************************/
#define MGMSG_MCM_EFS_REQ_HWINFO					0x40F8
#define MGMSG_MCM_EFS_GET_HWINFO					0x40F9
#define MGMSG_MCM_EFS_SET_FILEINFO					0x40FA
#define MGMSG_MCM_EFS_REQ_FILEINFO					0x40FB
#define MGMSG_MCM_EFS_GET_FILEINFO					0x40FC
#define MGMSG_MCM_EFS_SET_FILEDATA					0x40FD
#define MGMSG_MCM_EFS_REQ_FILEDATA					0x40FE
#define MGMSG_MCM_EFS_GET_FILEDATA					0x40FF

/****************************************************************************
 * Look-Up Table (0x4100 - 0x4107)                                          *
 ****************************************************************************/
#define MGMSG_MCM_LUT_SET_LOCK						0x4100
#define MGMSG_MCM_LUT_REQ_LOCK						0x4101
#define MGMSG_MCM_LUT_GET_LOCK						0x4102

/****************************************************************************
 * Error Reporting (0x4108 - 0x410F)                                        *
 ****************************************************************************/
#define MGMSG_MCM_REQ_PNPSTATUS						0x4108
#define MGMSG_MCM_GET_PNPSTATUS						0x4109

/****************************************************************************
 * Debugging - Difference for every device                                  *
 ****************************************************************************/
#define MGMSG_DEBUG_REQ_DATA                        0x41FD
#define MGMSG_DEBUG_GET_DATA                        0x41FE
#define MGMSG_DEBUG_SET_DATA                        0x41FF

#endif
