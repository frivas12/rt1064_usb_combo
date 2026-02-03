/**
 * @file init.c
 *
 * @brief User board initialization template
 *
 *  *@dot
 digraph G {
 node [shape=doublecircle]; "usb host task"; "error task";
 node [shape=ellipse];
 "init clocks and irq" -> "enable caches" -> "disable TCM" -> "start
 peripherals"; "start peripherals" -> "reset CPLD" -> "setup SPI" -> "config USB
 Host"; "config USB Host" -> "initialize slots" -> "config error thread" ->
 "return"; "config USB Host" -> "usb host task"; "config error thread" -> "error
 task";
 }
 * @enddot
 */
#include <asf.h>
#include <cpld.h>
#include <pins.h>

#include "../drivers/supervisor/supervisor.h"
#include "25lc1024.h"
#include "Debugging.h"
#include "board.h"
#include "conf_clock.h"
#include "delay.h"
#include "efs.h"
#include "helper.h"
#include "lut_manager.h"
#include "pmc.h"
#include "rt1064_spi_bridge.h"
#include "service-inits.h"
#include "soft_i2c.h"
#include "usb_host.h"
#include "usb_slave.h"
#include "user_adc.h"
#include "user_spi.h"
#include "user_usart.h"
#include "usr_interrupt.h"

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/
static void flash_Init(void);
static void reset_firmware_load_count(void);
static void program_fuses(void);
static void use_mainclk_before_sysclk_init(void);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONF_BOARD_ENABLE_TCM_AT_INIT
#if defined(__GNUC__)
extern char _itcm_lma, _sitcm, _eitcm;
#endif

/** \brief  TCM memory enable
 * The function enables TCM memories
 */
static inline void tcm_enable(void) {
    __DSB();
    __ISB();

    SCB->ITCMCR =
        (SCB_ITCMCR_EN_Msk | SCB_ITCMCR_RMW_Msk | SCB_ITCMCR_RETEN_Msk);
    SCB->DTCMCR =
        (SCB_DTCMCR_EN_Msk | SCB_DTCMCR_RMW_Msk | SCB_DTCMCR_RETEN_Msk);

    __DSB();
    __ISB();
}
#else
/** \brief  TCM memory Disable

 The function enables TCM memories
 */
static inline void tcm_disable(void) {
    __DSB();
    __ISB();
    SCB->ITCMCR &= ~(uint32_t)(1UL);
    SCB->DTCMCR &= ~(uint32_t)SCB_DTCMCR_EN_Msk;
    __DSB();
    __ISB();
}
#endif

