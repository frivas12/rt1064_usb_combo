// max3421e_usb.h

#ifndef SRC_DRIVERS_USB_HOST_MAX3421E_USB_H_
#define SRC_DRIVERS_USB_HOST_MAX3421E_USB_H_

#ifdef __cplusplus
extern "C"
{
#endif


/****************************************************************************
 * Defines
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
int8_t usb_read(uint8_t address, uint8_t endpoint, uint16_t length, uint8_t* buf,
		uint8_t* bytes_read, uint16_t nak_limit);
int8_t usb_write(uint8_t address, uint8_t endpoint, uint16_t length,
		uint8_t* data, uint16_t nak_limit);
uint8_t control_read(uint8_t address, uint8_t *pSUD, uint8_t* buf);
uint8_t control_write(uint8_t *pSUD);
void set_max3421_address(uint8_t address);
uint8_t get_device_descriptors(uint8_t address);
uint8_t set_device_address(uint8_t address);
uint8_t get_rest_of_discriptors(uint8_t address);

#ifdef __cplusplus
}
#endif

#endif /* SRC_DRIVERS_USB_HOST_MAX3421E_USB_H_ */
