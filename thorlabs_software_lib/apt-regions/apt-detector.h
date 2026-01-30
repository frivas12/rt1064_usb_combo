#ifndef APT_DETECTOR_H
#define APT_DETECTOR_H
/****************************************************************************
 * APT Region - Detector (0x4500 - 0x45FF)                                       *
 ****************************************************************************/
#define MGMSG_DET_REQ_INFO							0x4500
#define MGMSG_DET_GET_INFO							0x4501
#define MGMSG_DET_REQ_PARAMS						0x4502
#define MGMSG_DET_GET_PARAMS						0x4503
#define MGMSG_DET_SET_PARAMS						0x4504
#define MGMSG_DET_SET_STATE							0x4505
#define MGMSG_DET_REQ_STATE							0x4506
#define MGMSG_DET_GET_STATE							0x4507
#define MGMSG_DET_SET_FREQ							0x4508
#define MGMSG_DET_REQ_FREQ							0x4509
#define MGMSG_DET_GET_FREQ							0x450A
#define MGMSG_DET_TRIPCLR                           0x450B
#define MGMSG_DET_REQ_TRIP							0x450C
#define MGMSG_DET_GET_TRIP							0x450D
#define MGMSG_DET_REQ_TRIPCNT						0x450E
#define MGMSG_DET_GET_TRIPCNT						0x450F
#define MGMSG_DET_SET_GAIN							0x4510
#define MGMSG_DET_REQ_GAIN							0x4511
#define MGMSG_DET_GET_GAIN							0x4512
#define MGMSG_DET_SET_OFFSET						0x4513
#define MGMSG_DET_REQ_OFFSET						0x4514
#define MGMSG_DET_GET_OFFSET						0x4515
#define MGMSG_DET_SET_SER							0x4516
#define MGMSG_DET_SET_PRE_OFFSET					0x4517
#define MGMSG_DET_REQ_PRE_OFFSET					0x4518
#define MGMSG_DET_GET_PRE_OFFSET					0x4519
#define MGMSG_DET_SET_ATTENUATION					0x451A //RDS: now used for Hi-lo Amp Gain on PMT5100/41-0156
#define MGMSG_DET_REQ_ATTENUATION					0x451B
#define MGMSG_DET_GET_ATTENUATION					0x451C

// Unused: 0x451D - 0x451F

#define MGMSG_HPD_REQ_VBR							0x4520
#define MGMSG_HPD_GET_VBR							0x4521
#define MGMSG_HPD_SET_VBR							0x4522

#define MGMSG_DET_REQ_SATURATION_COUNT				0x4523
#define MGMSG_DET_GET_SATURATION_COUNT				0x4524

#define MGMSG_SIPM_SET_TEMPERATURE                  0x4525
#define MGMSG_SIPM_REQ_TEMPERATURE                  0x4526
#define MGMSG_SIPM_GET_TEMPERATURE                  0x4527
#define MGMSG_SIPM_SET_BIAS                         0x4528
#define MGMSG_SIPM_REQ_BIAS                         0x4529
#define MGMSG_SIPM_GET_BIAS                         0x452A

#define MGMSG_DET_SET_TRIP_MODEL                    0x452B
#define MGMSG_DET_REQ_TRIP_MODEL                    0x452C
#define MGMSG_DET_GET_TRIP_MODEL                    0x452D

// Unused: 0x452E - 0x4530

#define MGMSG_SIPM_OVERTEMPCLR                      0x4531
#define MGMSG_SIPM_REQ_TEMPSTATUS                   0x4532
#define MGMSG_SIPM_GET_TEMPSTATUS                   0x4533

#define MGMSG_DET__NotUsed                    		0x4534
#endif
