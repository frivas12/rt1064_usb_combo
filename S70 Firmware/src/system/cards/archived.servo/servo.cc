/**
 * @file servo.c
 *
 * @brief Functions for 41-0098 Servo Module
 *
 */

#include <asf.h>
#include "servo.h"
#include "hid-in-message.hh"
#include "itc-service.hh"
#include "usr_interrupt.h"
#include "sys_task.h"
#include "usb_slave.h"
#include "usb_device.h"
#include "Debugging.h"
#include <pins.h>
#include <cpld.h>
#include <usb_host.h>
#include "apt.h"
#include <string.h>
#include "25lc1024.h"
#include "hid.h"
#include <apt_parse.h>
#include <log.h>
#include <servo_log.h>

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
void trig_handler(slot_nums slot);
static void set_vars(Servo_info *info);
// static void set_default_params_servo(Servo_info *info);
static bool servo_store_not_default(const Servo_Store * const p_store);
static void read_eeprom_data(Servo_info *info);
static void read_eeprom_store(Servo_info *info);
static void write_eeprom_data(Servo_info *info);
static void write_eeprom_store(Servo_info *info);
static void set_duty_cycle(slot_nums slot, uint16_t speed);
static void set_direction(slot_nums slot, bool dir);
//static void afec_print_comp_result(void);
//static void config_afec(slot_nums slot);
static void servo_sleep(slot_nums slot, bool sleep_state);
static void parse(Servo_info *info, USB_Slave_Message *slave_message);
static void check_position_changed(Servo_info *info);
static void shutter_action(Servo_info *info);
static void service_servo(Servo_info *info, USB_Slave_Message *slave_message);
static void service_joystick(Servo_info *info, pipeline_hid_in_t::unique_queue& queue);
static void service_cdc(Servo_info *info, USB_Slave_Message *slave_message,
                        service::itc::pipeline_cdc_t::unique_queue& queue);
static void service_external_trigger(Servo_info *info,
        USB_Slave_Message *message);
static void setup_external_trigger(Servo_info *info);
static void service_store(Servo_info * p_info);
static void task_servo(Servo_info *info);

/****************************************************************************
 * Interrupt Handler
 ****************************************************************************/

/**
 * Interrupt handler for the interrupt coming in from CPLD.
 * @param slot
 */
