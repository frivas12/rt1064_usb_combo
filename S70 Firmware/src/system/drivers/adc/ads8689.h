/**
 * @file asd8689.h
 *
 * @brief Functions for ???
 *
 */

#ifndef SRC_SYSTEM_DRIVERS_ADC_ADS8689_ADC_H_
#define SRC_SYSTEM_DRIVERS_ADC_ADS8689_ADC_H_

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * Defines
 ****************************************************************************/
//  **************** ADS8689IPWR 16bit ADC ****************
#define PIEZO_ADC_SPI_MODE		_SPI_MODE_0
#define SPI_PIEZO_ADC_NO_READ 	PIEZO_ADC_SPI_MODE, SPI_NO_READ, CS_NO_TOGGLE, SLOT_CS(slot) + 1
#define SPI_PIEZO_ADC_READ 		PIEZO_ADC_SPI_MODE, SPI_READ, CS_NO_TOGGLE, SLOT_CS(slot) + 1

#define ADC_BUFFER_SIZE		50

#define DEVICE_ID_REG_23_16   0x02 //Device ID
#define RST_PWCTRL_REG_7_0    0x04 //Reset and power control
#define RST_PWCTRL_REG_15_8   0x05
#define SDI_CTL_REG_7_0       0x08 //SDI data input control
#define SDO_CTL_REG_7_0       0x0C //SDO-x data input control
#define SDO_CTL_REG_15_8      0x0D
#define DATAOUT_CTL_REG_7_0   0x10 //output data control
#define DATAOUT_CTL_REG_15_8  0x11
#define RANGE_SEL_REG_7_0     0x14 //input range selection control
#define ALARM_REG_7_0         0x20 //ALARM output register
#define ALARM_REG_15_8        0x21
#define ALARM_H_TH_REG_7_0    0x24 //ALARM high threshold and hystersis register
#define ALARM_H_TH_REG_15_8   0x25
#define ALARM_H_TH_REG_31_24  0x27
#define ALARM_L_TH_REG_7_0    0x28 //ALARM low threshold register
#define ALARM_L_TH_REG_15_8   0x29

//command op codes - LSB of all op codes (except NOP) is MSB of 9 bit address
#define NOP           0x00 //send to read current 16 bit value
#define CLEAR_HWORD   0xC0 //any bits marked 1 in data will be set to 0 in register at those positions
#define READ_HWORD    0xC8 //read 16 bits from register
#define READ          0x48 //read 8 bits from register
#define WRITE         0xD0 //write 16 bits to register
#define WRITE_MSBYTE  0xD2 //write 8 bits to register (MS Byte of data)
#define WRITE_LSBYTE  0xD4 //write 8 bits to register (LS Byte of data)
#define SET_HWORD     0xD8 //any bits marked 1 in data will be set to 1 in register at those positions

// END **************** ADS8689IPWR 16bit ADC ****************


/****************************************************************************
 * Public Data
 ****************************************************************************/

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
bool adc_init(uint8_t slot);
uint16_t adc_read(uint8_t slot);
uint16_t adc_write(uint8_t slot, uint8_t op, uint8_t address, uint16_t data);

#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_DRIVERS_ADC_ADS8689_ADC_H_ */
