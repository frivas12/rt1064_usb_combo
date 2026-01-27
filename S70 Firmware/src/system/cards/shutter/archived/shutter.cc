/**
 * @file shutter.c
 *
 * @brief Functions for Shutter card.  This card can handle 4 shutter independently with different
 * voltages up to 24V.  PWM runs at 20kHz.
 *
 * Shutter we use:
 *     Name                Type                        Voltage        Pulse Width     Hz        Ohm
 * UNIBLITZ DSS20         Pulse                        10.7V            35ms        5        7.5
 * Thorlabs SHB025(T)    Pulse/hold                    ??7V/5V            20ms/hold    50        8.2
 * Thorlabs SHB05(T)    Pulse/hold                    ??7V/5V            20ms/hold    25        8.2
 * Thorlabs SHB1(T)        Pulse/hold                    7V/3.1V            20ms/hold    15        8.5
 * Thorlabs SH1            Pulse/hold/no reverse        24V/10.7V        10ms/hold    25        31.84
 * Thorlabs SH05        Pulse/hold                    24V/10.7V        10ms/hold    25        28
 */

#include <asf.h>
#include "shutter.h"
#include "hid-in-message.hh"
#include "itc-service.hh"
#include "usr_interrupt.h"
#include "Debugging.h"
#include "sys_task.h"
#include "usb_slave.h"
#include "delay.h"
#include "usb_device.h"
#include "Debugging.h"
#include "apt.h"
#include "hid.h"
#include <cpld.h>
#include "25lc1024.h"
#include <string.h>
#include <apt_parse.h>
#include <shutter_log.h>
#include <helper.h>
#include <log.h>
#include <pins.h>

#include <save_constructor.hh>
#include <lut_manager.hh>
#include <lock_guard.hh>
#include <save_util.hh>


using namespace service::itc;

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/
static void trig_4_handler(uint8_t slot, void *);
static void setup_io(Shutter_info *info);
static void service_external_trigger(Shutter_info *info,
        USB_Slave_Message *slave_message);
static void poll_interlock(Shutter_info *info);
static void read_eeprom_data(Shutter_info *info, shutter_4_channels chan);
static void write_eeprom_data(Shutter_info *info, shutter_4_channels chan);
static void setup_external_trigger(Shutter_info *info);
static void setup_mcp23s09(Shutter_info *info);
static void parse(Shutter_info *info, USB_Slave_Message *slave_message);
static bool service_cdc(Shutter_info *info,
        USB_Slave_Message *slave_message,
        service::itc::pipeline_cdc_t::unique_queue& queue);
static void service_joystick_in(Shutter_info *info,
                                pipeline_hid_in_t::unique_queue& queue);
static void service_joystick_out(Shutter_info *info);
static void set_duty_cycle(slot_nums slot, uint8_t chan, uint16_t duty);
static void set_direction(slot_nums slot, uint8_t chan, bool dir);
static void shutter_action(Shutter_info *info, shutter_4_channels chan);
static void shutter_toggle(Shutter_info *info, shutter_4_channels chan);
static void shutter_trig_action(Shutter_info *info, shutter_4_channels chan);
static void service_shutter(Shutter_info *info, shutter_4_channels chan,
        USB_Slave_Message *slave_message);
// static void set_default_params_shutter(Shutter_info *info,
//         shutter_4_channels chan);
static void enable_drive(Shutter_info *info, shutter_4_channels chan);
static void task_shutter(Shutter_info *info);

/****************************************************************************
 * Interrupt Handler
 ****************************************************************************/
/**
 * Interrupt handler for the interrupt coming in from SMIO.
 * @param slot
 */
