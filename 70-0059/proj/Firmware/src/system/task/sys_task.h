/**
 * @file sys_task.h
 *
 * @brief Functions for ???
 *
 */

#ifndef SRC_SYSTEM_TASK_SYS_TASK_H_
#define SRC_SYSTEM_TASK_SYS_TASK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "UsbCore.h"
#include <asf.h>
#include "slots.h"

/****************************************************************************
 * Defines
 ****************************************************************************/
/**
 * Task information is all in this file so that we can view and adjust them all in one place.
 */

// The maximum amount of event-type watchdogs for all tasks.
#define TASK_EVENT_WATCHDOGS                    0

// The maximum amount of heartbeat-type watchdogs for all tasks.
#define TASK_HEARTBEAT_WATCHDOGS                ((NUMBER_OF_BOARD_SLOTS + 2))

/**
 * USB slave task.
 * This task grabs packets of data from the PC USB port then validates the packet
 * and parses the data.  If the data belongs to a slot then it is passed on to that
 * slot task.  If not it will handle the data itself such as ASF commands like
 * MGMSG_HW_REQ_INFO
 */
#define TASK_USB_SLAVE_STACK_SIZE       		(1536/sizeof(portSTACK_TYPE))
#define TASK_USB_SLAVE_PRIORITY            		(tskIDLE_PRIORITY)
#define USB_SLAVE_UPDATE_TIMEOUT				pdMS_TO_TICKS(1000)

/**
 * Stepper task
 */
#define TASK_STEPPER_STACK_SIZE       			(2048/sizeof(portSTACK_TYPE)) //2048
#define TASK_STEPPER_STACK_PRIORITY       	  	( ( UBaseType_t ) 1U )
#define STEPPER_UPDATE_INTERVAL					pdMS_TO_TICKS(10)
#define STEPPER_HEARTBEAT_INTERVAL              (2*STEPPER_UPDATE_INTERVAL)
#define STEPPER_CONFIGURING_INTERVAL            pdMS_TO_TICKS(200)

/**
 * Servo task
 */
#define TASK_SERVO_STACK_SIZE       			(1024/sizeof(portSTACK_TYPE))
#define TASK_SERVO_STACK_PRIORITY         		( ( UBaseType_t ) 1U )
#define SERVO_UPDATE_INTERVAL					pdMS_TO_TICKS(10)
// TODO: Heartbeat interval

/**
 * OTM DAC task
 */
#define TASK_OTM_DAC_STACK_SIZE       			(1024/sizeof(portSTACK_TYPE))
#define TASK_OTM_DAC_STACK_PRIORITY        		( ( UBaseType_t ) 1U )
#define OTM_DAC_UPDATE_INTERVAL					pdMS_TO_TICKS(10)

/**
 * OTM RS232 task
 */
#define TASK_OTM_RS232_STACK_SIZE       		(1024/sizeof(portSTACK_TYPE))
#define TASK_OTM_RS232_STACK_PRIORITY      		( ( UBaseType_t ) 1U )
#define OTM_RS232_UPDATE_INTERVAL				pdMS_TO_TICKS(10)

/**
 * IO task
 */
#define TASK_IO_STACK_SIZE       				(1024/sizeof(portSTACK_TYPE))
#define TASK_IO_STACK_PRIORITY        			( ( UBaseType_t ) 1U )
#define IO_UPDATE_INTERVAL						pdMS_TO_TICKS(10)

/**
 * Shutter task
 */
#define TASK_SHUTTER_STACK_SIZE       			(1024/sizeof(portSTACK_TYPE))
#define TASK_SHUTTER_STACK_PRIORITY        		(tskIDLE_PRIORITY )
//#define TASK_SHUTTER_STACK_PRIORITY        		( ( UBaseType_t ) 1U )
#define SHUTTER_UPDATE_INTERVAL					pdMS_TO_TICKS(10)
// TODO: Heartbeat interval

/**
 * Flipper Shutter task
 */
#define TASK_FLIPPER_SHUTTER_STACK_SIZE       			(2800/sizeof(portSTACK_TYPE))
#define TASK_FLIPPER_SHUTTER_PRIORITY                   ( ( UBaseType_t ) 1U )
#define FLIPPER_SHUTTER_UPDATE_INTERVAL					pdMS_TO_TICKS(10)
#define FLIPPER_SHUTTER_HEARTBEAT_INTERVAL              (2*FLIPPER_SHUTTER_UPDATE_INTERVAL)
#define FLIPPER_SHUTTER_CONFIGURING_INTERVAL            pdMS_TO_TICKS(200)

/**
 * Supervisor task
 */
#define TASK_SUPERVISOR_CHECK_STACK_SIZE     	(1024/sizeof(portSTACK_TYPE))
#define TASK_SUPERVISOR_CHECK_STACK_PRIORITY   	(tskIDLE_PRIORITY)
#define SUPERVISOR_TIMEOUT						pdMS_TO_TICKS(10)