static void flash_Init(void) {
    uint32_t ul_rc;
    ul_rc = flash_init(FLASH_ACCESS_MODE_128, 6);
    if (ul_rc != FLASH_RC_OK) {
        debug_print("ERROR: flash_Init failed");
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
static void reset_firmware_load_count(void) {
    uint32_t StatusFlg;
    uint32_t BootCount[1];
    StatusFlg = flash_read_user_signature(BootCount, 1);
    BootCount[0] = 0;
    StatusFlg = flash_erase_user_signature();
    delay_ms(10);
    StatusFlg = flash_write_user_signature(BootCount, 1);
}
#pragma GCC diagnostic pop

static void program_fuses(void) {
#define BOOTLOADER_FIRST_SECTOR 0x00400000
#define BOOTLOADER_LAST_SECTOR 0x0041C000
#define BOOTLOADER_REGIONS 8
    uint32_t locked_regions;
    locked_regions =
        flash_is_locked(BOOTLOADER_FIRST_SECTOR, BOOTLOADER_LAST_SECTOR);

    if (locked_regions < BOOTLOADER_REGIONS)
        flash_lock(BOOTLOADER_FIRST_SECTOR, BOOTLOADER_LAST_SECTOR, 0, 0);
}

static void use_mainclk_before_sysclk_init(void) {
#if (CONFIG_SYSCLK_SOURCE == SYSCLK_SRC_PLLACK || \
     CONFIG_SYSCLK_SOURCE == SYSCLK_SRC_UPLLCK)
    if (PMC_MCKR_CSS(PMC->PMC_MCKR) != PMC_MCKR_CSS_MAIN_CLK) {
        // See https://forum.microchip.com/s/topic/a5C3l0000003kefEAA/t390550
        if (pmc_switch_mck_to_mainck(CONFIG_SYSCLK_PRES) != 0) {
            restart_board();
        }
    }
    pmc_disable_pllack();
    pmc_disable_upll_clock();
    delay_ms(2);
#endif
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * @brief Initialize the processor details, the peripherals, then board and slot
 * details
 *
 */

void system_init(void) {
    irq_initialize_vectors();
    cpu_irq_enable();
    use_mainclk_before_sysclk_init();
    sysclk_init();
    delay_ms(50);

    // Disable the watchdog */
    //	WDT->WDT_MR = WDT_MR_WDDIS | WDT_MR_WDDBGHLT;

#ifdef CONF_BOARD_ENABLE_CACHE_AT_INIT
    /* Enabling the Cache */
    SCB_EnableICache();
    SCB_EnableDCache();
#endif

#ifdef CONF_BOARD_ENABLE_TCM_AT_INIT
    /* TCM Configuration */
    EFC->EEFC_FCR =
        (EEFC_FCR_FKEY_PASSWD | EEFC_FCR_FCMD_CGPB | EEFC_FCR_FARG(8));
    EFC->EEFC_FCR =
        (EEFC_FCR_FKEY_PASSWD | EEFC_FCR_FCMD_SGPB | EEFC_FCR_FARG(7));
    tcm_enable();
#if defined(__GNUC__)
    volatile char* dst = &_sitcm;
    volatile char* src = &_itcm_lma;
    /* copy code_TCM from flash to ITCM */
    while (dst < &_eitcm) {
        *dst++ = *src++;
    }
#endif
#else
    /* TCM Configuration */
    EFC->EEFC_FCR =
        (EEFC_FCR_FKEY_PASSWD | EEFC_FCR_FCMD_CGPB | EEFC_FCR_FARG(8));
    EFC->EEFC_FCR =
        (EEFC_FCR_FKEY_PASSWD | EEFC_FCR_FCMD_CGPB | EEFC_FCR_FARG(7));

    tcm_disable();
#endif

    /* Initialize IOPORTs */
    ioport_init();

    // Following three functions for Bootloader
    flash_Init();
    reset_firmware_load_count();
    program_fuses();

    /* PB5 function selected not TDO/TRACESWO*/
    /* PB12 function select not ERASE*/
    Matrix* p_matrix = MATRIX;
    p_matrix->CCFG_SYSIO = 0x20401010;

#if SYSTEM_VIEW_ENABLE
    SEGGER_SYSVIEW_Conf();
    debug_print("System boot was successful\r\n");
    debug_print("-- Compiled: %s %s --\n\r", __DATE__, __TIME__);

#endif

#if DEBUG_PID_JSCOPE
    JS_RTT_Channel = 1;
    SEGGER_RTT_ConfigUpBuffer(JS_RTT_Channel, "JScope_I4i4",
                              &JS_RTT_UpBuffer[0], sizeof(JS_RTT_UpBuffer),
                              SEGGER_RTT_MODE_NO_BLOCK_SKIP);

#endif

    user_spi_init();
    init_interrupts();

    // TODO fix the case for hexapod needing to power up specially
    set_pin_as_output(PIN_POWER_EN, 1);

    cpld_init();

    soft_i2c_init();
    init_adc();
    uart0_init();

    // Initialize slot synchronization objects to prevent system hang (even when
    // programming cpld).
    init_slots_synchronization();

    // Initialize supervisor with the cpld status.
    // This is needed to periodically reset the WDT.
    supervisor_init(cpld_programmed);

    /*Only continue if the CPLD has been programmed, else the usb slave is
     * running to load the CPLD with software*/
    if (cpld_programmed == true) {
        efs_init();
        board_init();
        hid_mapping_service_init();
        itc_service_init();
        if (board_type < MCM_41_0117_RT1064) {
            usb_host_init();
        } else {
            rt1064_spi_bridge_init();
        }
    }
    delay_ms(100);
    usb_slave_init();

#if 1
    /* Enable the watchdog, need to reload in the supervisor task */
    uint32_t wdt_temp = WDT->WDT_MR;
    Clr_bits(wdt_temp, WDT_MR_WDDIS);
    Set_bits(wdt_temp, WDT_MR_WDDBGHLT); /* WDT stops in debug state. */
    WDT->WDT_MR = wdt_temp;
#endif
}
