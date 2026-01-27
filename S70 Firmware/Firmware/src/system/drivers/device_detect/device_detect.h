/**
 * @file device_detect.h
 *
 * @brief Functions for ???
 *
 */

#ifndef SRC_SYSTEM_DRIVERS_DEVICE_DETECT_DEVICE_DETECT_H_
#define SRC_SYSTEM_DRIVERS_DEVICE_DETECT_DEVICE_DETECT_H_

#include "gate.h"
#include "slot_types.h"
#include "queue.h"
#include "defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * Defines
 ****************************************************************************/
/*Cable defines*/
#define DEVICE_GOOD            0
#define DEVICE_ERROR        1

#define NO_DEVICE_CHECK_ID      0x0000
#define DEVICE_CHECK_INCOMPLETE 0xFFFF

#define OW_RESET_PRESENCE_BAUD        9600    // bit width 104.17us frame with 937.5us
#define OW_READ_WRITE_BAUD            115200    // bit width 8.68us frame with 78.13us


#define OW_CHECK_DEVICE_INTERVAL_MS        (100/NUMBER_OF_BOARD_SLOTS)

// Commands
#define OW_PRESENCE                    0xF0
#define OW_PRESENCE_DETECTED           0xE0
#define OW_RD_ROM                     0x33
#define OW_SKIP_ROM                 0xCC
#define OW_RESUME                     0xA5
#define OW_SKIP_ROM_OVERDRIVE          0x3C    // Changes to high speed after this command is issued
#define OW_WR_SCRATCHPAD               0x0F
#define OW_RD_SCRATCHPAD               0xAA
#define OW_COPY_SCRATCHPAD             0x55    // copy data from the scratchpad to the data memory
#define OW_RD_MEMORY                   0xF0


// state machine for onewire
//#define    OW_IDEL                        0
#define    OW_SEND_RESET_SIGNAL        1
#define    OW_WAIT_PRESENSE_SIGNAL        2
#define    OW_WRITE_SRCATCH_PAD        3
#define    OW_READ_SRCATCH_PAD            4
#define    OW_COPY_SRCATCH_PAD            5
#define    OW_READ_FLASH                6
#define    OW_READ_MEMORY                7    // used for reading serial number


#define DEVICE_DETECT_DELAY_LOOPS                    2
#define DEVICE_STORAGE_BYTES_READ_UNTIL_YIELD        4

#define DEVICE_DETECT_V1                           250

#define DEVICE_DETECT_MISS_THRESHOLD                 2       // number of fail device detections to consider a device disconnected

#define DEVICE_DETECT_STATIC_BUFFER_SIZE            16      // Size of the static buffer (in bytes) for storing temporary data.

#define DEVICE_DETECT_PROGRAMMING_BUFFER_SIZE       64      // Size of the programming buffer (in bytes).
                                                            // Should be a multiple of SCRATCHPAD_SIZE.

#define DEVICE_DETECT_PART_NUMBER_LENGTH            15

//                                                  0x0000????????????  // Reserved for 48-bit serial numbers of the one-wire devices.
#define OW_SERIAL_NUMBER_WILDCARD                   0xFFFF000000000000  // Any device will succeed the serial number check.
#define OW_SERIAL_NUMBER_EMPTY                      0xFFFFFFFFFFFFFFFF  // All devices will fail.

// Device config type defined in defs.h

/// \todo How do I handle config data stored in one-wire?

/****************************************************************************
 * Public Data
 ****************************************************************************/

typedef struct  __attribute__((packed))
{
    device_signature_t signature;
    uint64_t serial_num;    // actual 48 bit
    char product_name[DEVICE_DETECT_PART_NUMBER_LENGTH]; // 15-character string (fixed width or null terminated)
} device_info;

typedef enum {
    // Version Independent
    _DDS_NO_DEVICE = 0,
    _DDS_CONNECTION_DELAY,
    _DDS_UNKNOWN_DEVICE,
    _DDS_READ_HEADER,
    _DDS_PROGRAMMING,
    _DDS_PROGRAMMING_VERIFY,
    _DDS_STATIC_DEVICE,        ///> Used for devices that do NOT support one-wire, or otherwise can't use device detection.
    _DDS_QP_SIGNATURE_START,
    _DDS_CHECK_VERSION,
    _DDS_PASSED,                        // State for allowed devices

    // Added in version 1 (refactor)
    _DDS_GET_HEADER,                    // Read in the header
    _DDS_CHECK_HEADER,                  // Add the header to rolling checksum and determine what state to go to (based on lengths)
    _DDS_GET_DEVICE_ENTRY_HEADER,       // Read in the header for the device entry
    _DDS_CHECK_DEVICE_ENTRY_HEADER,     // Check to see if the slot card is the one running, while adding data to the checksum
    _DDS_PROCESS_DEVICE_ENTRY,          // Reads in the data from the entry
    _DDS_GET_CONFIG_ENTRIES,            // Reads in the custom config signature
    _DDS_CHECK_CONFIG_ENTIRES,          // Adds to the checksum
    _DDS_VALIDATE_CHECKSUM,

    _DDS_BAD_DEVICE = 0xFF
} _device_detect_state_t;