void trig_handler(slot_nums slot)
{
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    /* Wake up the servo task to toggle the shutter in the task*/
    if (xSlotHandle[slot] != NULL)
    {
        slots[slot].interrupt_flag_cpld = 1;
        vTaskNotifyGiveFromISR(xSlotHandle[slot], &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static void set_vars(Servo_info *info)
{
    /*These are all the variables that are not saved in EEPROM and should be initialized*/
    info->ctrl.mode = SERVO_STOP;
    info->ctrl.goto_mode = SERVO_GOTO_IDLE;
}

constexpr servo_positions DEFAULT_POSITION = SERVO_NO_POSITION;
constexpr uint32_t DEFAULT_PWM = 470;
// static void set_default_params_servo(Servo_info *info)
// {
//     info->save.config.saved_params.params.lTransitTime = 2500;
//     info->save.store.position = DEFAULT_POSITION;
//     info->save.store.pwm = DEFAULT_PWM; /* 2.93V for mesoscope mirror*/
//     info->save.config.saved_params.shutter_params.on_time = 1; /* cycle time for shutters in 10's of ms*/
//     info->save.config.saved_params.shutter_params.initial_state = SHUTTER_CLOSED;
// //    info->save.config.saved_params.shutter_params.external_trigger_mode = DISABLED;
// }

static bool servo_store_not_default(const Servo_Store * const p_store)
{
    return p_store->position != DEFAULT_POSITION || p_store->pwm != DEFAULT_PWM;
}

/**
 * @brief Reads all the EEPROM stored variables including high frequency change variables
 * like position to their corresponding ram variables.
 * @param info
 */
// MARK:  SPI Mutex Required
static void read_eeprom_data(Servo_info *info)
{
    const uint32_t BASE_ADDRESS = SLOT_EEPROM_ADDRESS(info->slot, 0);

    info->save.config.read_eeprom(BASE_ADDRESS);
}

// MARK:  SPI Mutex Required
static void read_eeprom_store(Servo_info *info)
{
    const uint32_t BASE_ADDRESS = SLOT_EEPROM_ADDRESS(info->slot, 1);

    eeprom_25LC1024_read(
            BASE_ADDRESS,
            sizeof(info->save.store),
            (uint8_t*) &info->save.store
    );
}

// MARK:  SPI Mutex Required
static void write_eeprom_data(Servo_info *info)
{
    /* Calculate the address*/
    const uint32_t BASE_ADDRESS = SLOT_EEPROM_ADDRESS(info->slot, 0);

    save_util::update_save(info->slot, BASE_ADDRESS, info->save.config);
}

// MARK:  SPI Mutex Required
static void write_eeprom_store(Servo_info *p_info)
{
    // Page 1 offset, high frequency variables
    uint8_t length_params = sizeof(p_info->save.store);

    // Calculate the address
    /// \todo Could synchronize this.
    uint32_t params_address = SLOT_EEPROM_ADDRESS(p_info->slot, 1);

    // Save the new store info to EEPROM.
    eeprom_25LC1024_write(params_address, length_params,
        reinterpret_cast<uint8_t*>(&p_info->save.store));
}

/**
 * @brief Set the speed by setting the PWM counts high.  PWM frequency
 * is 37.109kHz.  This sets the voltage for the shutter as well 0 - 12V.
 * On the OTM (tweezers) the supply voltage is 10V and the hardware is on board
 * not a card slot.
 * @param slot
 * @param speed : (0 - 1024) 0 = Stop, 1024 = ,max speed
 */
// MARK:  SPI Mutex Required
static void set_duty_cycle(slot_nums slot, uint16_t speed)
{
    set_reg(C_SET_PWM_DUTY, slot, 0, speed);
}

// MARK:  SPI Mutex Required
static void set_direction(slot_nums slot, bool dir)
{
    set_reg(C_SET_SERVO_CARD_PHASE, slot, 0, dir);
}

/*Collision detection doesn't work because not enough current difference when the hard
 * stop is hit.*/
/*Keep in case we want to use later on another motor*/

#if 0
/** Reference voltage for AFEC in mv. */
#define VOLT_REF        (3300)
/** The maximal digital value */
#define MAX_DIGITAL     (4095UL)

/** Low threshold */
static uint16_t gs_us_low_threshold = 0;
/** High threshold */
static uint16_t gs_us_high_threshold = 0;

/** AFEC channel for potentiometer */
#define AFEC_CHANNEL_ADC1  AFEC_CHANNEL_8

/**
 * \brief Callback function for AFEC enter compasion window interrupt.
 */
static void afec_print_comp_result(void)
{
    uint16_t us_adc;

    /* Disable Compare Interrupt. */
    //afec_disable_interrupt(AFEC0, AFEC_INTERRUPT_COMP_ERROR);

    us_adc = afec_channel_get_value(AFEC0, AFEC_CHANNEL_ADC1);
//    em_stop = 1;
//    printf("-ISR-:Potentiometer voltage %d mv is in the comparison "
//            "window:%d -%d mv!\n\r",
//            (int)(us_adc * VOLT_REF / MAX_DIGITAL),
//            (int)(gs_us_low_threshold * VOLT_REF / MAX_DIGITAL),
//            (int)(gs_us_high_threshold * VOLT_REF / MAX_DIGITAL));
}

static void config_afec(slot_nums slot)
{
    struct afec_config afec_cfg;
    struct afec_ch_config afec_ch_cfg;

    gs_us_low_threshold = 0x0;
    gs_us_high_threshold = 2300;

    afec_enable(AFEC0);
    afec_get_config_defaults(&afec_cfg);

//    afec_cfg.stm = false;

    afec_init(AFEC0, &afec_cfg);
    afec_set_trigger(AFEC0, AFEC_TRIG_FREERUN);
    afec_ch_get_config_defaults(&afec_ch_cfg);

    /*
     * Because the internal AFEC offset is 0x200, it should cancel it and shift
     * down to 0.
     */
    afec_channel_set_analog_offset(AFEC0, AFEC_CHANNEL_ADC1, 0x200);

    afec_ch_cfg.gain = AFEC_GAINVALUE_0;

    afec_ch_set_config(AFEC0, AFEC_CHANNEL_ADC1, &afec_ch_cfg);
    afec_set_trigger(AFEC0, AFEC_TRIG_FREERUN);

    afec_set_comparison_mode(AFEC0, AFEC_CMP_MODE_3, AFEC_CHANNEL_ADC1, 0);
    afec_set_comparison_window(AFEC0, gs_us_low_threshold, gs_us_high_threshold);

    /* Enable channel for potentiometer. */
    afec_channel_enable(AFEC0, AFEC_CHANNEL_ADC1);

    afec_set_callback(AFEC0, AFEC_INTERRUPT_COMP_ERROR, afec_print_comp_result, 1);

}
#endif

/**
 * @brief Sleep mode enable/disables the drive so that the output are tristated when
 * in sleep mode.
 * @param slot
 * @param sleep_state : 0 = sleep; 1 = on
 */
static void servo_sleep(slot_nums slot, bool sleep_state)
{
    switch ((uint8_t) slot)
    {
    case SLOT_1:
        pin_state(PIN_SMIO_1, sleep_state);
        break;
    case SLOT_2:
        pin_state(PIN_SMIO_2, sleep_state);
        break;
    case SLOT_3:
        pin_state(PIN_SMIO_3, sleep_state);
        break;
    case SLOT_4:
        pin_state(PIN_SMIO_4, sleep_state);
        break;
    case SLOT_5:
        pin_state(PIN_SMIO_5, sleep_state);
        break;
    case SLOT_6:
        pin_state(PIN_SMIO_6, sleep_state);
        break;
    case SLOT_7:
        pin_state(PIN_SMIO_7, sleep_state);
        break;
    }
}

// MARK:  SPI Mutex Required
static void goto_ctrl(Servo_info *info)
{
    switch (info->ctrl.goto_mode)
    {
    case SERVO_GOTO_IDLE:
        break;

    case SERVO_SART_MOVE:
        /*If we are already at the position then get out*/
        if (info->ctrl.cmd_position == info->save.store.position)
        {
            info->ctrl.lTransitTime_counter =
                    info->save.config.saved_params.params.lTransitTime;
            info->ctrl.goto_mode = SERVO_WAIT_TIME;
        }
        else
        {
            if (info->ctrl.cmd_position == SERVO_POSITION_1)
                info->ctrl.cmd_dir = SHUTTER_CLOSED;
            else
                info->ctrl.cmd_dir = SHUTTER_OPENED;

            set_direction(info->slot, info->ctrl.cmd_dir);
            set_duty_cycle(info->slot, info->save.store.pwm);
            servo_sleep(info->slot, SLEEP_DISABLED);
            info->ctrl.lTransitTime_counter = 0;
            info->ctrl.goto_mode = SERVO_WAIT_TIME;
        }
        break;

    case SERVO_WAIT_TIME:
        if (info->ctrl.lTransitTime_counter
                >= info->save.config.saved_params.params.lTransitTime)
        {
            servo_sleep(info->slot, SLEEP_ON);
            info->ctrl.mode = SERVO_STOP;
            info->ctrl.goto_mode = SERVO_GOTO_IDLE;
            info->save.store.position = static_cast<servo_positions>(info->ctrl.cmd_position);
        }
        /*Increment the counter but we have to increment it by the tasl interval*/
        info->ctrl.lTransitTime_counter += SERVO_UPDATE_INTERVAL;
        break;
    }
}

/**
 * @brief Parse APT commands that are specific to stepper card
 *
 * Takes xUSB_Slave_TxSemaphore - sending data on the USB bus
 *
 * @param info : The servo info structure.
 * @param slave_message : This is the massage structure pass from the USB salve task
 */
// MARK:  SPI Mutex Required
static void parse(Servo_info *info, USB_Slave_Message *slave_message)
{
    uint8_t response_buffer[100] =
    { 0 };
    uint8_t length = 0;
    bool need_to_reply = false;
    servo_log_ids log_id = SERVO_NO_LOG;

    switch (slave_message->ucMessageID)
    {
    case MGMSG_MCM_ERASE_DEVICE_CONFIGURATION: /* 0x4020 */
    // Invalidate the configuration on the slot card identified
    if (slave_message->ExtendedData_len >= 2 && info->slot == slave_message->extended_data_buf[0])
    {
        const uint32_t BASE_ADDRESS = SLOT_EEPROM_ADDRESS(info->slot, 0);

        info->save.config.invalidate(BASE_ADDRESS);
        info->save.config.write_eeprom(BASE_ADDRESS);

        device_detect_reset(&slots[info->slot].device);
    }
    break;

    case MGMSG_MOT_MOVE_STOP: /* 0x0465*/
        info->ctrl.mode = SERVO_STOP;
        log_id = SERVO_MOVE_STOP;
        break;

    case MGMSG_MOT_MOVE_VELOCITY: /* 0x0457 */
        info->ctrl.cmd_dir = slave_message->param2;
        info->ctrl.mode = SERVO_VEL_MOVE;
        log_id = SERVO_MOVE_STOP;
        break;

    case MGMSG_MOT_SET_MFF_OPERPARAMS: /* 0x0510 */
        /*Copy lTransitTime data to structure*/
        memcpy(&info->save.config.saved_params.params.lTransitTime,
                &slave_message->extended_data_buf[2], 4);

        /*Copy PulseWidth1 data to structure*/
        memcpy(&info->save.store.pwm, &slave_message->extended_data_buf[14],
                4);

        /*Save the data to EEPROM*/
        write_eeprom_data(info);
        write_eeprom_store(info);

        log_id = SERVO_SET_MFF_OPERPARAMSP;
        break;

    case MGMSG_MOT_REQ_MFF_OPERPARAMS: /* 0x0511 */
        /* Header */
        response_buffer[0] = (uint8_t)MGMSG_MOT_GET_MFF_OPERPARAMS;
        response_buffer[1] = MGMSG_MOT_GET_MFF_OPERPARAMS >> 8;
        response_buffer[2] = 34;
        response_buffer[3] = 0x00;
        response_buffer[4] = HOST_ID | 0x80; /* destination & has extended data*/
        response_buffer[5] = SLOT_1_ID + static_cast<uint8_t>(info->slot); /* source*/

        /*Chan Ident*/
        response_buffer[6] = info->slot;
        response_buffer[7] = 0x00;

        /*lTransitTime*/
        memcpy(&response_buffer[8], &info->save.config.saved_params.params.lTransitTime, 4);

        /*lTransitTime*/
        memcpy(&response_buffer[20], &info->save.store.pwm, 4);

        need_to_reply = true;
        length = 40;
        break;

    case MGMSG_MOT_SET_BUTTONPARAMS: /* 0x04B6*/
        /* Use this to move to  position 1 or position 2*/
        info->ctrl.cmd_position = slave_message->extended_data_buf[4];
        info->ctrl.mode = SERVO_GOTO;
        info->ctrl.goto_mode = SERVO_SART_MOVE;
        log_id = SERVO_SET_BUTTONPARAMS;
        break;

    case MGMSG_MOT_REQ_BUTTONPARAMS: /* 0x04B7*/
        /* Header */
        response_buffer[0] = (uint8_t)MGMSG_MOT_GET_BUTTONPARAMS;
        response_buffer[1] = MGMSG_MOT_GET_BUTTONPARAMS >> 8;
        response_buffer[2] = 16;
        response_buffer[3] = 0x00;
        response_buffer[4] = HOST_ID | 0x80; /* destination & has extended data*/
        response_buffer[5] = SLOT_1_ID + static_cast<uint8_t>(info->slot); /* source*/

        /*Chan Ident*/
        response_buffer[6] = info->slot;
        response_buffer[7] = 0x00;

        /*Position1*/
        memcpy(&response_buffer[10], &info->save.store.position, 4);

        need_to_reply = true;
        length = 22;
        break;

        /*Shutter commands*/
    case MGMSG_MOT_SET_SOL_STATE: /* 0x04CB*/
        if (info->type == Shutter_type)
        {
            info->shutter.position = static_cast<shutter_positions>(slave_message->param2);
            if (info->shutter.position == SHUTTER_OPENED)
            {
                info->ctrl.mode = SHUTTER_OPEN;
            }
            else
            {
                info->ctrl.mode = SHUTTER_CLOSE;
            }
            log_id = SERVO_SET_SOL_STATE;
        }
        break;

    case MGMSG_MOT_REQ_SOL_STATE: /* 0x04CC*/
        if (info->type == Shutter_type)
        {
            // param1 is the slot number
            // param2 is the Mode
            /* Header */
            response_buffer[0] = (uint8_t)MGMSG_MOT_GET_SOL_STATE;
            response_buffer[1] = MGMSG_MOT_GET_SOL_STATE >> 8;
            response_buffer[2] = info->slot;
            response_buffer[3] = info->shutter.position;
            response_buffer[4] = HOST_ID; /* destination*/
            response_buffer[5] = SLOT_1_ID + static_cast<uint8_t>(info->slot); /* source*/

            need_to_reply = true;
            length = 6;
        }
        break;

    case MGMSG_MCM_SET_SHUTTERPARAMS:
        if (info->type == Shutter_type)
        {
            memcpy(&info->save.config.saved_params.shutter_params,
                    &slave_message->extended_data_buf[2],
                    sizeof(Shutter_params));

            /*Save the data to EEPROM*/
            write_eeprom_data(info);
            log_id = SERVO_SET_SHUTTERPARAMS;
        }
        break;

    case MGMSG_MCM_REQ_SHUTTERPARAMS:
        /* Header */
        response_buffer[0] = (uint8_t)MGMSG_MCM_GET_SHUTTERPARAMS;
        response_buffer[1] = MGMSG_MCM_GET_SHUTTERPARAMS >> 8;
        response_buffer[2] = 14;
        response_buffer[3] = 0x00;
        response_buffer[4] = HOST_ID | 0x80; /* destination & has extended data*/
        response_buffer[5] = SLOT_1_ID + static_cast<uint8_t>(info->slot); /* source*/

        /*Chan Ident*/
        response_buffer[6] = info->slot;
        response_buffer[7] = 0x00;

        memcpy(&response_buffer[8], &info->save.config.saved_params.shutter_params,
                sizeof(Shutter_params));
        need_to_reply = true;
        length = 20;
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
            log_id = SERVO_UNSUPPORTED_COMMAND;
        }
        break;
    }

    if (log_id != SERVO_NO_LOG)
        setup_and_send_log(slave_message, info->slot, LOG_REPEAT,
                SERVO_LOG_TYPE, log_id, 0, 0, 0);

    if (need_to_reply)
    {
        slave_message->write((uint8_t*) response_buffer, length);
    }
}

/**
 * @brief read the position saved in EEPROM to compare if the current position
 * is different.  If it is write the new value to EEPROM.
 * @param info
 */
// MARK:  SPI Mutex Required
static void check_position_changed(Servo_info *info)
{
    Servo_Store temp_save_params;

    // Page 0 offset, low frequency variables
    uint8_t length_params = sizeof(info->save.store);

    /* Page 0 Calculate the address*/
    uint32_t params_address = SLOT_EEPROM_ADDRESS(info->slot, 1);

    /* Page 0 read all the params data into params structure*/
    eeprom_25LC1024_read(params_address, length_params,
            (uint8_t*) &temp_save_params);

    if (temp_save_params.position != info->save.store.position)
    {
        write_eeprom_store(info);
    }
}

// MARK:  SPI Mutex Required
static void shutter_action(Servo_info *info)
{
    set_direction(info->slot, info->shutter.position);
    set_duty_cycle(info->slot,
            info->save.config.saved_params.shutter_params.duty_cycle_pulse);
    servo_sleep(info->slot, SLEEP_DISABLED);
    info->shutter.on_time_counter = 0;

    if (info->save.config.saved_params.shutter_params.on_time == 1)
        info->ctrl.mode = SERVO_STOP;
    else
    {
        info->ctrl.mode = SHUTTER_HOLD_PULSE;
    }
}

// MARK:  SPI Mutex Required
static void service_servo(Servo_info *info, USB_Slave_Message *slave_message)
{
    servo_log_ids log_id = SERVO_NO_LOG;

    switch (info->ctrl.mode)
    {
    case SERVO_STOP:
        if (info->type == Servo_type)
        {
            servo_sleep(info->slot, SLEEP_ON);
            set_duty_cycle(info->slot, 0);
            info->ctrl.mode = SERVO_IDLE;
            check_position_changed(info);
            debug_print("STOP\n");
        }
        else if (info->type == Shutter_type)
        {
            servo_sleep(info->slot, SLEEP_ON);
            set_duty_cycle(info->slot, 0);
            info->ctrl.mode = SERVO_IDLE;
        }
        log_id = SERVO_CNTRL_STOP;
        break;

    case SERVO_IDLE:
        log_id = SERVO_CNTRL_IDLE;
        break;

    case SERVO_VEL_MOVE:
        set_direction(info->slot, info->ctrl.cmd_dir);
        set_duty_cycle(info->slot, info->save.store.pwm);
        servo_sleep(info->slot, SLEEP_DISABLED);

        /* every time a velocity move is performed, we need to set the position
         * to SERVO_NO_POSITION because we may be somewhere in the middle */
        info->save.store.position = SERVO_NO_POSITION;
        log_id = SERVO_CNTRL_VEL_MOVE;
        break;

    case SERVO_GOTO:
        goto_ctrl(info);
        log_id = SERVO_CNTRL_GOTO;
        break;

    case SERVO_POSITION_TOGGLE:
        if (info->save.store.position == SERVO_POSITION_1)
        {
            info->ctrl.cmd_position = SERVO_POSITION_2;
            log_id = SERVO_CNTRL_POSITION_1;
        }
        else if (info->save.store.position == SERVO_POSITION_2)
        {
            info->ctrl.cmd_position = SERVO_POSITION_1;
            log_id = SERVO_CNTRL_POSITION_2;
        }
        else
        {
            info->ctrl.cmd_position = SERVO_POSITION_1;
            log_id = SERVO_CNTRL_POSITION_1;
        }

        /*Run the goto command for the new position*/
        info->ctrl.mode = SERVO_GOTO;
        info->ctrl.goto_mode = SERVO_SART_MOVE;
        break;

    case SHUTTER_OPEN:
        /*The task runs at 10ms so when we toggle shutter set the direction
         * and pwm high here then go to wait state
         */
        info->shutter.position = SHUTTER_OPENED;
        shutter_action(info);
        log_id = SERVO_CNTRL_OPEN;
        break;

    case SHUTTER_CLOSE:
        /*The task runs at 10ms so when we toggle shutter set the direction
         * and pwm high here then go to wait state
         */
        info->shutter.position = SHUTTER_CLOSED;
        shutter_action(info);
        log_id = SERVO_CNTRL_CLOSE;
        break;

    case SHUTTER_HOLD_PULSE:
        if (info->save.config.saved_params.shutter_params.mode == SHUTTER_PULSED)
        {
            if (info->shutter.on_time_counter
                    >= info->save.config.saved_params.shutter_params.on_time)
            {
                info->ctrl.mode = SERVO_STOP;
            }
        }
        else /*Hold pulse voltage*/
        {
            if (info->shutter.on_time_counter
                    >= info->save.config.saved_params.shutter_params.on_time)
            {
                set_duty_cycle(info->slot,
                        info->save.config.saved_params.shutter_params.duty_cycle_hold);
                info->ctrl.mode = SHUTTER_HOLD_VOLTGAE;
            }
        }
        info->shutter.on_time_counter++;
        log_id = SERVO_CNTRL_HOLD_PULSE;
        break;

    case SHUTTER_HOLD_VOLTGAE:
        /*Just sit here to hold the voltage until a new command comes in to change shutter
         * position*/
        log_id = SERVO_CNTRL_HOLD_VOLTGAE;
        break;

    }
    if (log_id != SERVO_NO_LOG)
        setup_and_send_log(slave_message, info->slot, LOG_NO_REPEAT,
                SERVO_LOG_TYPE, log_id, 0, 0, 0);
}

static void service_joystick(Servo_info *info,
                             pipeline_hid_in_t::unique_queue& queue) {
    message::hid_in_message msg;
    while (queue.try_pop(msg, 0)) {
#if 0
        switch (host_message->type)
        {
        case Xhid:
        case Yhid:
        case Zhid:
        case Rx:
        case Ry:
        case Rz:
            break;

        case Dial:
            break;

        case Wheel:
            break;

        case Button:
            switch ((uint8_t) msg.mode)
            {
            case BT_TOGGEL_DISABLE:
                break;

            case BT_TOGGEL_SPEED:
                break;

            case BT_TOGGEL_POSITION:
                if(info->type == Servo_type)
                {
                    info->ctrl.mode = SERVO_POSITION_TOGGLE;
                }
                else if(info->type == Shutter_type)
                {
                    if(info->shutter.position == SHUTTER_CLOSED)
                    {
                        info->ctrl.mode = SHUTTER_OPEN;
                    }
                    else
                    {
                        info->ctrl.mode = SHUTTER_CLOSE;
                    }
                }
                break;

            case BT_SLOT_SELECT:
                break;

            case BT_HOME:
                break;

            default:
                return;
            }
            break;

        }
#endif
    }
}

// MARK:  SPI Mutex Required
static void service_external_trigger(Servo_info *info,
        USB_Slave_Message *slave_message)
{
    uint32_t interrupt_val = 0;

    /*Read the interrupt to get the data and clear the interrupt*/
    read_reg(C_READ_INTERUPTS, info->slot, NULL, &interrupt_val);
    slots[info->slot].interrupt_flag_cpld = 0;

    if (info->save.config.saved_params.shutter_params.external_trigger_mode == TOGGLE)
    {
        /*Toggle the shutter*/
        if (info->shutter.position == SHUTTER_OPENED)
        {
            info->ctrl.mode = SHUTTER_CLOSE;
        }
        else
        {
            info->ctrl.mode = SHUTTER_OPEN;
        }
    }
    else if (Tst_bits(interrupt_val, 1 << 0))    // high
    {
        if (info->save.config.saved_params.shutter_params.external_trigger_mode == REVERSED)
        {
            info->ctrl.mode = SHUTTER_CLOSE;
        }
        else
        {
            info->ctrl.mode = SHUTTER_OPEN;
        }
    }
    else //low
    {
        if (info->save.config.saved_params.shutter_params.external_trigger_mode == REVERSED)
        {
            info->ctrl.mode = SHUTTER_OPEN;
        }
        else
        {
            info->ctrl.mode = SHUTTER_CLOSE;
        }
    }
}

/**
 * @brief There are 3 mode for the external trigger: Disabled, Enabled, and Reversed.
 * @param info
 */
static void setup_external_trigger(Servo_info *info)
{
    if (info->save.config.saved_params.shutter_params.external_trigger_mode >= ENABLED)
    {
        /*Enable the interrupt*/
        slots[info->slot].p_interrupt_cpld_handler = (void (*) (uint8_t, void*))&trig_handler;
        setup_slot_interrupt_CPLD(info->slot);
    }
}

// MARK:  SPI Mutex Required
static void service_cdc(Servo_info *info, USB_Slave_Message *slave_message,
                        service::itc::pipeline_cdc_t::unique_queue& queue) {
    while (queue.try_pop(*slave_message)) {
        parse(info, slave_message);
    }
}

// MARK:  SPI Mutex Required
static void service_store(Servo_info * p_info)
{
    if (p_info->store_counter < SERVO_STORE_DELAY) {
        ++p_info->store_counter;
    } else if (p_info->store_counter == SERVO_STORE_DELAY) {
        ++p_info->store_counter;
        write_eeprom_store(p_info);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
void setup_io(slot_nums slot)
{
    // setup the sleep pin
    switch ((uint8_t) slot)
    {
    case SLOT_1:
        set_pin_as_output(PIN_SMIO_1, 0);
        break;
    case SLOT_2:
        set_pin_as_output(PIN_SMIO_2, 0);
        break;
    case SLOT_3:
        set_pin_as_output(PIN_SMIO_3, 0);
        break;
    case SLOT_4:
        set_pin_as_output(PIN_SMIO_4, 0);
        break;
    case SLOT_5:
        set_pin_as_output(PIN_SMIO_5, 0);
        break;
    case SLOT_6:
        set_pin_as_output(PIN_SMIO_6, 0);
        break;
    case SLOT_7:
        set_pin_as_output(PIN_SMIO_7, 0);
        break;
    }
}

/****************************************************************************
 * Task
 ****************************************************************************/

static void task_servo (Servo_info * p_info)
{
    // BEGIN    Task Initialization
    
    /*Wait here until power is good*/
    while (board.power_good == POWER_NOT_GOOD)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    /* Create the structure for holding the USB data received from the USB slave task*/
    USB_Slave_Message slave_message;

    auto cdc_queue = *service::itc::pipeline_cdc().create_queue(
        static_cast<asf_destination_ids>((int)p_info->slot + (int)SLOT_1_ID));

    // Initialize device-indepedent IO
    setup_io(p_info->slot);

    auto joystick_in_queue = std::move(
        *pipeline_hid_in().create_queue(p_info->slot));

    // END      Task Initialization
    
    for (;;)
    {
        // Wait until the gate is opened (valid device is connected)
        xGatePass(slots[p_info->slot].device.connection_gate, portMAX_DELAY);
        
        // BEGIN    Device Initialization

        // Initialize variables not saved in the EEPROM.
        set_vars(p_info);

        // Read saved data for this stage
        xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);
        read_eeprom_data(p_info);
        read_eeprom_store(p_info);
        xSemaphoreGive(xSPI_Semaphore);

        bool save_data_valid = p_info->save.config.valid();

        if (! save_data_valid)
        {
            // Get configs from the device lut.
            Servo_Save_Union cfgs;
            lut_manager::device_lut_key_t key;
            xSemaphoreTake(slots[p_info->slot].xSlot_Mutex, portMAX_DELAY);
            key.running_slot_card = slots[p_info->slot].card_type;
            key.connected_device = slots[p_info->slot].device.info.signature;
            xSemaphoreGive(slots[p_info->slot].xSlot_Mutex);
            LUT_ERROR err = lut_manager::load_data(LUT_ID::DEVICE_LUT, &key, &cfgs.cfg);

            bool order_valid = cfgs.cfg.servo_params.struct_id == static_cast<struct_id_t>(Struct_ID::SERVO_PARAMS)
                && cfgs.cfg.shutter_params.struct_id == static_cast<struct_id_t>(Struct_ID::SERVO_SHUTTER_PARAMS)
                && cfgs.cfg.servo_store.struct_id == static_cast<struct_id_t>(Struct_ID::SERVO_STORE);

            if (err == LUT_ERROR::OKAY && ! order_valid)
            {
                err = LUT_ERROR::ORDER_VIOLATION;
            }

            if (err == LUT_ERROR::OKAY)
            {
                // Construct the save structure.
                save_data_valid = save_constructor::construct(cfgs.array,
                    SERVO_CONFIGS_IN_SAVE, &p_info->save.config.saved_params);
            }

            // Check to see if the store needs to be reloaded.
            const bool SHOULD_KEEP_STORE = servo_store_not_default(&p_info->save.store);
            if (save_data_valid && !SHOULD_KEEP_STORE)
            {
                // Construct the store.
                save_data_valid = save_constructor::construct(&cfgs.cfg.servo_store, 1, &p_info->save.store);
            }
        }

        if (save_data_valid)
        {
            // Configuration was successfully loaded into memory.

            xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);
            if (p_info->type == Servo_type)
            {
                set_duty_cycle(p_info->slot, 0);
            }
            else /*Shutter_type*/
            {
                if (p_info->save.config.saved_params.shutter_params.initial_state == SHUTTER_CLOSED)
                    p_info->ctrl.mode = SHUTTER_CLOSE;
                else
                    p_info->ctrl.mode = SHUTTER_OPEN;

                setup_external_trigger(p_info);

                uint32_t interrupt_val = 0;
                /*Read the interrupt to get the data and clear the interrupt*/
                read_reg(C_READ_INTERUPTS, p_info->slot, NULL, &interrupt_val);
                slots[p_info->slot].interrupt_flag_cpld = 0;

            }

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

            if (slots[p_info->slot].interrupt_flag_cpld == 1)
            {
                service_external_trigger(p_info, &slave_message);
            }
            else
            {
                service_cdc(p_info, &slave_message, cdc_queue);

                /*Check if new data from a USB host device (joystick)*/
                service_joystick(p_info, joystick_in_queue);

                service_store(p_info);
            }

            /*Service the main control loop for the servo task*/
            service_servo(p_info, &slave_message);

            xSemaphoreGive(xSPI_Semaphore);

            ulTaskNotifyTake( pdTRUE, SERVO_UPDATE_INTERVAL);

            // END      Operation Loop
        }

        // BEGIN    Device Cleanup

        // Setup servo to stop.
        p_info->ctrl.mode = SERVO_CNTRL_STOP;
        service_servo(p_info, &slave_message);

        // END      Device Cleanup
    }
}


extern "C" void servo_init(uint8_t slot, slot_types type)
{
    Servo_info *pxServo_info;

    /* Allocate a stepper structure for each task.*/
    pxServo_info = new Servo_info;

    char pcName[6] =
    { 'S', 'e', 'r', 'v', 'X' };
    pcName[4] = slot + 0x30;

    pxServo_info->slot = static_cast<slot_nums>(slot);

    pxServo_info->type = type;

    pxServo_info->store_counter = SERVO_STORE_DELAY + 1;

    /*The shutter has an external trigger on an interrupt so we need to raise the
     * priority by 1*/
    UBaseType_t uxPriority = ((type == Shutter_type) ?
    (TASK_SERVO_STACK_PRIORITY + 1) :TASK_SERVO_STACK_PRIORITY);

    /* Create task for each servo card */
    if (xTaskCreate((TaskFunction_t) task_servo, pcName,
    TASK_SERVO_STACK_SIZE, (Servo_info*) pxServo_info, uxPriority,
            &xSlotHandle[slot]) != pdPASS)
    {
        debug_print("Failed to create servo task for slot %d\r\n", slot + 1);

        /* free pxServo_info*/
        delete pxServo_info;
    }
}

