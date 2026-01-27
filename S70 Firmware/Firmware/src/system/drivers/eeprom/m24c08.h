/**
 * @file
 *
 * @brief USB slave device handling functions
 *
 * USB data received flow diagram
 * ======================
 *
 *
 *
 *
 *
 <table>
<caption id="multi_row">Complex table</caption>
<tr><th>Column 1                      <th>Column 2        <th>Column 3
<tr><td rowspan="2">cell row=1+2,col=1<td>cell row=1,col=2<td>cell row=1,col=3
<tr><td rowspan="2">cell row=2+3,col=2                    <td>cell row=2,col=3
<tr><td>cell row=3,col=1                                  <td rowspan="2">cell row=3+4,col=3
<tr><td colspan="2">cell row=4,col=1+2
<tr><td>cell row=5,col=1              <td colspan="2">cell row=5,col=2+3
<tr><td colspan="2" rowspan="2">cell row=6+7,col=1+2      <td>cell row=6,col=3
<tr>                                                      <td>cell row=7,col=3
<tr><td>cell row=8,col=1              <td>cell row=8,col=2\n
  <table>
    <tr><td>Inner cell row=1,col=1<td>Inner cell row=1,col=2
    <tr><td>Inner cell row=2,col=1<td>Inner cell row=2,col=2
  </table>
  <td>cell row=8,col=3
  <ul>
    <li>Item 1
    <li>Item 2
  </ul>
</table>
 **/

#ifndef SRC_INCLUDE_M24C08_H_
#define SRC_INCLUDE_M24C08_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <asf.h>

/****************************************************************************
 * Defines
 ****************************************************************************/
/**
 * @brief The M24C08 has 8 Kbit (1 Kbyte) of EEPROM with Page size: 16 byte
 */

#define EEPROM_PAGESIZE_M24C08    	16	// bytes

/* The M24C08W contains 4 blocks (128byte each) with the addresses below: E2 = 0
   EEPROM Addresses defines */
#define EEPROM_ADDRESS_M24C08_BLOCK0      0xA0
#define EEPROM_ADDRESS_M24C08_BLOCK1      0xA2
#define EEPROM_ADDRESS_M24C08_BLOCK2      0xA4
#define EEPROM_ADDRESS_M24C08_BLOCK3      0xA6

#define EEPROM_OK                       0
#define EEPROM_FAIL                     1
#define EEPROM_TIMEOUT                  2

#define I2C_8CH_SWITCH_ADDRESS		0x70<<1	// PCA9548A
#define I2C_BOARD_EEPROM_CH			7




/****************************************************************************
 * Public Data
 ****************************************************************************/

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
uint8_t m24c08_read_page(uint8_t channel, uint8_t block, uint8_t address,
		uint8_t NumByteToRead, uint8_t *pBuffer);
uint8_t m24c08_write_page(uint8_t channel, uint8_t block, uint8_t address,
		uint8_t NumByteToWrite, uint8_t *pBuffer);

#ifdef __cplusplus
}
#endif

#endif /* SRC_INCLUDE_M24C08_H_ */
