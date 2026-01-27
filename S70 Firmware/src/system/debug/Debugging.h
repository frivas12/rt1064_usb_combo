// debug.h

#ifndef SRC_DEBUG_DEBUG_H_
#define SRC_DEBUG_DEBUG_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <SEGGER_RTT.h>
#include <conf_board.h>
#include <asf.h>

	/**
	 * function prototypes
	 */
#if DEBUG_ENABLE
// This one is used to print to regular terminal
//#define debug_print(n, ...) SEGGER_RTT_printf(0, n, ##__VA_ARGS__)
// This one is used to print to SystemView terminal
#define debug_print(n, ...) SEGGER_SYSVIEW_PrintfHost(n, ##__VA_ARGS__)
#else
#define debug_print(n, ...) (void)0
#endif

#if DEBUG_ENABLE && DEBUG_ERRORS
#define error_print(n, ...) SEGGER_SYSVIEW_PrintfHost(n, ##__VA_ARGS__)
#else
#define error_print(n, ...) (void)0
#endif

#if DEBUG_ENABLE && DEBUG_USB_FTDI
#define usb_ftdi_print(n, ...) SEGGER_SYSVIEW_PrintfHost(n, ##__VA_ARGS__)
#else
#define usb_ftdi_print(n, ...) (void)0
#endif

#if DEBUG_ENABLE && DEBUG_HID_REPORT
#define hid_report_print(n, ...) SEGGER_SYSVIEW_PrintfHost(n, ##__VA_ARGS__)
#else
#define hid_report_print(n, ...) (void)0
#endif

#if DEBUG_ENABLE && DEBUG_USB_HOST
#define usb_host_print(n, ...) SEGGER_SYSVIEW_PrintfHost(n, ##__VA_ARGS__)
#else
#define usb_host_print(n, ...) (void)0
#endif

#if DEBUG_ENABLE && DEBUG_USB_HOST_JOYSTICK
#define joystick_print(n, ...) SEGGER_SYSVIEW_PrintfHost(n, ##__VA_ARGS__)
#else
#define joystick_print(n, ...) (void)0
#endif

#if DEBUG_ENABLE && DEBUG_DEVICE_DETECT
#define device_print(n, ...) SEGGER_SYSVIEW_PrintfHost(n, ##__VA_ARGS__)
#else
#define device_print(n, ...) (void)0
#endif

//temp globals for ozone debugger
#if DEBUG_PID_OZONE
// PID
#if 1
float setpoint_s1;
int32_t encoder_s1;

#else
float setpoint_s1;
float setpoint_s2;
float setpoint_s3;
int32_t encoder_s1;
int32_t encoder_s2;
int32_t encoder_s3;
float integral_s1;
float integral_s2;
float integral_s3;

//sync motion
uint8_t sm_axis_longest_time;

#endif
#endif

#if DEBUG_PID_JSCOPE
char JS_RTT_UpBuffer[4096];    // J-Scope RTT Buffer
int  JS_RTT_Channel;       // J-Scope RTT Channel

typedef struct {
	int32_t encoder;
	int32_t command;
} JS_RTT_Data;
#endif


#ifdef __cplusplus
}
#endif

#endif /* SRC_DEBUG_DEBUG_H_ */
