
/**
 * @file
 *
 * @brief APT Commands
 *
 *
 *
 *
 *
<b> MGMSG_HW_REQ_INFO 	&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;  0x0005
\n  MGMSG_HW_GET_INFO 	&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;  0x0006 </b>

<b>Function: </b>
Sent to request hardware information from the controller.
\n<b>REQ: </b>
\nCommand structure (6 bytes):
\n
<table>
<tr><td>0<td>1<td>2<td>5<td>4<td>5
<tr><td colspan="6">header only
<tr><td>05<td>00<td>00<td>00<td>d<td>s
</table>

Example: Request hardware info from controller #1
\n TX 05, 00, 00, 00, 11, 01

\n GET:
\n Response structure (90 bytes):
\n 6 byte header followed by 84 byte (0x54) data packet as follows:

<table>
<tr>
<td>&nbsp;0<td>&nbsp;1<td>&nbsp;2<td>&nbsp;3<td>&nbsp;4<td>&nbsp;5<td>&nbsp;6<td>&nbsp;7<td>&nbsp;8<td>&nbsp;9
<td style="width:4%">10<td>11<td>12<td>13<td>14<td>15<td>16<td>17<td>18<td>19
<tr>
<td colspan="6" style="text-align:center;">header
<td colspan="4" style="text-align:center;">Serial Number
<td colspan="8" style="text-align:center;">Model Number
<td colspan="2" style="text-align:center;">Type
</table>

<table>
<tr>
<td>20<td>21<td>22<td>23<td>24<td>25<td>26<td>27<td>28<td>29
<td>30<td>31<td>32<td>33<td>34<td>35<td>36<td>37<td>38<td>39
<tr><td colspan="4" style="text-align:center;">Firmware Version
<td colspan="16" style="text-align:center;">For internal use only
</table>

<table>
<tr><td>40<td>41<td>42<td>43<td>44<td>45<td>46<td>47<td>48<td>49
<td>50<td>51<td>52<td>53<td>54<td>55<td>56<td>57<td>58<td>59
<tr><td colspan="20" style="text-align:center;">For internal use only
</table>

<table>
<tr><td>60<td>61<td>62<td>63<td>64<td>65<td>66<td>67<td>68<td>69
<td>70<td>71<td>72<td>73<td>74<td>75<td>76<td>77<td>78<td>79
<tr><td colspan="20" style="text-align:center;">For internal use only
</table>

<table>
<tr><td>80<td>81<td>82<td>83<td>84<td>85<td>86<td>87<td>88<td>89
<tr><td colspan="4" style="text-align:center;">For internal use only
<td colspan="2" style="text-align:center;">HW Version
<td colspan="2" style="text-align:center;">Mod State
<td colspan="2" style="text-align:center;">nchs
</table>

 */

/****************************************************************************
 * Defines
 ****************************************************************************/
#ifndef APT_H
#define APT_H

#include "apt-deprecation.h"                    // No region

#define HOST_ID							0x01	// source

enum asf_destination_ids	// destinations
{
    MOTHERBOARD_ID 				= 0x11,
    MOTHERBOARD_ID_STANDALONE 	= 0x50,
    SLOT_1_ID 					= 0x21,
    SLOT_2_ID 					= 0x22,
    SLOT_3_ID 					= 0x23,
    SLOT_4_ID 					= 0x24,
    SLOT_5_ID					= 0x25,
    SLOT_6_ID 					= 0x26,
    SLOT_7_ID 					= 0x27,
    SYNC_MOTION_ID 				= 0x28,
    SIPM_1_ID                   = 0x51,
    SIPM_2_ID                   = 0x52,
    SIPM_3_ID                   = 0x53,
    SIPM_4_ID                   = 0x54,
    SIPM_5_ID                   = 0x55,
    SIPM_6_ID                   = 0x56,
    SIPM_7_ID                   = 0x57,
    SIPM_8_ID                   = 0x58,
};

#define APT_COMMAND_SIZE		6


// ! The "region" inline comment is used in the validation script.
// ! For the purposes of validation, the comment is *not* optional.


#include "apt-regions/apt-bootloader.h"         // Region:  0x0000 - 0x00FF