/// \warning These enums are encoded into the APT commands, so don't change them once they are declared.
typedef enum {
    // General Success         [0]
    OWPE_OKAY = 0,

    // Errors                [1,   149]
    OWPE_NO_DEVICE,
    OWPE_DEVICE_DOES_NOT_SUPPORT_OW,
    OWPE_ALREADY_PROGRAMMING,
    OWPE_NOT_PROGRAMMING,
    OWPE_OUT_OF_MEMORY,
    OWPE_PACKET_TOO_LARGE,

    // Events (error)        [150, 199]
    // None.

    // Events (no error)    [200, 255]
    // None
} OW_Programming_Error_t;

typedef enum {
    _NOT_PROGRAMMING = 0,   // Device is not programming.
    _RAW_PROGRAMMING,       // Device is in raw programming mode (ow programming handshake).
    _QP_SIGNATURE,          // Device is quickly programming a new signature.
} _device_detect_program_mode_t;

typedef struct
{
    // Connected Device Info Fields (needs slot mutex)
    device_info info;
    GateHandle_t connection_gate;               ///> Openned when a device can be used.  Otherwise, closed.
                                                ///  \note Does NOT need slot mutex.

    // Public Device-Detect Fields (needs slot mutex)
    bool is_save_structure;                     ///> Set to true when the data in p_slot_config is a save structure.
                                                ///  False indicates that it is a configuration structure.
    config_signature_t * p_slot_config;         ///> Data buffer for the custom device entry from the device.
                                                ///  \warning Slot driver must free memory and set to NULL.
    uint16_t p_slot_config_size;                ///> Size of p_slot_config in bytes.  Can be used to determine if a config is out-of-date.
    uint8_t * p_config_entries;                 ///> Custom LUT entries for the device.
    uint16_t p_config_entries_size;             ///> Size of p_config_entries in bytes.
    

    // One-Wire Fields (managed by device detect task, so no mux is needed)
    uint8_t _ow_state;
    uint8_t _ow_next_state;
    uint8_t _TA1;
    uint8_t _TA2;
    uint8_t _ES;
    uint16_t _address;
    uint8_t * _p_ow_buffer;
    uint16_t _bytes_to_read;

    // Device-Detect FSM (managed by device detect task, so no mux is needed)
    _device_detect_state_t _dd_state;
    uint8_t _dd_delay_counter;                  // Counter that determines the amount of loops before the device can be processed.
    uint8_t _dd_fail_count;                     // Number of times the device failed to be detected
    bool _dd_abort_device_detection;            // Flag that indicates if device-detection should be skipped.
                                                // Reset after use.
    _device_detect_program_mode_t _dd_programming;
                                                // the unknown device state (false) or programming state (true).
                                                // When set low, the FSM will exit the programming state.
    uint8_t * _dd_p_programming_buffer;
    uint16_t _dd_data_in_buffer;
    uint16_t _dd_programming_address;
    uint8_t _dd_previous_status_flags;

    // Device-Detect Command Queue (interfaced with public device-detect calls)
    QueueHandle_t _dd_command_queue;            // Mailbox handling DD commands from public interfaces.
    
} Device;

typedef struct __attribute__((packed))
{
    config_signature_t signature;
    uint16_t index;
} One_Wire_Config_Header_t;


/**
 * Initializes the device detection task.
 */
void device_detect_init(void);


//
/**
 * \brief Called to restart device detection for the selected.
 * Also causes the system to reload slot configuration into its local memory.
 * Useful when LUT is modified, allowed devices is modified, device detect is disabled, etc.
 * \param p_device Pointer to the device to reset.
 */
void device_detect_reset(Device * const p_device);

/**
 * Changes the programming state of a device.
 * \note Static devices cannot be programmed.
 * \note Devices can be programmed over any slot.
 * \param[in]       p_device Device to program.
 * \param[in]       enable_programming If the device should be programming.
 * \param[out]      p_callback Pointer to the callback function to call when a response is ready.  May be NULL.
 * \param[in]       p_callback_param Parameter passed to callback function.  May be NULL.  
 */
void device_detect_change_programming(Device * const p_device,
    const bool enable_programming, void (*p_callback)(void * const, const OW_Programming_Error_t),
    void * const p_callback_param);

/**
 * Checks the status of the device's programming.
 * \param[in]       p_device Device to check.
 * \param[out]      p_callback Pointer to the callback function to call when a response is ready.  May be NULL.
 * \param[in]       p_callback_param Parameter passed to callback function.  May be NULL.  
 */
void device_detect_check_programming(Device * const p_device,
    void (*p_callback)(void * const, const OW_Programming_Error_t), void * const p_callback_param);

/**
 * Handles a programming packet sent to the device, writing it to one-wire.
 * \note The lifetime of p_data_to_write must be maintained until p_callback is called.
 * \param[in]       p_device Device to program.
 * \param[in]       p_data_to_write Pointer to a buffer of data to write.
 * \param[in]       length_of_data Length of data to write.
 * \param[in]       p_callback Pointer to the callback function to call when a response is ready.  May be NULL.
 *                             Also provides the parameter p_data_to_write as a parameter for ease of lifetime control.
 * \param[in]       p_callback_param Parameter passed to callback function.  May be NULL.  
 */
void device_detect_handle_ow_programming_packet(
    Device * const p_device, uint8_t * const p_data_to_write,
    uint16_t length_of_data, void (*p_callback)(void * const, const OW_Programming_Error_t, void * const),
    void * const p_callback_param);

    
#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_DRIVERS_DEVICE_DETECT_DEVICE_DETECT_H_ */
