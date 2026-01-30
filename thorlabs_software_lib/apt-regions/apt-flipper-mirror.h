#ifndef APT_FLIPPER_MIRROR_H
#define APT_FLIPPER_MIRROR_H

#include "../apt-deprecation.h"

/****************************************************************************
 * APT Region - Flipper Mirror (0x4800 - 0x481F)                            *
 ****************************************************************************/
#define MGMSG_FMV2_SET_DATA_ENDIANNESS              APT_DEPRECATION_ERROR("deprecated")
#define MGMSG_FMV2_REQ_DATA_ENDIANNESS              APT_DEPRECATION_ERROR("deprecated")
#define MGMSG_FMV2_GET_DATA_ENDIANNESS              APT_DEPRECATION_ERROR("deprecated")
#define MGMSG_FMV2_SET_PROTOCOL_VERSION             APT_DEPRECATION_ERROR("deprecated")
#define MGMSG_FMV2_REQ_PROTOCOL_VERSION             APT_DEPRECATION_ERROR("deprecated")
#define MGMSG_FMV2_GET_PROTOCOL_VERSION             APT_DEPRECATION_ERROR("deprecated")
#define MGMSG_FMV2_SET_UNIQUE_IDENTIFER             APT_DEPRECATION_ERROR("deprecated")
#define MGMSG_FMV2_REQ_UNIQUE_IDENTIFER             APT_DEPRECATION_ERROR("deprecated")
#define MGMSG_FMV2_GET_UNIQUE_IDENTIFER             APT_DEPRECATION_ERROR("deprecated")
#define MGMSG_FMV2_SET_SUPPORTED_COMMANDS           APT_DEPRECATION_ERROR("MGMSG_FMV2_SET_SUPPORTED_COMMANDS is deprecated.  Use MGMSG_PREL_SET_SUPPORTED_COMMANDS instead")
#define MGMSG_FMV2_REQ_SUPPORTED_COMMANDS           APT_DEPRECATION_ERROR("MGMSG_FMV2_REQ_SUPPORTED_COMMANDS is deprecated.  Use MGMSG_PREL_REQ_SUPPORTED_COMMANDS instead")
#define MGMSG_FMV2_GET_SUPPORTED_COMMANDS           APT_DEPRECATION_ERROR("MGMSG_FMV2_GET_SUPPORTED_COMMANDS is deprecated.  Use MGMSG_PREL_GET_SUPPORTED_COMMANDS instead")
#define MGMSG_FMV2_SET_DATA                         0x480C
#define MGMSG_FMV2_REQ_DATA                         0x480D
#define MGMSG_FMV2_GET_DATA                         0x480E

#endif