#include "apt-regions/apt-external.h"           // Region:  0x0000 - 0x08FF

/****************************************************************************
 * External APT Region - Hall Sensors                                       *
 ****************************************************************************/
#define MGMSG_HS_REQ_STATUSUPDATE					0x0482
#define MGMSG_HS_GET_STATUSUPDATE					0x0483

/****************************************************************************
 * External APT Region - Lattice Debug                                      *
 ****************************************************************************/
#define MGMSG_LATTICE_JED_UPDATE					0x0855
#define MGMSG_DBG_PRINT								0x0856

/****************************************************************************
 * Virtual Moves (depriciated)                                              *
 ****************************************************************************/
#define MGMSG_MOT_SET_VIRTUAL_P1					APT_DEPRECATION_ERROR("deprecated") //0x0857
#define MGMSG_MOT_REQ_VIRTUAL_P1					APT_DEPRECATION_ERROR("deprecated") //0x0858
#define MGMSG_MOT_GET_VIRTUAL_P1					APT_DEPRECATION_ERROR("deprecated") //0x0859

#define MGMSG_MOT_SET_VIRTUAL_P2					APT_DEPRECATION_ERROR("deprecated") //0x085A
#define MGMSG_MOT_REQ_VIRTUAL_P2					APT_DEPRECATION_ERROR("deprecated") //0x085B
#define MGMSG_MOT_GET_VIRTUAL_P2					APT_DEPRECATION_ERROR("deprecated") //0x085C

#define MGMSG_MOT_SET_VIRTUAL_MODE					APT_DEPRECATION_ERROR("deprecated") //0x085D
#define MGMSG_MOT_REQ_VIRTUAL_MODE					APT_DEPRECATION_ERROR("deprecated") //0x085E
#define MGMSG_MOT_GET_VIRTUAL_MODE					APT_DEPRECATION_ERROR("deprecated") //0x085F

/****************************************************************************
 * Stickscope Commands (depricated)
 ****************************************************************************/
#include "apt-regions/apt-stick-scope.h"            // No region


/****************************************************************************
 * External APT Region - Resonant Scanner                                   *
 ****************************************************************************/
#define MGMSG_SET_RES_SCAN_ZOOM						0x0859
#define MGMSG_SET_RES_SCAN_OFFSET					0x085A

// 3 Photon Sampling
/****************************************************************************
 * External APT Region - 3-Photon                                           *
 ****************************************************************************/
#define MGMSG_SET_3P_CONFIG							0x0860
#define MGMSG_REQ_3P_CONFIG							0x0861
#define MGMSG_GET_3P_CONFIG							0x0862
#define MGMSG_3P_SAVE_CONFIG						0x0863
/****************************************************************************
 * Sterling VA APT Commands (0x4000 - 0x4FFF)                               *
 ****************************************************************************/
#include "apt-regions/apt-motion-controllers.h" // Region:  0x4000 - 0x41FF
                                                // Empty:   0x4200 - 0x42FF
#include "apt-regions/apt-prelude.h"            // Region:  0x4300 - 0x43FF
                                                // Empty:   0x4400 - 0x44FF
#include "apt-regions/apt-pmt.h"                // Deprecated header
#include "apt-regions/apt-detector.h"           // Region:  0x4500 - 0x45FF
#include "apt-regions/apt-dual-plane.h"         // Region:  0x4600 - 0x46FF
#include "apt-regions/apt-flipper-mirror.h"     // Region:  0x4800 - 0x481F
#include "apt-regions/apt-bcm-pa.h"             // Region:  0x4820 - 0x483F

/****************************************************************************
 * Depr
 ****************************************************************************/

/****************************************************************************
 * APT Region - To-Be-Moved Commands (0x4F00 - 0x4FFF)                      *
 ****************************************************************************/
#define MGMSG_SIG_GEN_SET							0x4FFF


/****************************************************************************
 * Non-APT Region - Testing (0xFF00 - 0xFFFF)
 ****************************************************************************/
#define MSMSG_REQ_SYS_TEMP							0xfff2
#define MSMSG_GET_SYS_TEMP							0xfff3
#define MSMSG_GET_SYS_PWM							0xfff4
#define MSMSG_GET_SYS_PWM2							0xfff5

#endif // APT_H
