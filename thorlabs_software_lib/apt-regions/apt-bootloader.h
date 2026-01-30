#ifndef APT_BOOTLOADER_H
#define APT_BOOTLOADER_H

/****************************************************************************
 * Non-APT Region - Bootloader (0x0000 - 0x00FF)                            *
 ****************************************************************************/
#define MGMSG_BL_REQ_FIRMWAREVER                    0x002F
#define MGMSG_BL_GET_FIRMWAREVER			        0x0030

#define MGMSG_SET_SER_TO_EEPROM						0x00A0
#define MGMSG_GET_SER_STATUS						0x00A1
#define MGMSG_BL_SET_FIRMWARE						0x00A3
#define MGMSG_BL_REQ_FIRMWARE						0x00A4
#define MGMSG_BL_GET_FIRMWARE			            0x00A5
#define MGMSG_GET_UPDATE_FIRMWARE                   0x00A6
#define MGMSG_RESET_FIRMWARE_LOADCOUNT				0x00A7
#define MGMSG_BL_SET_FLASHPAGE 						0x00A8
#define MGMSG_BL_GET_FLASHPAGE 						0x00A9

#endif