static void trig_4_handler(uint8_t slot, void * _unused)
{
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    /* Wake up the servo task to toggle the shutter in the task*/
    if (xSlotHandle[slot] != NULL)
    {
        slots[slot].interrupt_flag_smio = 1;
        vTaskNotifyGiveFromISR(xSlotHandle[slot], &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/
/*This io is used for the safety interlock input, mainly from the trinoc
 * Open shutter for camera port = 5V
 * Closed shutter for eye port = 0V */

static void setup_io(Shutter_info *info)
{
    switch ((uint8_t) info->slot)
    {
    case SLOT_1:
        set_pin_as_input(PIN_ADC_1);
        break;

    case SLOT_2:
        set_pin_as_input(PIN_ADC_2);
        break;

    case SLOT_3:
        set_pin_as_input(PIN_ADC_3);
        break;

    case SLOT_4:
        set_pin_as_input(PIN_ADC_4);
        break;

    case SLOT_5:
        set_pin_as_input(PIN_ADC_5);
        break;

    case SLOT_6:
        set_pin_as_input(PIN_ADC_6);
        break;

    case SLOT_7:
        set_pin_as_input(PIN_ADC_7);
        break;
    }
}

// MARK:  SPI Mutex Required
static void poll_interlock(Shutter_info *info)
{
    /* read the busy status of the L6470 or L6480 low means device is busy*/
    switch ((uint8_t) info->slot)
    {
    case SLOT_1:
        info->interlock_state = !get_pin_level(PIN_ADC_1);
        break;
    case SLOT_2:
        info->interlock_state = !get_pin_level(PIN_ADC_2);
        break;
    case SLOT_3:
        info->interlock_state = !get_pin_level(PIN_ADC_3);
        break;
    case SLOT_4:
        info->interlock_state = !get_pin_level(PIN_ADC_4);
        break;
    case SLOT_5:
        info->interlock_state = !get_pin_level(PIN_ADC_5);
        break;
    case SLOT_6:
        info->interlock_state = !get_pin_level(PIN_ADC_6);
        break;
    case SLOT_7:
        info->interlock_state = !get_pin_level(PIN_ADC_7);
        break;
    }

    // rev 06 has the interlock inverted from the previous version of the board
    if(slots[info->slot].card_type == Shutter_4_type_REV6)
        info->interlock_state = !info->interlock_state;

    if(!info->interlock_state)
    {
        // this is just to update the status of the blade type shutter which have hold voltage to keep open
        for (int chan = 0; chan < SHUTTER_MAX_CHAN; chan++)
        {
            if (info->shutter[chan].position == SHUTTER_4_OPENED)
            {
                if ((uint8_t) info->shutter[chan].mode >= SHUTTER_4_PULSE_HOLD)
                {
                    info->shutter[chan].position = SHUTTER_4_CLOSED;
                    shutter_action(info, static_cast<shutter_4_channels>(chan));
                }
            }
        }
    }
}

static void service_external_trigger(Shutter_info *info,
        USB_Slave_Message *slave_message)
{
//    uint8_t chan;
    slots[info->slot].interrupt_flag_smio = 0;

    if (Tst_bits(info->mcp_info.io_interrupt_flags, 1 << 0))
    {
        shutter_trig_action(info, SHUTTER_4_CHAN_1);
//        chan = 0;
    }
    if (Tst_bits(info->mcp_info.io_interrupt_flags, 1 << 1))
    {
        shutter_trig_action(info, SHUTTER_4_CHAN_2);
//        chan = 1;
    }
    if (Tst_bits(info->mcp_info.io_interrupt_flags, 1 << 2))
    {
        shutter_trig_action(info, SHUTTER_4_CHAN_3);
//        chan = 2;
    }
    if (Tst_bits(info->mcp_info.io_interrupt_flags, 1 << 3))
    {
        shutter_trig_action(info, SHUTTER_4_CHAN_4);
//        chan = 3;
    }

//    setup_and_send_log(slave_message, info->slot, LOG_REPEAT,
//            SHUTTER_LOG_TYPE, SHUTTER_EXTERNAL_TRIGGER, chan, 0, 0);
}

// MARK:  SPI Mutex Required
static void read_eeprom_data(Shutter_info *info, shutter_4_channels chan)
{
    /* Calculate the address*/
    const uint32_t BASE_ADDRESS = SLOT_EEPROM_ADDRESS(info->slot, chan);

    /* Read all the params data into params structure*/
    info->shutter[chan].save.config.read_eeprom(BASE_ADDRESS);
}

// MARK:  SPI Mutex Required
static void write_eeprom_data(Shutter_info *info, shutter_4_channels chan)
{
    /* Calculate the address*/
    const uint32_t BASE_ADDRESS = SLOT_EEPROM_ADDRESS(info->slot, chan);

    /*Save the new default values to EEPROM*/
    save_util::update_save(info->slot, BASE_ADDRESS, info->shutter[chan].save.config);
}

/**
 * @brief There are 3 mode for the external trigger: Disabled, Enabled, and Reversed.
 * @param info
 */
static void setup_external_trigger(Shutter_info *info)
{
    /*Set GPOI interrupt on change register depending on which shutters have external trigger enabled*/
    info->mcp_info.io_interrupt_on_change = 0; //128;// 0x0F;

    for (int chan = 0; chan < SHUTTER_MAX_CHAN; chan++)
    {
        if ((info->shutter[chan].save.config.saved_params.params.external_trigger_mode
                == SHUTTER_4_TRIG_ENABLED)
                || (info->shutter[chan].save.config.saved_params.params.external_trigger_mode
                        == SHUTTER_4_TRIG_REVERSED))
        {
            Set_bits(info->mcp_info.io_interrupt_on_change, 1 << chan);
        }
    }

    /*If any external triggers are enabled, enable the SMIO interrupt for this slot*/
    if (info->mcp_info.io_interrupt_on_change > 0)
    {
        /*Enable the interrupt*/
        slots[info->slot].p_interrupt_smio_handler = &trig_4_handler;
        setup_slot_interrupt_SMIO(info->slot);
    }
}

// MARK:  SPI Mutex Required
static void setup_mcp23s09(Shutter_info *info)
{
    /*Pins 0-3 need to always be set as inputs, 4 - 7 are outputs */

    /*Set GPOI direction 0 for output, 1 for input*/
    info->mcp_info.io_dir = GPIO_ALL_INPUTS; /*Start with all set to input*/
//    Clr_bits(info->mcp_info.io_dir, EN_DRIVE_1); /*set as output*/
//    Clr_bits(info->mcp_info.io_dir, EN_DRIVE_2); /*set as output*/
//    Clr_bits(info->mcp_info.io_dir, EN_DRIVE_3); /*set as output*/
//    Clr_bits(info->mcp_info.io_dir, EN_DRIVE_4); /*set as output*/
    write_mcp_io_dir(info->slot, info->mcp_info.io_dir);

    /*Set GPOI polarity IPOL*/
    info->mcp_info.io_polarity = 0; /*Same as input for all io's*/
    write_mcp_input_polarity(info->slot, info->mcp_info.io_polarity);

    /*Set GPOI interrupt on change GPINTEN*/
    info->mcp_info.io_interrupt_on_change = 0xff;
    write_mcp_interrupt_on_change(info->slot,
            info->mcp_info.io_interrupt_on_change);

    /*Set GPOI compare register DEFVAL*/
    info->mcp_info.io_compare = 0xff; /*compare settings*/
    write_mcp_compare_reg(info->slot, info->mcp_info.io_compare);

    /*Set GPOI interrupt compare register INTCON*/
    info->mcp_info.io_int_comp_reg = 0x00; /*interrupt compare settings needed*/
    write_mcp_interrupt_control_reg(info->slot, info->mcp_info.io_int_comp_reg);

    /*Set Configuration register IOCON*/
    info->mcp_info.io_config_reg = 6; /*Configuration register*/
    write_mcp_config_reg(info->slot, info->mcp_info.io_config_reg);

    /*Set Pull-up register GPPU*/
    info->mcp_info.io_pullup = 0; /*Disable pull-ups because we have external pull-ups*/
    write_mcp_pullup_reg(info->slot, info->mcp_info.io_pullup);

    info->mcp_info.io_pin_state = 0;

}

// MARK:  SPI Mutex Required
static void parse(Shutter_info *info, USB_Slave_Message *slave_message)
{
    uint8_t response_buffer[100] =
    { 0 };
    uint8_t length = 0;
    bool need_to_reply = false;
    uint8_t chan = 0;
    shutter_log_ids log_id = SHUTTER_NO_LOG;

    switch (slave_message->ucMessageID)
    {
    case MGMSG_MCM_ERASE_DEVICE_CONFIGURATION: /* 0x4020 */
    // Invalidate the configuration on the slot card identified
    if (slave_message->ExtendedData_len >= 2 && info->slot == slave_message->extended_data_buf[0])
    {
        const uint32_t BASE_ADDRESS = SLOT_EEPROM_ADDRESS(info->slot, 0);

        info->shutter[slave_message->param1].save.config.invalidate(BASE_ADDRESS);

        device_detect_reset(&slots[info->slot].device);
    }
    break;

    case MGMSG_MCM_SET_SHUTTERPARAMS:
        chan = slave_message->extended_data_buf[0];
        memcpy(&info->shutter[chan].save.config.saved_params.params,
                &slave_message->extended_data_buf[2], sizeof(Shutter_4_params));

        /*Save the data to EEPROM*/
        write_eeprom_data(info, static_cast<shutter_4_channels>(chan));

        log_id = SHUTTER_SET_SHUTTERPARAMS;
        break;

    case MGMSG_MCM_REQ_SHUTTERPARAMS:
        /* Header */
        response_buffer[0] = MGMSG_MCM_GET_SHUTTERPARAMS;
        response_buffer[1] = MGMSG_MCM_GET_SHUTTERPARAMS >> 8;
        response_buffer[2] = sizeof(Shutter_4_params) + 2;
        response_buffer[3] = 0x00;
        response_buffer[4] = HOST_ID | 0x80; /* destination & has extended data*/
        response_buffer[5] = SLOT_1_ID + static_cast<uint8_t>(info->slot); /* source*/

        /*Chan Ident*/
        response_buffer[6] = slave_message->destination - SLOT_1_ID;    // slot
        response_buffer[7] = slave_message->param1;    // channel

        memcpy(&response_buffer[8],
                &info->shutter[slave_message->param1].save.config.saved_params.params,
                sizeof(Shutter_4_params));
        need_to_reply = true;
        length = 6 + response_buffer[2];
        break;

    case MGMSG_MCM_REQ_INTERLOCK_STATE: /* 0x4067*/
        /* Header */
        response_buffer[0] = MGMSG_MCM_GET_INTERLOCK_STATE;
        response_buffer[1] = MGMSG_MCM_GET_INTERLOCK_STATE >> 8;
        response_buffer[2] = (uint8_t) info->interlock_state;
        response_buffer[3] = 0;
        response_buffer[4] = HOST_ID;
        response_buffer[5] = SLOT_1_ID + static_cast<uint8_t>(info->slot); /* source*/

        need_to_reply = true;
        length = 6;
        break;

    case MGMSG_MOT_SET_SOL_STATE: /* 0x04CB*/
        chan = slave_message->param1;
        info->shutter[chan].position = static_cast<shutter_4_positions>(slave_message->param2);
        if (info->shutter[chan].position == SHUTTER_4_OPENED)
        {
            info->shutter[chan].mode = SHUTTER_4_OPEN_STATE;
        }
        else
        {
            info->shutter[chan].mode = SHUTTER_4_CLOSE_STATE;
        }
        log_id = SHUTTER_SET_SOL_STATE;
        break;

    case MGMSG_MOT_REQ_SOL_STATE: /* 0x04CC*/
        // param1 is the channel number
        // param2 is the position open/closed
        /* Header */
        response_buffer[0] = MGMSG_MOT_GET_SOL_STATE;
        response_buffer[1] = MGMSG_MOT_GET_SOL_STATE >> 8;
        response_buffer[2] = slave_message->param1;
        response_buffer[3] = info->shutter[slave_message->param1].position;
        response_buffer[4] = HOST_ID;
        response_buffer[5] = SLOT_1_ID + static_cast<uint8_t>(info->slot); /* source*/

        need_to_reply = true;
        length = 6;
        break;

    default:
        if (!slot_parse_generic_apt(
                info->slot,
                slave_message,
                (uint8_t*)&response_buffer,
                &length,
                &need_to_reply
        )){
            usb_slave_command_not_supported(slave_message);
            log_id = SHUTTER_UNSUPPORTED_COMMAND;
        }
        break;
    }

    if (log_id != SHUTTER_NO_LOG)
        setup_and_send_log(slave_message, info->slot, LOG_REPEAT, SHUTTER_LOG_TYPE,
            log_id, chan, 0, 0);

    if (need_to_reply)
    {
        slave_message->write((uint8_t*) response_buffer, length);
    }
}

// MARK:  SPI Mutex Required
static bool service_cdc(Shutter_info *info,
        USB_Slave_Message *slave_message,
        service::itc::pipeline_cdc_t::unique_queue& queue) {
    std::size_t counter = 0;
    while (queue.try_pop(*slave_message)) {
        ++counter;
        parse(info, slave_message);
    }
    return counter != 0;
}

// MARK:  SPI Mutex Required
static void service_joystick_in(Shutter_info *info,
                                pipeline_hid_in_t::unique_queue& queue) {
    uint8_t chan = 0;

    message::hid_in_message msg;
    while (queue.try_pop(msg, 0)) {
        switch (msg.usage_page)
        {
        case HID_USAGE_PAGE_BUTTON:
            switch ((uint8_t) msg.mode)
            {
            case CTL_BTN_POS_TOGGLE:
                if (Tst_bits(msg.destination_channel_bitset, 1 << 0))
                {
                    shutter_toggle(info, SHUTTER_4_CHAN_1);
                    chan = 0;
                }
                if (Tst_bits(msg.destination_channel_bitset, 1 << 1))
                {
                    shutter_toggle(info, SHUTTER_4_CHAN_2);
                    chan = 1;
                }
                if (Tst_bits(msg.destination_channel_bitset, 1 << 2))
                {
                    shutter_toggle(info, SHUTTER_4_CHAN_3);
                    chan = 2;
                }
                if (Tst_bits(msg.destination_channel_bitset, 1 << 3))
                {
                    shutter_toggle(info, SHUTTER_4_CHAN_4);
                    chan = 3;
                }
                break;
            }
        }
    }
}

static void service_joystick_out(Shutter_info *info)
{
    /*Send position data to USB data out queue*/
    auto msg = message::hid_out_message::get_default(info->slot);

    msg.on_stored_position_bitset = 0;
// TODO need to fix after adding USB out positions
    /*Set position of each shutter channel*/
    for (int itr = 0; itr < SHUTTER_4_CHAN_MAX; itr++)
    {
        const shutter_4_channels chan = static_cast<shutter_4_channels>(itr);
        if (info->shutter[chan].position == SHUTTER_4_OPENED)
            Set_bits(msg.on_stored_position_bitset, (1 << chan));
    }

    pipeline_hid_out().broadcast(msg);
}

/**
 * @brief Set the shutter voltage by setting the PWM counts high.  PWM frequency
 * is 20kHz.  This sets the voltage for the shutter 0 - 24V.
 * @param slot
 * @param chan  : There are 4 shutters per card
 * @param duty : (0 - 1900) 0 = Stop, 1900 = ,max voltage 24V
 */
// MARK:  SPI Mutex Required
static void set_duty_cycle(slot_nums slot, uint8_t chan, uint16_t duty)
{
    set_reg(C_SET_SUTTER_PWM_DUTY, slot, 0, duty + (chan << 16));
}

// MARK:  SPI Mutex Required
static void set_direction(slot_nums slot, uint8_t chan, bool dir)
{
    set_reg(C_SET_SUTTER_PHASE_DUTY, slot, 0, dir + (chan << 1));
}

// MARK:  SPI Mutex Required
static void shutter_action(Shutter_info *info, shutter_4_channels chan)
{
    if (info->shutter[chan].save.config.saved_params.params.mode
            == SHUTTER_4_PULSE_NO_REVERSE)
    {

        /*This is for the SH1 shutter.  This shutter 0V is closed 10V is opened*/
        if (info->shutter[chan].position == SHUTTER_4_OPENED)
        {
            set_direction(info->slot, chan, info->shutter[chan].position);
            set_duty_cycle(info->slot, chan,
                    info->shutter[chan].save.config.saved_params.params.duty_cycle_pulse);
            info->shutter[chan].on_time_counter = 0;
            info->shutter[chan].mode = SHUTTER_4_HOLD_PULSE_STATE;
        }
        else /*Closed*/
        {
            set_direction(info->slot, chan, info->shutter[chan].position);
            set_duty_cycle(info->slot, chan, 0);
            info->shutter[chan].mode = SHUTTER_4_STOP_STATE;
        }
    }
    else
    {
        set_direction(info->slot, chan, info->shutter[chan].position);
        set_duty_cycle(info->slot, chan,
                info->shutter[chan].save.config.saved_params.params.duty_cycle_pulse);

        //    servo_sleep(info->slot, SLEEP_DISABLED);
        info->shutter[chan].on_time_counter = 0;

        if (info->shutter[chan].save.config.saved_params.params.on_time == 1)
            info->shutter[chan].mode = SHUTTER_4_STOP_STATE;
        else
        {
            info->shutter[chan].mode = SHUTTER_4_HOLD_PULSE_STATE;
        }
    }
}

// MARK:  SPI Mutex Required
static void shutter_toggle(Shutter_info *info, shutter_4_channels chan)
{
    if (info->shutter[chan].position == SHUTTER_4_OPENED)
        info->shutter[chan].position = SHUTTER_4_CLOSED;
    else
        info->shutter[chan].position = SHUTTER_4_OPENED;

    shutter_action(info, chan);
}

static void shutter_trig_action(Shutter_info *info, shutter_4_channels chan)
{
#if 1
    bool external_trig_level = Tst_bits(info->mcp_info.io_pin_state, 1 << chan);

    // enable
    if (info->shutter[chan].save.config.saved_params.params.external_trigger_mode
            == SHUTTER_4_TRIG_ENABLED)
    {
        info->shutter[chan].position = static_cast<shutter_4_positions>(external_trig_level);
        if (info->shutter[chan].position == SHUTTER_4_OPENED)
        {
            info->shutter[chan].mode = SHUTTER_4_OPEN_STATE;
        }
        else
        {
            info->shutter[chan].mode = SHUTTER_4_CLOSE_STATE;
        }
    }
    // reversed
    else if (info->shutter[chan].save.config.saved_params.params.external_trigger_mode
            == SHUTTER_4_TRIG_REVERSED)
    {
        info->shutter[chan].position = static_cast<shutter_4_positions>(!external_trig_level);
        if (info->shutter[chan].position == SHUTTER_4_OPENED)
        {
            info->shutter[chan].mode = SHUTTER_4_OPEN_STATE;
        }
        else
        {
            info->shutter[chan].mode = SHUTTER_4_CLOSE_STATE;
        }
    }
    //toggle
    else if (info->shutter[chan].save.config.saved_params.params.external_trigger_mode
            == SHUTTER_4_TRIG_TOGGLE)
    {
        if (info->shutter[chan].position == SHUTTER_4_OPENED)
        {
            info->shutter[chan].mode = SHUTTER_4_CLOSE_STATE;
        }
        else
        {
            info->shutter[chan].mode = SHUTTER_4_OPEN_STATE;
        }
    }

#else

    bool external_trig_level = Tst_bits(info->mcp_info.io_pin_state, 1 << chan);

    if (info->shutter[chan].save.config.saved_params.params.external_trigger_mode
            == SHUTTER_4_TRIG_ENABLED)
    {
        if (external_trig_level)
            info->shutter[chan].position = SHUTTER_4_CLOSED;
        else
            info->shutter[chan].position = SHUTTER_4_OPENED;
    }
    else if (info->shutter[chan].save.config.saved_params.params.external_trigger_mode
            == SHUTTER_4_TRIG_REVERSED)
    {
        if (external_trig_level)
            info->shutter[chan].position = SHUTTER_4_OPENED;
        else
            info->shutter[chan].position = SHUTTER_4_CLOSED;
    }
    else if (info->shutter[chan].save.config.saved_params.params.external_trigger_mode
            == SHUTTER_4_TRIG_TOGGLE)
    {
        if (info->shutter[chan].position == SHUTTER_4_OPENED)
        {
            info->shutter[chan].position = SHUTTER_4_CLOSED;
        }
        else
        {
            info->shutter[chan].position = SHUTTER_4_OPENED;
        }
    }

    shutter_action(info, chan);
#endif
}

// MARK:  SPI Mutex Required
static void service_shutter(Shutter_info *info, shutter_4_channels chan,
        USB_Slave_Message *slave_message)
{
    shutter_log_ids log_id = SHUTTER_NO_LOG;

    // check if channel is enabled
    if (info->shutter[chan].save.config.saved_params.params.mode
            == SHUTTER_4_DISABLED)
    {
        return;
    }

    switch (info->shutter[chan].mode)
    {
    case SHUTTER_4_STOP_STATE:
        set_duty_cycle(info->slot, chan, 0);
        info->shutter[chan].mode = SHUTTER_4_IDLE_STATE;
        log_id = SHUTTER_STOP_LOG;
        break;

    case SHUTTER_4_IDLE_STATE:
        log_id = SHUTTER_IDLE_LOG;
        break;

    case SHUTTER_4_OPEN_STATE:
        /*The task runs at 10ms so when we toggle shutter set the direction
         * and pwm high here then go to wait state
         */
        info->shutter[chan].position = SHUTTER_4_OPENED;
        shutter_action(info, chan);
        log_id = SHUTTER_OPEN_LOG;
        break;

    case SHUTTER_4_CLOSE_STATE:
        /*The task runs at 10ms so when we toggle shutter set the direction
         * and pwm high here then go to wait state
         */
        info->shutter[chan].position = SHUTTER_4_CLOSED;
        shutter_action(info, chan);
        log_id = SHUTTER_CLOSE_LOG;
        break;

    case SHUTTER_4_HOLD_PULSE_STATE:
        // pulsed
        if (info->shutter[chan].save.config.saved_params.params.mode
                == SHUTTER_4_PULSED)
        {
            if (info->shutter[chan].on_time_counter
                    >= info->shutter[chan].save.config.saved_params.params.on_time
                            - 2)
            {
                info->shutter[chan].mode = SHUTTER_4_STOP_STATE;
            }
        }
        else /*Hold pulse voltage*/
        {
            if (info->shutter[chan].on_time_counter
                    >= info->shutter[chan].save.config.saved_params.params.on_time
                            - 2)
            {
                set_duty_cycle(info->slot, chan,
                        info->shutter[chan].save.config.saved_params.params.duty_cycle_hold);
                info->shutter[chan].mode = SHUTTER_4_HOLD_VOLTGAE_STATE;
            }
        }
        info->shutter[chan].on_time_counter++;
        log_id = SHUTTER_HOLD_PULSE_LOG;
        break;

    case SHUTTER_4_HOLD_VOLTGAE_STATE:
        /*Just sit here to hold the voltage until a new command comes in to change shutter
         * position*/
        log_id = SHUTTER_HOLD_VOLTGAE_LOG;
        break;
    }

    if (log_id != SHUTTER_NO_LOG)
        setup_and_send_log(slave_message, info->slot, LOG_NO_REPEAT, SHUTTER_LOG_TYPE,
                log_id, chan, 0, 0);
}

// static void set_default_params_shutter(Shutter_info *info,
//         shutter_4_channels chan)
// {
//     info->shutter[chan].save.config.saved_params.params.initial_state =
//             SHUTTER_4_CLOSED;
//     info->shutter[chan].save.config.saved_params.params.mode = SHUTTER_4_DISABLED;
//     info->shutter[chan].save.config.saved_params.params.external_trigger_mode =
//             SHUTTER_4_TRIG_DISABLED;
//     /* cycle time for shutters in 10's of ms*/
//     info->shutter[chan].save.config.saved_params.params.on_time = 1;
//     info->shutter[chan].save.config.saved_params.params.duty_cycle_pulse = 0;
//     info->shutter[chan].save.config.saved_params.params.duty_cycle_hold = 0;
// }

static void enable_drive(Shutter_info *info, shutter_4_channels chan)
{
    uint8_t chan_mask = EN_DRIVE_1 << chan;

    /*Low enable the output channel on the L2293QTR*/
    if (info->shutter[chan].save.config.saved_params.params.mode
            == SHUTTER_4_DISABLED)
    {
        /*Disable*/
        Set_bits(info->mcp_info.io_pin_state, chan_mask);
    }
    else
    {
        /*Enable*/
        Clr_bits(info->mcp_info.io_pin_state, chan_mask);
    }
    // took out the signal to disable to instead use the hardware interlock
//    write_mcp_port_reg(info->slot, info->mcp_info.io_pin_state);
}

/****************************************************************************
 * Task
 ****************************************************************************/
static void task_shutter (Shutter_info * p_info)
{
    // BEGIN    Task Initialization
    
    /*Wait here until power is good*/
    while (board.power_good == POWER_NOT_GOOD)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    auto cdc_queue = *service::itc::pipeline_cdc().create_queue(
        static_cast<asf_destination_ids>((int)p_info->slot + (int)SLOT_1_ID));

    /* Create the structure for holding the USB data received from the USB slave task*/
    USB_Slave_Message slave_message;

    auto in_queue = *pipeline_hid_in().create_queue(p_info->slot);

    // END      Task Initialization
    
    for (;;)
    {
        // Wait until the gate is opened (valid device is connected)
        xGatePass(slots[p_info->slot].device.connection_gate, portMAX_DELAY);
        
        // BEGIN    Device Initialization

        // Read saved data for this stage.
        {
            lock_guard lg(xSPI_Semaphore);
            for (uint8_t chan = 0; chan < SHUTTER_MAX_CHAN; chan++)
            {
                read_eeprom_data(p_info, static_cast<shutter_4_channels>(chan));
            }
        }

        // Load the configs if needed.
        bool save_data_valid = true;
        Shutter_4_Save_Union cfgs;
        bool cfgs_loaded;
        {
            lut_manager::device_lut_key_t key;
            xSemaphoreTake(slots[p_info->slot].xSlot_Mutex, portMAX_DELAY);
            key.running_slot_card = slots[p_info->slot].card_type;
            key.connected_device = slots[p_info->slot].device.info.signature;
            xSemaphoreGive(slots[p_info->slot].xSlot_Mutex);

            cfgs_loaded = lut_manager::load_data(LUT_ID::DEVICE_LUT, &key, &cfgs.cfg)
                == LUT_ERROR::OKAY;
        }

        for (int i = 0; i < SHUTTER_4_SUBDEVICES; ++i)
        {
            Shutter_4 * const p_dev = p_info->shutter + i;

            bool subdevice_valid = p_dev->save.config.valid();

            bool order_valid = cfgs.cfg[i].parameters.struct_id == static_cast<struct_id_t>(Struct_ID::SHUTTER_4_PARAMS);

            if (!subdevice_valid && order_valid && cfgs_loaded)
            {
                // Construct the save structure.
                subdevice_valid = save_constructor::construct(
                    cfgs.array + SHUTTER_4_CONFIGS_TOTAL*i,
                    SHUTTER_4_CONFIGS_TOTAL,
                    &p_dev->save.config.saved_params
                );
            }

            save_data_valid = save_data_valid && subdevice_valid && order_valid;
        }

        if (save_data_valid) {

        
            xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);
            setup_mcp23s09(p_info);

            for (uint8_t chan = 0; chan < SHUTTER_MAX_CHAN; chan++)
            {

                if(p_info->shutter[chan].save.config.saved_params.params.mode == SHUTTER_4_DISABLED)
                {
                    continue;
                }

                enable_drive(p_info, static_cast<shutter_4_channels>(chan));

                /*Set initial state*/
                if (p_info->shutter[chan].save.config.saved_params.params.initial_state
                        == SHUTTER_4_CLOSED)
                    p_info->shutter[chan].mode = SHUTTER_4_CLOSE_STATE;
                else
                    p_info->shutter[chan].mode = SHUTTER_4_OPEN_STATE;
            }

            setup_io(p_info);
            setup_external_trigger(p_info);

            /*Set the interrupt flags for all the external triggers so we can get the initial states set
            * for all the shutters based on the configuration*/
            
            /*Read the MCP23S09 GPIO to clear the interrupt and get the io states*/
            p_info->mcp_info.io_interrupt_flags = read_mcp_interrupt_flag_reg(p_info->slot);
            p_info->mcp_info.io_pin_state = read_mcp_port_reg(p_info->slot);
            xSemaphoreGive(xSPI_Semaphore);
        } else {
            // Close the gate so it has to wait again.
            // A reset of this slot's device-detect FSM will be needed to try again.
            // Use the reset function or disconnect-reconnect.
            xGateClose(slots[p_info->slot].device.connection_gate);
        }

        // END      Device Initialization
        
        // Run while the gate is open (device is connected)
        while (xGatePass(slots[p_info->slot].device.connection_gate, 0) == pdPASS)
        {
            // BEGIN    Operation Loop
            xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);

            poll_interlock(p_info);

            /*Read the MCP23S09 GPIO to clear the interrupt and get the io states*/
            p_info->mcp_info.io_interrupt_flags = read_mcp_interrupt_flag_reg(
                    p_info->slot);
            p_info->mcp_info.io_pin_state = read_mcp_port_reg(p_info->slot);

            // about 60us latency
            if (slots[p_info->slot].interrupt_flag_smio == 1)
            {
    //            pin_high(PIN_M1);
                service_external_trigger(p_info, &slave_message);
            }
            else
            {
                service_joystick_in(p_info, in_queue);
                service_joystick_out(p_info);
            }

            service_cdc(p_info, &slave_message, cdc_queue);

            for (int itr = 0; itr < SHUTTER_MAX_CHAN; itr++)
            {
                const shutter_4_channels chan = static_cast<shutter_4_channels>(itr);
                service_shutter(p_info, chan, &slave_message);
            }

            xSemaphoreGive(xSPI_Semaphore);

    //        pin_low(PIN_M1);

            ulTaskNotifyTake( pdTRUE, SHUTTER_UPDATE_INTERVAL);
            // END      Operation Loop
        }

        // BEGIN    Device Cleanup

        // Stop (and close) the shutters
        for (int itr = 0; itr < SHUTTER_MAX_CHAN; ++itr)
        {
            const shutter_4_channels chan = static_cast<shutter_4_channels>(itr);
            p_info->shutter[chan].mode = SHUTTER_4_STOP_STATE;
            xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);
            service_shutter(p_info, chan, &slave_message);
            xSemaphoreGive(xSPI_Semaphore);
        }

        // END      Device Cleanup
    }
}


/****************************************************************************
 * Public Functions
 ****************************************************************************/

extern "C" void shutter_init(uint8_t slot)
{
    Shutter_info *pxShutter_info;

    /* Allocate a IO structure for each task.*/
    pxShutter_info = new Shutter_info;

    char pcName[6] =
    { 'S', 'H', 'U', 'T', 'X' };
    pcName[4] = slot + 0x30;

    pxShutter_info->slot = static_cast<slot_nums>(slot);

    /* Create task for each Shutter card */
    if (xTaskCreate((TaskFunction_t) task_shutter, pcName,
    TASK_SHUTTER_STACK_SIZE, (Shutter_info*) pxShutter_info,
    TASK_SHUTTER_STACK_PRIORITY, &xSlotHandle[slot]) != pdPASS)
    {
        debug_print("Failed to create Shutter task for slot %d\r\n", slot + 1);

        /* free pxServo_info*/
        delete pxShutter_info;
    }
}
