# Frequency Sources
## Clocks
| Name | In Use | Frequency | Source |
| ---- | ------ | --------- | ------ |
| I2C | Yes | ≤10 MHz | Estimate |
| SPI0 | Yes | 12 MHz | SAMS70 Doc |
| SPI1 | Yes | 12 MHz | proj/Firmware/src/system/drivers/cpld/cpld.c |
| UART | Yes | ≤12 MHz | proj/Firmware/src/system/drivers/usart/user_usart.c |
| CPLD CLK Default | Yes | 38 MHz | proj/cpld/sources/mcm_top.v |
| CPLD CLK 96M | Yes | 96 MHz | proj/cpld/sources/mcm_top.v |
| CPLD CLK 1M | Yes | 1 MHz | proj/cpld/sources/mcm_top.v |
| CPLD CLK 100k | Yes | 100 kHz | proj/cpld/sources/mcm_top.v |
| CPLD CLK 20k | Yes | 20 kHz | proj/cpld/sources/mcm_top.v |
| XTAL Oscillator | Yes | 12 MHz | Altium - MC1000 Processor.SchDoc |
| PLL A | Yes | 300 MHz | proj/Firmware/src/config/conf_clock.h |
| UP PLL | Yes | 480 MHz | proj/Firmware/src/config/conf_clock.h |
| Processor Clock | Yes | 300 MHz | proj/Firmware/src/config/conf_clock.h |
| SysTick Clock | No | X | X |
| Host Clock | Yes | 150 MHz | proj/Firmware/src/config/conf_clock.h |

SPI0 are using the default values it is not set in firmware.
I2C driver is software, so the estimate is based on 30 clock cycles between positive clock edges.


## Pseudo-Clocks
| Name | In Use | Frequency | Source |
| ---- | ------ | --------- | ------ |
| One-Wire | Yes | 115 kHz | proj/Firmware/src/system/drivers/device_detect/device_detect.h |
| Power Supply | Yes | 1 MHz | LT8641 Docs |

## Unclocked Modulated Signals
| Name | In Use | Frequency | Source |
| ---- | ------ | --------- | ------ |
| Fan PWM | No | X | ? |
| Stepper Driver | Yes | 16 MHz | L6470 Docs |

Stepper driver is using the default oscillator.