/**
 * USB Host task
 */
#define TASK_USB_HOST_STACK_SIZE     			(1024/sizeof(portSTACK_TYPE))
#define TASK_USB_HOST_STACK_PRIORITY   			(tskIDLE_PRIORITY)
#define USB_HOST_TIMEOUT						pdMS_TO_TICKS(1000)
#define USB_HOST_ROOT_PORT_TASK_RATE			pdMS_TO_TICKS(100) /* in ms */

/**
 * USB device task
 */
#define TASK_USB_HOST_DEVICE_STACK_SIZE 		(1024/sizeof(portSTACK_TYPE))
#define TASK_USB_HOST_DEVICE_STACK_PRIORITY		(tskIDLE_PRIORITY)
#define USB_HOST_DEVICE_TIMEOUT					pdMS_TO_TICKS(3000)
#define USB_HOST_DEVICE_TASK_RATE				pdMS_TO_TICKS(10) /* in ms */

/**
 * FTDI task
 */
#define TASK_FTDI_STACK_SIZE     				(1024/sizeof(portSTACK_TYPE))
#define TASK_FTDI_STACK_PRIORITY   				(tskIDLE_PRIORITY)
#define FTDI__TASK_RATE							pdMS_TO_TICKS(10)

/**
 * stepper synchronized motion task
 */
#define TASK_STEPPER_SM_STACK_SIZE     			(3072/sizeof(portSTACK_TYPE))
#define TASK_STEPPER_SM_STACK_PRIORITY  		(tskIDLE_PRIORITY)
#define STEPPER_SM_TIMEOUT						pdMS_TO_TICKS(1000)
#define STEPPER_SM_TASK_RATE					pdMS_TO_TICKS(10) /* in ms */  //TODO EL changed to keep up with 10ms USB host messages

/**
 * Piezo task
 */
#define TASK_PIEZO_STACK_SIZE       				(1024/sizeof(portSTACK_TYPE))
#define TASK_PIEZO_STACK_PRIORITY        			(tskIDLE_PRIORITY)
#define PIEZO_UPDATE_INTERVAL						pdMS_TO_TICKS(1)
// TODO: Heartbeat interval

/**
 * Device Detect Task
 */
#define TASK_DEVICE_DETECT_STACK_SIZE               (1024/sizeof(portSTACK_TYPE))
#define TASK_DEVICE_DETECT_STACK_PRIORITY           (tskIDLE_PRIORITY)
#define DEVICE_DETECT_UPDATE_INTERVAL               pdMS_TO_TICKS(10)
#define DEVICE_DETECT_HEARTBEAT_INTERVAL            pdMS_TO_TICKS(5000)

/**
 * Standard task
 */
#define TASK_STANDARD_STACK_SIZE       			(1024/sizeof(portSTACK_TYPE))
#define TASK_STANDARD_STACK_PRIORITY        	( ( UBaseType_t ) 1U )
#define STANDARD_UPDATE_INTERVAL				pdMS_TO_TICKS(10)


/****************************************************************************
 * Public Data
 ****************************************************************************/
/*Task Handles*/
extern TaskHandle_t xDeviceHandle[USB_NUMDEVICES]; /*0 is used for the USB host task*/

/*Holds the handle for all slot task*/
extern TaskHandle_t xSlotHandle[NUMBER_OF_BOARD_SLOTS];

/*Holds the handle for the supervisor task*/
extern TaskHandle_t xSupervisorHandle;

/*Stepper Synchronized Motion Task Handle*/
extern TaskHandle_t xStepperSMHandle;

/*Device Detect Task Handle*/
extern TaskHandle_t xDeviceDetection;

/*Queue Handles*/

/* Queue handle for receiving data from a slot stepper to the synchronized motion task*/
// extern QueueHandle_t xStepper_SM_Rx_Queue[NUMBER_OF_BOARD_SLOTS];

/* Queue handle for sending data from the synchronized motion task to a slot stepper task*/
// extern QueueHandle_t xStepper_SM_Tx_Queue[NUMBER_OF_BOARD_SLOTS];

/* Queue handle for receiving data from synchronized stepper task to a slot piezo task*/
// extern QueueHandle_t xPiezo_SM_Rx_Queue[NUMBER_OF_BOARD_SLOTS];

/* Queue handle for sending data from the slot piezo task to the synchronized stepper task*/
// extern QueueHandle_t xPiezo_SM_Tx_Queue[NUMBER_OF_BOARD_SLOTS];

#define SLOT_SM_QUEUE_INDEX		8


/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
void suspend_all_task(void);
void resume_all_task(void);

#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_TASK_SYS_TASK_H_ */
