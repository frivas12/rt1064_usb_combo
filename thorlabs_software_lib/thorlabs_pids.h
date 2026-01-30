//  thorlabs_pids.h

#ifndef THORLABS_SOFTWARE_LIB_THORLABS_PIDS_H_
#define THORLABS_SOFTWARE_LIB_THORLABS_PIDS_H_

#include "./apt-deprecation.h"

/****************************************************************************
 * Defines
 ****************************************************************************/


/****************************************************************************
 * ! IMPORTANT !
 * Thorlabs Virginia has been allocated the range [0x2000, 0x2FFF].
 * See BW topic 10789 for more details.
 * Do *NOT* allocate any values outside of this range!
 ****************************************************************************/

#define  THORLABS_USB_DEVICE_VID                    0x1313

//Bootloader PIDs
#define  THORLABS_CDC_BOOTLOADER_PID                0x2F02
#define  THORLABS_MCM_BOOTLOADER_PID                0x2F03

// Devices
//       PID Macro                                  PID Value (Hex)             // Description (typically FGPN)
#define  THORLABS_MCM6000_PID                       0x2003                      // Note:  MCM60 has the SAME PID as the MCM6000.  // TODO:  Change the MCM6000's PID.
#define  THORLABS_3_AXIS_PID                        0x2005                      // MCMK3
#define  THORLABS_OBJECTIVE_MOVER_RES_PID           0x2006                      // 
#define  THORLABS_SCANNER_GALVO_RES_PID             0x2007                      // 
#define  THORLABS_EPI_TURRET_6_POSITION_PID         0x2008                      // 
#define  THORLABS_3_PHOTON_SAMPLING_PID             0x2009                      // 
#define  THORLABS_MCP7_PID                          0x200A                      // MCMK4S
#define  THORLABS_RGG_PID                           0x200B                      // 
#define  THORLABS_SCANNER_GALVO_RES12KHZ_PID        0x200C                      // 
#define  THORLABS_DUAL_AXIS_JS_PID                  0x200D                      // MCMJ1
#define  THORLABS_CDC_DEVICE_PID                    0x200E                      // 
#define  THORLABS_CLK_RX_MEZZANINE_PID              0x200F                      // 
#define  THORLABS_GRIFFIN_PID                       0x2010                      // 
#define  THORLABS_SINGLE_AXIS_JS_PID                0x2011                      // VJS10
//#define  THORLABS_PMT_CDC_PID                     0x2012                      // never actually put into use
#define  THORLABS_PUSHBUTTON_JS_PID                 0x2013                      // 
#define  THORLABS_LED_CHAMBER_PID                   0x2014                      // 
#define  THORLABS_DUAL_PLANE_PID                    APT_DEPRECATION_ERROR("Please use THORALBS_UPDATED_DUAL_PLANE_PID due to PID collision")
#define  THORLABS_PRELUDE_PID                       0x2015                      // Prelude
#define  THORLABS_MCM301                            0x2016                      // 
#define  THORLABS_FLIPPER_MIRROR_V2                 0x2017                      // 
#define  THORLABS_MCMK1                             0x2018                      // MCMK1
#define  THORLABS_BCM_PA_V2                         0x2019                      // BCM-PA v2
#define  THORLABS_MCMJ1S                            0x201A                      // MCMJ1S
#define  THORLABS_MCMK4                             0x201B                      // MCMK4
#define  THORLABS_UPDATED_DUAL_PLANE_PID            0x201C                      //
#define  THORLABS_REMOTE_FOCUS_LCPG_PID             0x201D                      //
#define  THORLABS_MCM6101                           0x201E
#define  THORLABS_MCM6101F                          0x201F

//*******************************************************************************
// 0x2100 - 0x210F Reserved for MCM Plug-and-Play-Compatible Controllers
// Software should target this range to reduce changes.

#define  THORLABS_PMT_PID                           0x2F00                      // This is the TMC device PID
//*******************************************************************************
//0x2E00 - 0x2E80 Reserved for PMT Products
#define  THORLABS_PMT1000_PID                       0x2E01                      // 
#define  THORLABS_PMT2100_PID                       0x2E02                      // 
#define  THORLABS_PMT2106_PID                       0x2E03                      // GaAsP with shutter
#define  THORLABS_HPD1000_PID                       0x2E06                      // 
#define  THORLABS_SIPM100_PID                       0x2E07                      // 
#define  THORLABS_PMT2110_PID                       0x2E08                      // Higher BW version of PMT2100
#define  THORLABS_PMT3100_PID                       0x2E09                      // Lower gain version with different BW settings
#define  THORLABS_PMT5100_PID                       0x2E0A                      // PMT2100 Replacement
#define  THORLABS_PMT5103_PID                       0x2E0B						// Lower gain version with same BW settings
#define  THORLABS_SIPM400_PID                       0x2E0C                      // 
#define  THORLABS_PMT6000_PID                       0x2E0D						// PMT1000 CDC Replacement

#endif /* THORLABS_SOFTWARE_LIB_THORLABS_PIDS_H_ */
