// usb_slave.h

#ifndef SRC_SYSTEM_DRIVERS_USB_SLAVE_USB_SLAVE_H_
#define SRC_SYSTEM_DRIVERS_USB_SLAVE_USB_SLAVE_H_

#ifdef __cplusplus
extern "C" {
#endif

// #include "compiler.h"
// #include "board.h"
// #include "slots.h"
#include <FreeRTOS.h>

#include "../buffers/fifo.h"
#include "semphr.h"

/****************************************************************************
 * Defines
 ****************************************************************************/
#define USB_SLAVE_BUFFER_SIZE 125
#define USB_SLAVE_QUEUE_LENGTH 1

#define PC_PORT 0
#define TABLET_PORT 1

#define MAX_NUM_APT_FAILS 5

#define SERIAL_NO_DIGITS 17

#define APT_PARSER_TIMEOUT_PERIOD pdMS_TO_TICKS(400)

/****************************************************************************
 * Public Data
 ****************************************************************************/
/* Define the data type that will be queued. */
typedef struct {
    iram_size_t (*write)(const void*, iram_size_t);
    SemaphoreHandle_t xUSB_Slave_TxSemaphore;
    uint16_t ucMessageID;
    uint16_t ExtendedData_len;
    uint8_t param1;
    uint8_t param2;
    uint8_t destination;
    uint8_t source;
    uint8_t extended_data_buf[USB_SLAVE_BUFFER_SIZE];
    bool bHasExtendedData;
    bool ftdi_flag;
} USB_Slave_Message;

typedef enum {
    USBSMBS_COMMAND_OVERFLOWED  = -2,
    USBSMBS_REALIGN_UNAVAILABLE = -1,
    USBSMBS_IDLE                = 0,
    USBSMBS_BUILDING            = 1,
    USBSMBS_GOT_COMMAND         = 2,
} USB_Slave_Message_Builder_State;

typedef struct {
    uint8_t* buffer;
    size_t buffer_length;
    size_t buffer_capacity;
    size_t counter;
    USB_Slave_Message_Builder_State state;
    uint8_t realigned_try_cnt;
    uint8_t max_realigned_attempts;
} USB_Slave_Message_Builder;

extern SemaphoreHandle_t xUSB_Slave_TxSemaphore;

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
void prvUSB_slave_Rx_Handler(uint8_t port);
void usb_slave_init(void);

void usb_slave_message_builder_init(USB_Slave_Message_Builder* out,
                                    uint8_t* buffer, size_t buffer_size,
                                    uint8_t max_realign_attempts);

/// \brief Resets the state of the message builder to its initial conditions.
void usb_slave_message_builder_reset(USB_Slave_Message_Builder* out);

void* usb_slave_message_builder_input_dest(USB_Slave_Message_Builder* builder);

size_t usb_slave_message_builder_input_length(
    USB_Slave_Message_Builder* builder);

/// \brief Reads data from the source buffer into the builder.
/// \return The number of bytes copied into the builder.
size_t usb_slave_message_builder_read(USB_Slave_Message_Builder* builder,
                                      const void* src, size_t length);

/// \brief Returns the number of bytes needed to build commands.
/// \note Ensuring that the builder only has the exact bytes
///       needed guarantees that no array-shift operations occur.
///       This should be levereged if the implementation API
///       allows.
size_t usb_slave_message_builder_bytes_needed(
    const USB_Slave_Message_Builder* builder);

/**
 * Services the message builder to attempt to build a command or
 * dispose of a command that is too large.
 * \note The dest parameter must be the same until this function
 *       returns true.  This is because this function partially initializes
 *       dest.
 * \param[in]   builder       The builder to consume data from.
 * \param[out]  dest          The command to build.
 * \param[out]  overflow_size A pointer to store the size of the extended data
 *                            length when an overflow command is received.
 *                            Optional. Omit with NULL.
 * \return if the dest parameter was initialized
 */
bool usb_slave_message_builder_service(USB_Slave_Message_Builder* builder,
                                       USB_Slave_Message* dest,
                                       size_t* overflow_size);

/**
 * Services the message builder to attempt to build a command
 * or dispose of a command that is too large within the timeout.
 * This will block the calling thread until either a command is built or the
 * timeout is reached.
 * \param[in]     builder                 The builder to service.
 * \param[inout]  message                 The command to build into.
 * \param[in]     timeout                 The maximum amount of time a message
 *                  should take. Messages not complete by this timeout will be
 *                  discarded.
 * \param[in]     callback_context        A user-defined variable to pass into
 *                  the callbacks.
 * \param[in]     callback_read           The function to call to read bytes
 *                  into the builder.
 * \param[in]     callback_on_error_state If not NULL, a function to call
 *                  when the builder reaches an error condition.
 *                  The function will guarantee to return false if this occurs.
 *
 * Convention for callback_read:
 * (callback_context, buffer, to_read, timeout) -> bytes read into buffer
 *
 *
 * Convention for callback_on_error_state:
 * (callback_context, error state, union) -> void
 *    for USBSMBS_REALIGN_UNAVAILABLE, union = NULL
 *    for USBSMBS_COMMAND_OVERFLOWED, union = size_t* to overflow_size
 *
 * \return true  The message buffer has been filled with a valid command for
 *               parsing.
 * \return false The message buffer has not been filled with a valid command for
 *               parsing.
 */
bool usb_slave_message_builder_reading_service(
    USB_Slave_Message_Builder* const builder, USB_Slave_Message* message,
    TickType_t timeout, void* const callback_context,
    size_t (*callback_read)(void*, void*, size_t, TickType_t),
    void (*callback_on_error_state)(void*, USB_Slave_Message_Builder_State,
                                    void*));

#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_DRIVERS_USB_SLAVE_USB_SLAVE_H_ */
