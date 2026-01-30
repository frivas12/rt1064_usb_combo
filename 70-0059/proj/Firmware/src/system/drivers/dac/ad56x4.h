/**
 * @file ad56x4.h
 *
 * @brief Functions for ???
 *
 */

#ifndef SRC_SYSTEM_CARDS_STEPPER_AD56X4_H_
#define SRC_SYSTEM_CARDS_STEPPER_AD56X4_H_

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * Defines
 ****************************************************************************/
#define AD56X4_SPI_MODE			SPI_MODE_2
#define SPI_AD56X4_NO_READ 		AD56X4_SPI_MODE, SPI_NO_READ, CS_NO_TOGGLE, SLOT_CS(slot)
#define SPI_AD56X4_READ 		AD56X4_SPI_MODE, SPI_READ, CS_NO_TOGGLE, SLOT_CS(slot)

#define AD56X4_MAX_VAL	4095	// 5V

#define AD56X4_COMMAND_WRITE_INPUT_REGISTER            0b00000000
#define AD56X4_COMMAND_UPDATE_DAC_REGISTER             0b00001000
#define AD56X4_COMMAND_WRITE_INPUT_REGISTER_UPDATE_ALL 0b00010000
#define AD56X4_COMMAND_WRITE_UPDATE_CHANNEL            0b00011000
#define AD56X4_COMMAND_POWER_UPDOWN                    0b00100000
#define AD56X4_COMMAND_RESET                           0b00101000
#define AD56X4_COMMAND_SET_LDAC                        0b00110000
#define AD56X4_COMMAND_REFERENCE_ONOFF                 0b00111000

#define AD56X4_CHANNEL_A                               0b00000000
#define AD56X4_CHANNEL_B                               0b00000001
#define AD56X4_CHANNEL_C                               0b00000010
#define AD56X4_CHANNEL_D                               0b00000011
#define AD56X4_CHANNEL_ALL                             0b00000111

#define AD56X4_SETMODE_INPUT                           AD56X4_COMMAND_WRITE_INPUT_REGISTER
#define AD56X4_SETMODE_INPUT_DAC                       AD56X4_COMMAND_WRITE_UPDATE_CHANNEL
#define AD56X4_SETMODE_INPUT_DAC_ALL                   AD56X4_COMMAND_WRITE_INPUT_REGISTER_UPDATE_ALL

#define AD56X4_POWERMODE_NORMAL                        0b00000000
#define AD56X4_POWERMODE_POWERDOWN_1K                  0b00010000
#define AD56X4_POWERMODE_POWERDOWN_100K                0b00100000
#define AD56X4_POWERMODE_TRISTATE                      0b00110000

/****************************************************************************
 * Public Data
 ****************************************************************************/

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
void AD56X4UpdateChannel(uint8_t slot, uint8_t channel);
void AD56X4PowerUpDown(uint8_t slot, uint8_t powerMode, uint8_t channelMask);
void AD56X4Reset(uint8_t slot, bool fullReset);
void AD56X4SetInputMode(uint8_t slot, uint8_t channelMask);
void AD56X4SetChannel(uint8_t slot, uint8_t setMode, uint8_t channel, uint16_t value);
void AD56X4SetOutput(uint8_t slot, uint8_t channel, uint16_t value);
void InitAd5624Dac(uint8_t slot);

#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_CARDS_STEPPER_AD56X4_H_ */
