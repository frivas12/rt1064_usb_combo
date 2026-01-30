#ifndef APT_DUAL_PLANE_H
#define APT_DUAL_PLANE_H
/****************************************************************************
 * APT Region - Dual Plane (0x4600 - 0x46FF)                                *
 ****************************************************************************/
#define MGMSG_CLK_SET_INPUT_THRESH					0x4601
#define MGMSG_CLK_REQ_INPUT_THRESH					0x4602
#define MGMSG_CLK_GET_INPUT_THRESH					0x4603

#define MGMSG_CLK_SET_INPUT_SELECT					0x4604
#define MGMSG_CLK_REQ_INPUT_SELECT					0x4605
#define MGMSG_CLK_GET_INPUT_SELECT					0x4606

#define MGMSG_CLK_REQ_INPUT_STATUS					0x4607
#define MGMSG_CLK_GET_INPUT_STATUS					0x4608

#define MGMSG_CLK_SET_OUTPUT_CONFIG					0x4609
#define MGMSG_CLK_REQ_OUTPUT_CONFIG					0x460A
#define MGMSG_CLK_GET_OUTPUT_CONFIG					0x460B

#define MGMSG_CLK_SET_INPUT_FILTER					0x460C
#define MGMSG_CLK_REQ_INPUT_FILTER					0x460D
#define MGMSG_CLK_GET_INPUT_FILTER					0x460E

// DZimmerman for I2C
#define MGMSG_READ_CPLD_PGM_STATUS					0x46FE // I2C READ returns single byte STATUS, polled by I2C master
#define MGMSG_CPLD_REPROGRAM						0x46FF // .jed line count for reprogramming

#endif
