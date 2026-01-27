/**
 * @file stepper.c
 *
 * @brief Functions for ???
 *
 */

#include "stepper.h"
#include "apt-command.hh"
#include "apt-parsing.hh"
#include "apt.h"
#include "hid-in-message.hh"
#include "hid_in.h"
#include "itc-service.hh"
#include "mcm_speed_limit.hh"
#include "pins.h"
#include "stepper.details.hh"
#include "stepper_control.h"
#include "stepper_log.h"
#include "supervisor.h"
#include "sys_task.h"
#include "usb_device.h"
#include "usb_slave.h"
#include "usr_limits.h"
#include <apt_parse.h>
#include <asf.h>
#include <cpld.h>
#include "Debugging.h"
#include <pins.h>
#include <string.h>
#include <usb_host.h>

#include "../../drivers/log/log.h"
#include "25lc1024.h"
#include "cppmem.hh"
#include "heartbeat_watchdog.hh"
#include "helper.h"
#include "hid.h"
#include "lock_guard.hh"
#include "lut_manager.hh"
#include "pnp_status.hh"
#include "save_constructor.hh"
// #include "synchronized_motion.h"
#include <save_util.hh>

// #include "piezo.h"
#include <encoder_abs_magnetic_rotation.h>

// extern SemaphoreHandle_t xUSB_Slave_TxSemaphore;
// extern SemaphoreHandle_t xSPI_Semaphore;

/****************************************************************************
 * Private Data
 ****************************************************************************/

constexpr pnp_status_t PNP_STATUS_MASK =
    ~(pnp_status::GENERAL_CONFIG_ERROR | pnp_status::CONFIGURATION_SET_MISS |
      pnp_status::CONFIGURATION_STRUCT_MISS);

constexpr std::size_t APT_RESPONSE_BUFFER_SIZE = drivers::usb::apt_response::RESPONSE_BUFFER_SIZE;

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/
static void setup_io(Stepper_info *info);
// static void reset_card(slot_nums slot, uint8_t mode);
static void load_params_to_drive(Stepper_info *info);
static void set_position_stepper(Stepper_info *info);
static void get_position_stepper(Stepper_info *info);
static void set_el_pos_stepper(Stepper_info *info);
static bool slot_number_matches_slot_id(uint8_t slot_number, uint8_t slot_id);
static void set_vars(Stepper_info *info);
static void parse(Stepper_info *info, USB_Slave_Message *slave_message,
                  bool active);
static bool parse_active(Stepper_info *info, USB_Slave_Message *slave_message,
                         uint8_t *response_buffer, uint8_t *p_length,
                         bool *p_need_to_reply, stepper_log_ids *p_log_id);
static void handle_save_command(Stepper_info *const p_info,
                                const uint16_t command_to_save);
static void service_joystick(Stepper_info *info,
                             service::itc::pipeline_hid_in_t::unique_queue& queue,
                             USB_Slave_Message *slave_message, bool active);
static bool service_cdc(Stepper_info *info, USB_Slave_Message *slave_message,
                        bool active, service::itc::pipeline_cdc_t::unique_queue& queue);
static void service_encoder(Stepper_info *info);
// static void service_synchronized_motion(Stepper_info *info,
//                                         stepper_sm_Rx_data *sm_rx,
//                                         stepper_sm_Tx_data *sm_tx);
// static void service_piezo_synchronized_motion(Stepper_info *info,
//                                               piezo_sm_Rx_data *sm_rx,
//                                               piezo_sm_Tx_data *sm_tx);
static bool try_start_home(Stepper_info* info);
static void start_jog(Stepper_info* const info, bool ccw_cw_flag);
static void start_velocity_simple(Stepper_info& info, bool flag_reverse_forward);
static void start_velocity(Stepper_info& info, bool flag_reverse_forward, uint32_t speed);
static void reset_speed_channels(Stepper_info* p_info);

static void task_stepper(Stepper_info *p_info);

/****************************************************************************
 * Private Functions
 ****************************************************************************/
/**
 * @brief Setup the busy pin for each slot if the slot has a stepper card.  This
 * signal let us know when the drive has completed its task like a goto or if it
 * is moving
 *
 * If the encoder is AEAT-8800-Q24 magnetic rotational encoder we need to to set
 * the ADC pin from the uC to an output.  This is used to turn power on and off
 * to this device because we need to reset it before every move. There is no
 * reset signal so we just power cycle it.  We found when the magnet was removed
 * hen reinserted, the counts where not the same, reseting made the counts
 * always the same.
 * @param slot
 */

static void setup_io(Stepper_info *info) {
    bool mag_enc = false;
    if ((info->enc.type == ENCODER_TYPE_ABS_MAGNETIC_ROTATION) ||
        (info->enc.type == ENCODER_TYPE_ABS_MAGNETIC_ROTATION_AUTO_HOME) ||
        (info->enc.type == ENCODER_TYPE_ABS_MAGNETIC_ROTATION_LW) ||
        (info->enc.type == ENCODER_TYPE_ABS_MAGNETIC_ROTATION_MANUAL)) {
        mag_enc = true;
    }

    switch ((uint8_t)info->slot) {
    case SLOT_1:
        // busy pin for the L6470 stepper drive IC
        set_pin_as_input(PIN_SMIO_1);
        // ADC pin use as IO for magnetic encoder power
        if (mag_enc) {
            set_pin_as_output(PIN_ADC_1, 1);
        }

        break;

    case SLOT_2:
        // busy pin for the L6470 stepper drive IC
      set_pin_as_input(PIN_SMIO_2);
        // ADC pin use as IO for magnetic encoder power
        if (mag_enc)
            set_pin_as_output(PIN_ADC_2, 1);
        break;
    case SLOT_3:
        // busy pin for the L6470 stepper drive IC
        set_pin_as_input(PIN_SMIO_3);
        // ADC pin use as IO for magnetic encoder power
        if (mag_enc)
            set_pin_as_output(PIN_ADC_3, 1);
        break;
    case SLOT_4:
        // busy pin for the L6470 stepper drive IC
        set_pin_as_input(PIN_SMIO_4);
        // ADC pin use as IO for magnetic encoder power
        if (mag_enc)
            set_pin_as_output(PIN_ADC_4, 1);
        break;
    case SLOT_5:
        // busy pin for the L6470 stepper drive IC
        set_pin_as_input(PIN_SMIO_5);
        // ADC pin use as IO for magnetic encoder power
        if (mag_enc)
            set_pin_as_output(PIN_ADC_5, 1);
        break;
    case SLOT_6:
        // busy pin for the L6470 stepper drive IC
        set_pin_as_input(PIN_SMIO_6);
        // ADC pin use as IO for magnetic encoder power
        if (mag_enc)
            set_pin_as_output(PIN_ADC_6, 1);
        break;
    case SLOT_7:
        // busy pin for the L6470 stepper drive IC
        set_pin_as_input(PIN_SMIO_7);
        // ADC pin use as IO for magnetic encoder power
        if (mag_enc)
            set_pin_as_output(PIN_ADC_7, 1);
        break;
    }
}

// Reference code section for default parameters.
#if 0
static void set_default_params_lc(Stepper_info *info)
{
    /*Load default values into ram*/
    info->save.config.params.drive.acc = 800;
    info->save.config.params.drive.dec = 800;
    info->save.config.params.drive.max_speed = 15;
    info->save.config.params.drive.min_speed = 0;
    info->save.config.params.drive.fs_spd = 1023;
    info->save.config.params.drive.kval_hold = 23;
    info->save.config.params.drive.kval_run = 200;
    info->save.config.params.drive.kval_acc = 200;
    info->save.config.params.drive.kval_dec = 200;
    info->save.config.params.drive.int_speed = 6930;
    info->save.config.params.drive.stall_th = 31;
    info->save.config.params.drive.st_slp = 29;
    info->save.config.params.drive.fn_slp_acc = 68;
    info->save.config.params.drive.fn_slp_dec = 68;
    info->save.config.params.drive.ocd_th = 31;
    info->save.config.params.drive.step_mode = 7;
    info->save.config.params.drive.config = 3720; // 0x2e88
}

/**
 * @brief Sets the default drive parameters to ram variables.
 * @param info
 */
static void set_default_params_hc(Stepper_info *info)
{
    /*Load default values into ram*/
    info->save.config.params.drive.acc = 800;
    info->save.config.params.drive.dec = 800;
    info->save.config.params.drive.max_speed = 50;
    info->save.config.params.drive.min_speed = 0;
    info->save.config.params.drive.fs_spd = 255;
    info->save.config.params.drive.kval_hold = 16;
    info->save.config.params.drive.kval_run = 20;
    info->save.config.params.drive.kval_acc = 20;
    info->save.config.params.drive.kval_dec = 20;
    info->save.config.params.drive.int_speed = 2000;
    info->save.config.params.drive.stall_th = 31;
    info->save.config.params.drive.st_slp = 29;
    info->save.config.params.drive.fn_slp_acc = 95;
    info->save.config.params.drive.fn_slp_dec = 95;
    info->save.config.params.drive.ocd_th = 31;
    info->save.config.params.drive.step_mode = 7;
    info->save.config.params.drive.config = 0x2F00;
    info->save.config.params.drive.gatecfg1 = 0x00EC; // 0x00AC; /*set IGATE to 32mA and TCC to 1250ns to charge 50 nC gate*/
    info->save.config.params.drive.gatecfg2 = 0xE0; //0x61; /*500 ns blanking and dead times*/
    info->save.config.params.drive.approach_vel = 1000;
    info->save.config.params.drive.deadband = 0;
    info->save.config.params.drive.backlash = 0;
    info->save.config.params.drive.kickout_time = 0;
}
#endif

static void read_eeprom_store(Stepper_info *p_info) {
    const uint32_t BASE_ADDRESS = SLOT_EEPROM_ADDRESS(p_info->slot, 1);

    // Read in high-frequency data on page 1.
    /* Page 1 high frequency changing variables*/
    /* read all the stepper save data into stepper save structure*/
    eeprom_25LC1024_read(BASE_ADDRESS, sizeof(p_info->save.store),
                         (uint8_t *)&p_info->save.store);
}

/**
 * @brief Control of reset pin for the stepper driver.
 * @param slot : slot number starting with 0
 * @param mode :  High (1) is enabled, low (0) is disabled.
 */
// static void reset_card(slot_nums slot, uint8_t mode)
//{
//     set_reg(C_SET_DIG_OUT, slot, 0, (uint32_t) mode);
// }
/**
 * This load the parameters to the drive from the EEPROM stored values.
 * @param info : The structure that holds all the stepper info for this slot
 */
// MARK:  SPI Mutex Required
static void load_params_to_drive(Stepper_info *info) {

    // New device detection system ensures that only vaid configurations and
    // devices will advance to this function call.

    if ((info->enc.type == ENCODER_TYPE_ABS_MAGNETIC_ROTATION) ||
        (info->enc.type == ENCODER_TYPE_ABS_MAGNETIC_ROTATION_AUTO_HOME) ||
        (info->enc.type == ENCODER_TYPE_ABS_MAGNETIC_ROTATION_LW) ||
        (info->enc.type == ENCODER_TYPE_ABS_MAGNETIC_ROTATION_MANUAL)) {
        set_reg(C_SET_ENABLE_STEPPER_CARD, info->slot, 0, STEP_MODE_MAG);
        // Power off the optical switch
        set_reg(C_SET_STEPPER_DIGITAL_OUTPUT, info->slot, 0, 0);
    } else if (info->enc.type == ENCODER_TYPE_ABS_BISS_LINEAR)
        set_reg(C_SET_ENABLE_STEPPER_CARD, info->slot, 0, STEP_MODE_ENC_BISS);
    else
        set_reg(C_SET_ENABLE_STEPPER_CARD, info->slot, 0, ENABLED_MODE);

    xSemaphoreGive(xSPI_Semaphore);
    vTaskDelay(pdMS_TO_TICKS(2));
    xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);

    // make sure motors are in Hi impedance
    hard_hiz_stepper(info->slot);

    set_parameter(info->slot, ACC_L, 2, info->save.config.params.drive.acc);
    set_parameter(info->slot, DEC, 2, info->save.config.params.drive.dec);
    set_parameter(info->slot, MAX_SPEED, 2,
                  info->save.config.params.drive.max_speed);
    set_parameter(info->slot, MIN_SPEED, 2,
                  info->save.config.params.drive.min_speed);
    set_parameter(info->slot, FS_SPD, 2, info->save.config.params.drive.fs_spd);
    set_parameter(info->slot, KVAL_RUN, 1,
                  info->save.config.params.drive.kval_run);
    set_parameter(info->slot, KVAL_HOLD, 1,
                  info->save.config.params.drive.kval_hold);
    set_parameter(info->slot, KVAL_ACC, 1,
                  info->save.config.params.drive
                      .kval_run); /*make acc current same as kval_run*/
    set_parameter(info->slot, KVAL_DEC, 1,
                  info->save.config.params.drive
                      .kval_run); /*make dec current same as kval_run*/
    set_parameter(info->slot, INT_SPD, 2,
                  info->save.config.params.drive.int_speed);
    set_parameter(info->slot, STALL_TH, 1,
                  info->save.config.params.drive.stall_th);
    set_parameter(info->slot, ST_SLP, 1, info->save.config.params.drive.st_slp);
    set_parameter(info->slot, FN_SLP_ACC, 1,
                  info->save.config.params.drive.fn_slp_acc);
    set_parameter(info->slot, FN_SLP_DEC, 1,
                  info->save.config.params.drive.fn_slp_dec);
    set_parameter(info->slot, OCD_TH, 1, info->save.config.params.drive.ocd_th);
    set_parameter(info->slot, STEP_MODE, 1,
                  info->save.config.params.drive.step_mode);

    if (info->ctrl.which_stepper_drive == LOW_CURRENT_DRIVE) {
        set_parameter(info->slot, CONFIG_L6470, 2,
                      info->save.config.params.drive.config);
    } else {
        set_parameter(info->slot, CONFIG_L6480, 2,
                      info->save.config.params.drive.config);
        set_parameter(info->slot, GATECFG1, 2,
                      info->save.config.params.drive.gatecfg1);
        set_parameter(info->slot, GATECFG2, 1,
                      info->save.config.params.drive.gatecfg2);
    }
}

// MARK:  SPI Mutex Required
static void set_position_stepper(Stepper_info *info) {
    int32_t position = info->counter.step_pos;

    /*Make sure the motor is stopped or the command will be ignored*/
    hard_stop_stepper(info->slot);

    /* if counting is reversed multiple by -1*/
    if (info->save.config.params.flags.flags & STEPPER_REVERSED)
        position *= -1;

    if (position < 0) {
        convert_signed32_to_twos_comp(position);
    }

    set_parameter(info->slot, ABS_POS, 3, position);
}

/**
 * @brief Get the position value from the stepper drive, this is in 2's
 * complement
 * @param info
 */
// MARK:  SPI Mutex Required
static void get_position_stepper(Stepper_info *info) {
    uint8_t spi_tx_data[4];
    int32_t delta_counts;
    int32_t pre_step_pos_raw = info->counter.step_pos;

    get_parameter(info->slot, (ABS_POS | 0x20), 3, spi_tx_data);

    uint32_t temp =
        (spi_tx_data[1] << 16) + (spi_tx_data[2] << 8) + (spi_tx_data[3]);

    /* convert the previous step_pos to signed value and use this to calculate
     * the delta between the previous value and this one */
    // pre_step_pos_raw = convert_twos_comp_to_signed32(info->counter.step_pos);

    /* convert the value read from the stepper IC position to signed value and
     * store*/
    info->counter.step_pos = convert_twos_comp_to_signed32(temp);

    /* if counting is reversed multiple by -1*/
    if (info->save.config.params.flags.flags & STEPPER_REVERSED) {
        /*TODO if it is a magnetic encoder then we need to calculate the
         * wrap-around*/
        info->counter.step_pos *= -1;
    }

    delta_counts = info->counter.step_pos - pre_step_pos_raw;

    constexpr int32_t HALF_ABS_POS_WIDTH = 0x3FFFFF / 2;
    if (abs(delta_counts) > HALF_ABS_POS_WIDTH) {
        delta_counts = -info->counter.step_pos - pre_step_pos_raw;
    }

    info->counter.step_pos_32bit += delta_counts;
}

// MARK:  SPI Mutex Required
static void set_el_pos_stepper(Stepper_info *info) {
    set_parameter(info->slot, EL_POS, 2, info->save.store.el_pos);
}

static bool slot_number_matches_slot_id(uint8_t slot_number, uint8_t slot_id) {
    uint8_t temp = (slot_id ^ 0x20) - 1;
    return slot_number == temp;
}

static void set_vars(Stepper_info *info) {
    /*These are all the variables that are not saved in EEPROM and should be
     * initialized*/
    info->channel_enable = false;

    info->counter.cmnd_pos = 0;
    info->counter.goto_pos = 0;
    info->counter.step_pos = 0;
    info->counter.step_pos_32bit = 0;

    info->limits.cw = 0;
    info->limits.ccw = 0;
    info->limits.index = 0;
    info->limits.soft_cw = 0;
    info->limits.soft_ccw = 0;
    info->limits.hard_stop = 0;
    info->limits.homed = NOT_HOMED_STATUS;
    info->limits.interlock = 0;
    info->limits.limit_flag_for_logging = false;

    info->ctrl.status_bits = 0;
    info->ctrl.busy = 0;

    //    if (info->enc.type == ENCODER_TYPE_ABS_MAGNETIC_ROTATION_AUTO_HOME)
    //    {
    //        info->ctrl.mode = HOMING;
    //        info->enc.homed = 0;
    //    }
    //    else
    info->ctrl.mode = STOP;

    info->ctrl.current_speed_channel = STEPPER_SPEED_CHANNEL_UNBOUND;
    info->ctrl.speed_control.absolute = 0;
    info->ctrl.speed_control.relative = 0;
    info->ctrl.speed_control.velocity = 0;

    info->enc.home_delay_cnt = 0;

    info->ctrl.cmd_dir = DIR_REVERSE;
    info->ctrl.cur_dir = DIR_REVERSE;
    info->ctrl.cur_vel = 0;
    info->ctrl.goto_mode = GOTO_IDLE;
    info->ctrl.selected_stage = false;

    info->enc.home_delay_cnt = 0;

    // temp set default PID params EEPROM isn't done yet
    // hexapod Kp = 100, Ki = 0, Kd = 350
    // pls stage  Kp = 100, Ki = 0, Kd = 120
    //    info->save.config.params.pid.Kp = 5;
    //    info->save.config.params.pid.Ki = 0;
    //    info->save.config.params.pid.Kd = 5;
    //    info->save.config.params.pid.imax = 1e6;

    info->counter.cmnd_pos = 0;
    info->pid.iterm = 0;
    info->pid.lastin = 0;

    info->collision = false;
}

/**
 * Parses APT commands to the stepper card that only run while a driving a
 * device. \param[inout]        info The stepper info structure. \param[in]
 * slave_message This is the message structured passed from the USB slave task.
 * \return true The message was parsed.
 * \return false The message was not parsed.
 */
// MARK:  SPI Mutex Required
static bool parse_active(Stepper_info *info, USB_Slave_Message *slave_message,
                         uint8_t *response_buffer, uint8_t *p_length,
                         bool *p_need_to_reply, stepper_log_ids *p_log_id) {
    using namespace drivers::apt;
    bool rt = true;

    int32_t temp_32_1;

    auto command = drivers::usb::apt_basic_command(*slave_message);

    switch (slave_message->ucMessageID) {
    /*TODO may want to take out because we are replacing with our own*/
    case MGMSG_MOT_REQ_STATUSUPDATE: /* 0x0480 */
        /* Header */
        response_buffer[0] = (uint8_t)MGMSG_MOT_GET_STATUSUPDATE;
        response_buffer[1] = MGMSG_MOT_GET_STATUSUPDATE >> 8;
        response_buffer[2] = 14;
        response_buffer[3] = 0x00;
        response_buffer[4] =
            HOST_ID | 0x80; /* destination & has extended data*/
        response_buffer[5] =
            SLOT_1_ID + static_cast<uint8_t>(info->slot); /* source*/

        /*Chan Ident*/
        response_buffer[6] = info->slot;
        response_buffer[7] = 0x00;

        /*Position*/
        memcpy(&response_buffer[8], &info->counter.step_pos,
               sizeof(info->counter.step_pos));

        /*EncCount*/
        memcpy(&response_buffer[12], &info->enc.enc_pos,
               sizeof(info->enc.enc_pos));

        /*Status Bits*/
        memcpy(&response_buffer[16], &info->ctrl.status_bits,
               sizeof(info->ctrl.status_bits));

        *p_need_to_reply = true;
        *p_length = 20;
        break;

    case MGMSG_MCM_REQ_STEPPER_LOG: /* 0x4051 */
        /* Header */
        response_buffer[0] = (uint8_t)MGMSG_MCM_GET_STEPPER_LOG;
        response_buffer[1] = MGMSG_MCM_GET_STEPPER_LOG >> 8;
        response_buffer[2] = 14 +
#if ENABLE_STEPPER_LOG_STATUS_FLAGS
                             4
#else
                             0
#endif
            ;
        response_buffer[3] = 0x00;
        response_buffer[4] =
            HOST_ID | 0x80; /* destination & has extended data*/
        response_buffer[5] =
            SLOT_1_ID + static_cast<uint8_t>(info->slot); /* source*/

        /*Chan Ident*/
        response_buffer[6] = info->slot;
        response_buffer[7] = 0x00;

        /*Command position*/
#if 1 // normal
        memcpy(&response_buffer[8], &info->counter.cmnd_pos,
               sizeof(info->counter.cmnd_pos));
#elif 0 // send the stepper counts
        memcpy(&response_buffer[8], &info->counter.step_pos_32bit,
               sizeof(info->counter.step_pos_32bit));
#elif 0 // send the stepper error
        memcpy(&response_buffer[8], &info->pid.error, sizeof(info->pid.error));
#endif

        /*PID error*/
        memcpy(&response_buffer[12], &info->pid.error, sizeof(info->pid.error));

        /*EncCount*/
        memcpy(&response_buffer[16], &info->enc.enc_pos,
               sizeof(info->enc.enc_pos));

#if ENABLE_STEPPER_LOG_STATUS_FLAGS
        memcpy(&response_buffer[20], &info->stepper_status,
               sizeof(info->stepper_status));
        info->stepper_status_read = false;
#endif

        *p_need_to_reply = true;
        *p_length = 6 + 14 +
#if ENABLE_STEPPER_LOG_STATUS_FLAGS
                    4
#else
                    0
#endif
            ;
        break;

    case MGMSG_MOT_SET_ENCCOUNTER: /* 0x0409 */
        /* Check slot_number matches slot_id, if they don't return throwing out
         * the message*/
        if (slot_number_matches_slot_id(slave_message->extended_data_buf[0],
                                        slave_message->destination)) {

            if (Tst_bits(info->save.config.params.flags.flags, HAS_ENCODER)) {
                int32_t new_enc_counts;
                memcpy(&new_enc_counts, &slave_message->extended_data_buf[2],
                       4);
                switch (info->enc.type) {
                case ENCODER_TYPE_NONE:
                case ENCODER_TYPE_QUAD_LINEAR:
                    shift_soft_limits(&info->save.config.params.limits,
                                      new_enc_counts - info->enc.enc_pos_raw);
                    break;
                default:
                    break;
                }
                set_encoder_position(info->slot,
                                     info->save.config.params.flags.flags,
                                     &info->enc, new_enc_counts);
                sync_stepper_steps_to_encoder_counts(info);

                if (Tst_bits(info->save.config.params.flags.flags,
                             USE_PID)) //&&
                                       //!Tst_bits(info->save.config.params.flags.flags,
                                       //PID_KICKOUT))
                {
                    info->counter.cmnd_pos = info->enc.enc_pos;
                }
            }
            /* If no encoder then copy stepper count to encoder counts with
               coef*/
            else /* No encoder*/
            {
                memcpy(&info->enc.enc_pos, &slave_message->extended_data_buf[2],
                       4);
                sync_stepper_steps_to_encoder_counts(info);
            }

            // Reset the homed flag.
            info->limits.homed = NOT_HOMED_STATUS;

            /*Set flags to save it to EEPROM*/
            info->stage_position_saved = false;
            info->stage_moved_elapse_count =
                STEPPER_SAVE_POSITION_IDLE_TIME + 1;
            write_flash(info);
            *p_log_id = STEPPER_SET_ENCCOUNTER;
        }
        break;

    case MGMSG_MOT_REQ_ENCCOUNTER: /* 0x040A */
        /* Header */
        response_buffer[0] = (uint8_t)MGMSG_MOT_GET_ENCCOUNTER;
        response_buffer[1] = MGMSG_MOT_GET_ENCCOUNTER >> 8;
        response_buffer[2] = 12;
        response_buffer[3] = 0x00;
        response_buffer[4] =
            HOST_ID | 0x80; /* destination & has extended data*/
        response_buffer[5] =
            SLOT_1_ID + static_cast<uint8_t>(info->slot); /* source*/

        /*Chan Ident*/
        response_buffer[6] = info->slot;
        response_buffer[7] = 0x00;

        /*Encoder Counts*/
        memcpy(&response_buffer[8], &info->enc.enc_pos,
               sizeof(info->enc.enc_pos));

        *p_need_to_reply = true;
        *p_length = 12 + 6;
        break;

    case MGMSG_MOT_SET_POSCOUNTER: /* 0x0410 */
        memcpy(&info->counter.step_pos, &slave_message->extended_data_buf[2],
               4);
        set_position_stepper(info);
        *p_log_id = STEPPER_SET_POSCOUNTER;
        break;

    case MGMSG_MOT_REQ_POSCOUNTER: /* 0x0411 */
        /* Header */
        response_buffer[0] = (uint8_t)MGMSG_MOT_GET_POSCOUNTER;
        response_buffer[1] = MGMSG_MOT_GET_POSCOUNTER >> 8;
        response_buffer[2] = 12;
        response_buffer[3] = 0x00;
        response_buffer[4] =
            HOST_ID | 0x80; /* destination & has extended data*/
        response_buffer[5] =
            SLOT_1_ID + static_cast<uint8_t>(info->slot); /* source*/

        /*Chan Ident*/
        response_buffer[6] = info->slot;
        response_buffer[7] = 0x00;

        /*Stepper Counts*/
        memcpy(&response_buffer[8], &info->counter.step_pos, 4);

        *p_need_to_reply = true;
        *p_length = 12 + 6;
        break;

    case MGMSG_MOT_MOVE_ABSOLUTE: /* 0x0453*/
        /* Check slot_number matches slot_id, if they don't return throwing out
         * the message*/
        if (slot_number_matches_slot_id(slave_message->extended_data_buf[0],
                                        slave_message->destination)) {

            memcpy(&info->counter.cmnd_pos,
                   &slave_message->extended_data_buf[2], 4);

            sync_stepper_steps_to_encoder_counts(info);

            info->ctrl.current_speed_channel = STEPPER_SPEED_CHANNEL_ABSOLUTE;

            /*Use PID controller for goto*/
            if (Tst_bits(info->save.config.params.flags.flags, USE_PID)) {
                info->pid.max_velocity = 1;
                info->ctrl.mode = PID;
            } else /*Used normal goto*/
            {
                if (info->ctrl.mode != IDLE)
                    info->ctrl.goto_mode = GOTO_CLEAR;
                else
                    info->ctrl.goto_mode = GOTO_START;

                info->ctrl.mode = GOTO;
            }
            *p_log_id = STEPPER_MOVE_ABSOLUTE;
        }
        break;

    case MGMSG_MOT_MOVE_RELATIVE: /* 0x0448 */
    case MGMSG_MCM_MOT_MOVE_BY: /* 0x4050 */
        /* Check slot_number matches slot_id, if they don't return throwing out
         * the message*/
        if (slot_number_matches_slot_id(slave_message->extended_data_buf[0],
                                        slave_message->destination)) {
            memcpy(&temp_32_1, &slave_message->extended_data_buf[2], 4);
            info->counter.cmnd_pos = temp_32_1 + info->enc.enc_pos;

            sync_stepper_steps_to_encoder_counts(info);
            info->ctrl.current_speed_channel = STEPPER_SPEED_CHANNEL_RELATIVE;

            /*Use PID controller for goto*/
            if (Tst_bits(info->save.config.params.flags.flags, USE_PID)) {
                info->pid.max_velocity = 1;
                info->ctrl.mode = PID;
            } else /*Used normal goto*/
            {
                if (info->ctrl.mode != IDLE)
                    info->ctrl.goto_mode = GOTO_CLEAR;
                else
                    info->ctrl.goto_mode = GOTO_START;

                info->ctrl.mode = GOTO;
            }
            *p_log_id = STEPPER_MOVE_BY;
        }
        break;

    case MGMSG_MOT_MOVE_STOP: /* 0x0465*/
        info->ctrl.mode = STOP;
        *p_log_id = STEPPER_STOP;
        break;

    case MGMSG_MOT_MOVE_VELOCITY: /* 0x0457 */
        // param1 => channel, param2 => direction (0x01 forward, 0x02 reverse)
        if (slave_message->param1 == info->slot &&
            (slave_message->param2 == 0x01 || slave_message->param2 == 0x02)) {

            start_velocity_simple(*info, slave_message->param2 == 0x01 ? DIR_FORWARD : DIR_REVERSE);
            *p_log_id = STEPPER_SET_VELOCITY;
        }
        break;

    case MGMSG_MOT_MOVE_HOME: /* 0x0443*/
        if (try_start_home(info)) {
            *p_log_id = STEPPER_HOME;
        }
        break;

    case MGMSG_MOT_MOVE_JOG: /* 0x046A*/
        // get the direction of the jog
        start_jog(info, slave_message->param2 != 0);
        *p_log_id = STEPPER_JOG;
        break;

    case MGMSG_MOD_SET_CHANENABLESTATE: /*0x0210*/
        info->channel_enable = (bool)slave_message->param2;
        info->collision = false;
        sync_stepper_steps_to_encoder_counts(info);
        *p_log_id = STEPPER_SET_CHANENABLESTATE;
        break;

    case MGMSG_MOD_REQ_CHANENABLESTATE: /*0x0211*/
        response_buffer[0] = (uint8_t)MGMSG_MOD_GET_CHANENABLESTATE;
        response_buffer[1] = MGMSG_MOD_GET_CHANENABLESTATE >> 8;
        response_buffer[2] = info->slot;
        response_buffer[3] = (uint8_t)info->channel_enable;
        response_buffer[4] = HOST_ID; /* destination */
        response_buffer[5] =
            SLOT_1_ID + static_cast<uint8_t>(info->slot); /* source*/

        *p_need_to_reply = true;
        *p_length = 6;
        break;

    case MGMSG_SET_GOTO_STORE_POSITION: /* 0x4024*/
        sync_stepper_steps_to_encoder_counts(info);
        stepper_goto_stored_pos(info, slave_message->param2);
        *p_log_id = STEPPER_SET_GOTO_STORE_POSITION;
        break;

    case MGMSG_MCM_MOT_SET_VELOCITY: /* 0x4053*/
        start_velocity(*info, (bool)slave_message->param1, slave_message->param2);
        *p_log_id = STEPPER_SET_VELOCITY;
        break;

    case mcm_speed_limit::COMMAND_REQ: {
        parser_result<mcm_speed_limit::request_type> maybe = apt_struct_req<mcm_speed_limit>(command);

        if (maybe) {
            cards::stepper::with_response_builder(info->slot, response_buffer, *p_length, [&](drivers::usb::apt_response_builder& builder) {
                apt_struct_get<mcm_speed_limit>(builder, cards::stepper::apt_handler(*maybe, *info));
            });
            *p_need_to_reply = true;
        }
    } break;

    case drivers::apt::mcm_speed_limit::COMMAND_SET: {
        parser_result<mcm_speed_limit::payload_type> maybe = apt_struct_set<mcm_speed_limit>(command);

        if (maybe) {
            cards::stepper::apt_handler(*maybe, *info);
        }
    } break;

    default:
        rt = false;
        break;
    }

    return rt;
}

/**
 * @brief Parse APT commands that are specific to stepper card
 *
 * Takes xUSB_Slave_TxSemaphore - sending data on the USB bus
 *
 * @param info : The stepper info structure.
 * @param slave_message : This is the massage structure pass from the USB salve
 * task
 */
// MARK:  SPI Mutex Required
static void parse(Stepper_info *info, USB_Slave_Message *slave_message,
                  bool active) {
    uint8_t response_buffer[APT_RESPONSE_BUFFER_SIZE] = {0};
    uint8_t length = 0;
    bool need_to_reply = false;
    int32_t temp_32_1, temp_32_2;
    stepper_log_ids log_id = STEPPER_NO_LOG;

    if (active) {
        active = parse_active(info, slave_message, response_buffer, &length,
                              &need_to_reply, &log_id);
    }

    if (!active) {
        switch (slave_message->ucMessageID) {

        case MGMSG_MCM_ERASE_DEVICE_CONFIGURATION: /* 0x4020 */
            // Invalidate the configuration on the slot card identified
            if (slave_message->ExtendedData_len >= 2 &&
                info->slot == slave_message->extended_data_buf[0]) {
                {
                    info->save.config.invalidate();
                }

                device_detect_reset(&slots[info->slot].device);
            }
            break;

        case MGMSG_MCM_REQ_STATUSUPDATE: /* 0x4044 */
            /* Header */
            response_buffer[0] = (uint8_t)MGMSG_MCM_GET_STATUSUPDATE;
            response_buffer[1] = MGMSG_MCM_GET_STATUSUPDATE >> 8;
            response_buffer[2] = 19;
            response_buffer[3] = 0x00;
            response_buffer[4] =
                HOST_ID | 0x80; /* destination & has extended data*/
            response_buffer[5] =
                SLOT_1_ID + static_cast<uint8_t>(info->slot); /* source*/

            /*Chan Ident*/
            response_buffer[6] = info->slot;
            response_buffer[7] = 0x00;

            /*Position*/
            memcpy(&response_buffer[8], &info->counter.step_pos_32bit,
                   sizeof(info->counter.step_pos_32bit));
            /*EncCount*/
            memcpy(&response_buffer[12], &info->enc.enc_pos,
                   sizeof(info->enc.enc_pos));

            /*Status Bits 4 bytes*/
            memcpy(&response_buffer[16], &info->ctrl.status_bits,
                   sizeof(info->ctrl.status_bits));

            /*Current store position, 0 to 10 , if not at any stored position
             * 0xff*/
            memcpy(&response_buffer[20], &info->current_stored_position,
                   sizeof(info->current_stored_position));

            /*EncCountRaw*/
            memcpy(&response_buffer[21], &info->enc.enc_pos_raw,
                   sizeof(info->enc.enc_pos_raw));

            need_to_reply = true;
            length = 6 + 19;
            break;

        case MGMSG_MCM_MOT_SET_LIMSWITCHPARAMS: /* 0x4047*/
            // block changing abs limits because the are set with a but to
            // capture the raw value.
            temp_32_1 = info->save.config.params.limits.abs_high_limit;
            temp_32_2 = info->save.config.params.limits.abs_low_limit;

            memcpy(&info->save.config.params.limits,
                   &slave_message->extended_data_buf[2],
                   sizeof(info->save.config.params.limits));

            {
                auto cw_temp = info->save.config.params.limits.cw_soft_limit;
                auto ccw_temp = info->save.config.params.limits.ccw_soft_limit;
                encoder_position_pair_to_raw_encoder_position_pair(
                    &ccw_temp, &cw_temp,
                    Tst_bits(info->save.config.params.flags.flags,
                             ENCODER_REVERSED));
                info->save.config.params.limits.cw_soft_limit = cw_temp;
                info->save.config.params.limits.ccw_soft_limit = ccw_temp;
            }

            // Return the values back to what was set in ram
            info->save.config.params.limits.abs_high_limit = temp_32_1;
            info->save.config.params.limits.abs_low_limit = temp_32_2;

            // (sbenish) The pointer is maintained over a long period of time.
            // Let this warning exist.
            setup_limits(
                info->slot, &info->limits, &info->enc.enc_pos_raw,
                (bool)Tst_bits(info->save.config.params.flags.flags,
                               ENCODER_REVERSED));

            log_id = STEPPER_SET_LIMSWITCHPARAMS;
            break;

        case MGMSG_MCM_MOT_REQ_LIMSWITCHPARAMS: /* 0x4048*/
            /* Header */
            response_buffer[0] = (uint8_t)MGMSG_MCM_MOT_GET_LIMSWITCHPARAMS;
            response_buffer[1] = MGMSG_MCM_MOT_GET_LIMSWITCHPARAMS >> 8;
            response_buffer[2] = 24;
            response_buffer[3] = 0x00;
            response_buffer[4] =
                HOST_ID | 0x80; /* destination & has extended data*/
            response_buffer[5] =
                SLOT_1_ID + static_cast<uint8_t>(info->slot); /* source*/

            /*Chan Ident*/
            response_buffer[6] = info->slot;
            response_buffer[7] = 0x00;

            /*CW Hardlimit*/
            memcpy(&response_buffer[8],
                   &info->save.config.params.limits.cw_hard_limit, 2);

            /*CCW Hardlimit*/
            memcpy(&response_buffer[10],
                   &info->save.config.params.limits.ccw_hard_limit, 2);

            {
                int32_t soft_limit_cw =
                    info->save.config.params.limits.cw_soft_limit;
                int32_t soft_limit_ccw =
                    info->save.config.params.limits.ccw_soft_limit;
                int32_t abs_high_limit =
                    info->save.config.params.limits.abs_high_limit;
                int32_t abs_low_limit =
                    info->save.config.params.limits.abs_low_limit;

                raw_encoder_position_pair_to_encoder_position_pair(
                    &soft_limit_ccw, &soft_limit_cw,
                    Tst_bits(info->save.config.params.flags.flags,
                             ENCODER_REVERSED));

                raw_encoder_position_pair_to_encoder_position_pair(
                    &abs_low_limit, &abs_high_limit,
                    Tst_bits(info->save.config.params.flags.flags,
                             ENCODER_REVERSED));

                /*CW Soft Limit*/
                memcpy(&response_buffer[12], &soft_limit_cw, 4);

                /*CCW Soft Limit*/
                memcpy(&response_buffer[16], &soft_limit_ccw, 4);

                /*abs hi Limit*/
                memcpy(&response_buffer[20], &abs_high_limit, 4);

                /*abs low Limit*/
                memcpy(&response_buffer[24], &abs_low_limit, 4);
            }

            /*Limit Mode*/
            memcpy(&response_buffer[28],
                   &info->save.config.params.limits.limit_mode, 2);

            need_to_reply = true;
            length = 24 + 6;
            break;

        case MGMSG_MCM_SET_SOFT_LIMITS:
            switch (slave_message->param1) {
            case SET_CCW_SOFT_LIMIT:
                /*Set whatever the encoder counts are to this limit, use enc_pos
                 * not enc_pos_raw because we may have a virtual zero*/
                if ((bool)Tst_bits(info->save.config.params.flags.flags,
                                   ENCODER_REVERSED)) {
                    info->save.config.params.limits.cw_soft_limit =
                        info->enc.enc_pos_raw;
                } else {
                    info->save.config.params.limits.ccw_soft_limit =
                        info->enc.enc_pos_raw;
                }

                break;

            case SET_CW_SOFT_LIMIT:
                /*Set whatever the encoder counts are to this limit, use enc_pos
                 * not enc_pos_raw because we may have a virtual zero*/
                if ((bool)Tst_bits(info->save.config.params.flags.flags,
                                   ENCODER_REVERSED)) {
                    info->save.config.params.limits.ccw_soft_limit =
                        info->enc.enc_pos_raw;
                } else {
                    info->save.config.params.limits.cw_soft_limit =
                        info->enc.enc_pos_raw;
                }

                break;

            case CLEAR_SOFT_LIMITS:
                /*Set both cw and ccw soft limits to 2147483647 & -2147483648
                 * out of range*/
                info->save.config.params.limits.cw_soft_limit = 2147483647;
                info->save.config.params.limits.ccw_soft_limit = -2147483648;

                break;

            default:
                break;
            }

            log_id = STEPPER_SET_SOFT_LIMITS;
            break;

        case MGMSG_MCM_SET_ABS_LIMITS:
            switch (slave_message->param1) {
            case SET_ABS_LOW_LIMIT:
                /*Set whatever the encoder counts are to this limit*/
                info->save.config.params.limits.abs_low_limit =
                    info->enc.enc_pos_raw;

                break;

            case SET_ABS_HIGH_LIMIT:
                /*Set whatever the encoder counts are to this limit*/
                info->save.config.params.limits.abs_high_limit =
                    info->enc.enc_pos_raw;

                break;

            case CLEAR_ABS_LIMITS:
                /*Set both cw and ccw soft limits to 2147483647 & -2147483648
                 * out of range*/
                info->save.config.params.limits.abs_high_limit = 2147483647;
                info->save.config.params.limits.abs_low_limit = -2147483648;

                break;

            default:
                break;
            }
            log_id = STEPPER_SET_ABS_LIMITS;
            break;

        case MGMSG_MCM_SET_HOMEPARAMS:
            memcpy(&info->save.config.params.home,
                   &slave_message->extended_data_buf[2],
                   sizeof(info->save.config.params.home));

            log_id = STEPPER_SET_HOMEPARAMS;
            break;

        case MGMSG_MCM_REQ_HOMEPARAMS:
            /* Header */
            response_buffer[0] = (uint8_t)MGMSG_MCM_GET_HOMEPARAMS;
            response_buffer[1] = MGMSG_MCM_GET_HOMEPARAMS >> 8;
            response_buffer[2] = 14;
            response_buffer[3] = 0x00;
            response_buffer[4] =
                HOST_ID | 0x80; /* destination & has extended data*/
            response_buffer[5] =
                SLOT_1_ID + static_cast<uint8_t>(info->slot); /* source*/

            /*Chan Ident*/
            response_buffer[6] = info->slot;
            response_buffer[7] = 0x00;

            /*Home Mode*/
            memcpy(&response_buffer[8],
                   &info->save.config.params.home.home_mode,
                   sizeof(info->save.config.params.home.home_mode));

            /*Home Dir*/
            memcpy(&response_buffer[9], &info->save.config.params.home.home_dir,
                   sizeof(info->save.config.params.home.home_dir));

            /*Limit Switch*/
            memcpy(&response_buffer[10],
                   &info->save.config.params.home.limit_switch,
                   sizeof(info->save.config.params.home.limit_switch));

            /*Home Velocity*/
            memcpy(&response_buffer[12],
                   &info->save.config.params.home.home_velocity,
                   sizeof(info->save.config.params.home.home_velocity));

            /*Offset Distance*/
            memcpy(&response_buffer[16],
                   &info->save.config.params.home.offset_distance,
                   sizeof(info->save.config.params.home.offset_distance));

            need_to_reply = true;
            length = 20;
            break;

        case MGMSG_MCM_SET_STAGEPARAMS:
            /*0-1 for chan_id*/
            /*Stepper config*/
            memcpy(&info->save.config.params.config.axis_serial_no,
                   &slave_message->extended_data_buf[22], 16);
            memcpy(&info->save.config.params.config.collision_threshold,
                   &slave_message->extended_data_buf[83], 2);

            /*Correct the counts_per_unit*/
            {
                uint32_t temp = slave_message->extended_data_buf[26] +
                                (slave_message->extended_data_buf[27] << 8) +
                                (slave_message->extended_data_buf[28] << 16) +
                                (slave_message->extended_data_buf[29] << 24);
                info->save.config.params.config.counts_per_unit =
                    (float)temp / 100000.0;
            }

            /*Stepper_drive_params*/
            // Acceleration through IC Configuration
            memcpy(&info->save.config.params.drive,
                   &slave_message->extended_data_buf[38], 30);

            // Gate Config 1 through Kickout Time
            memcpy(reinterpret_cast<char *>(&info->save.config.params.drive) +
                       30,
                   &slave_message->extended_data_buf[72], 10);

            /*Encoder_save*/
            memcpy(&info->save.config.params.flags,
                   &slave_message->extended_data_buf[82], 1);

            /*Encoder type*/
            memcpy(&info->save.config.params.encoder,
                   &slave_message->extended_data_buf[85], 5);
            memcpy(&info->save.config.params.encoder.nm_per_count,
                   &slave_message->extended_data_buf[68], 4);
            info->enc.type = info->save.config.params.encoder
                                 .encoder_type; // FIXME why is this information
                                                // captured in two structures?

            hard_hiz_stepper(info->slot);
            //        reset_card(info->slot, 0);
            //        reset_card(info->slot, 1);
            load_params_to_drive(info);

            // (sbenish) The pointer is maintained over a long period of time.
            // Let this warning exist.
            setup_limits(
                info->slot, &info->limits, &info->enc.enc_pos_raw,
                (bool)Tst_bits(info->save.config.params.flags.flags,
                               ENCODER_REVERSED));

            log_id = STEPPER_SET_STAGEPARAMS;
            break;

        case MGMSG_MCM_REQ_STAGEPARAMS:
            /* Header */
            response_buffer[0] = (uint8_t)MGMSG_MCM_GET_STAGEPARAMS;
            response_buffer[1] = MGMSG_MCM_GET_STAGEPARAMS >> 8;
            response_buffer[2] = 90;
            response_buffer[3] = 0x00;
            response_buffer[4] =
                HOST_ID | 0x80; /* destination & has extended data*/
            response_buffer[5] =
                SLOT_1_ID + static_cast<uint8_t>(info->slot); /* source*/

            /*Chan Ident*/
            response_buffer[6] = info->slot;
            response_buffer[7] = 0x00;

            /*0-1 for chan_id*/
            /*Stepper config*/
            {
                const char SLOT_FIELDS[4] = "BAD";
                const char AXIS_FIELD[16] = "DEPRECIATED";
                memcpy(&response_buffer[6 + 2], SLOT_FIELDS, 4);
                memcpy(&response_buffer[6 + 6], AXIS_FIELD, 16);
            }
            memcpy(&response_buffer[6 + 22],
                   &info->save.config.params.config.axis_serial_no, 16);
            memcpy(response_buffer + 6 + 83,
                   &info->save.config.params.config.collision_threshold, 2);

            /*Correct the counts_per_unit*/
            {
                uint32_t counts_per_unit_fixed =
                    (info->save.config.params.config.counts_per_unit *
                     100000.0);
                response_buffer[6 + 26] = counts_per_unit_fixed;
                response_buffer[6 + 27] = counts_per_unit_fixed >> 8;
                response_buffer[6 + 28] = counts_per_unit_fixed >> 16;
                response_buffer[6 + 29] = counts_per_unit_fixed >> 24;
            }

            /*Stepper_drive_params*/
            // Acceleration through IC Configuration
            memcpy(response_buffer + 6 + 38, &info->save.config.params.drive,
                   30);

            // Gate Config 1 through Kickout Time
            memcpy(response_buffer + 6 + 72,
                   reinterpret_cast<char *>(&info->save.config.params.drive) +
                       30,
                   10);

            /*Encoder_save*/
            memcpy(response_buffer + 6 + 82, &info->save.config.params.flags,
                   1);

            /*Encoder type*/
            memcpy(response_buffer + 6 + 85, &info->save.config.params.encoder,
                   5);
            memcpy(response_buffer + 6 + 68,
                   &info->save.config.params.encoder.nm_per_count, 4);

            need_to_reply = true;
            length = 6 + 90;
            break;

        case MGMSG_MOT_SET_JOGPARAMS: /* 0x0416*/
            /* Check slot_number matches slot_id, if they don't return throwing
             * out the message*/
            if (!slot_number_matches_slot_id(
                    slave_message->extended_data_buf[0],
                    slave_message->destination))
                return;

            /*0-1 for chan_id*/


            {
                const uint32_t OLD_MAX_VEL = info->save.config.params.jog.max_vel;
                /*Jog_Params*/
                memcpy(&info->save.config.params.jog,
                       &slave_message->extended_data_buf[2], 20);

                // (sbenish) Compatability option with the old API.
                //           This will allow the previous velocity to be kept.
                if (info->save.config.params.jog.max_vel == 0) {
                    info->save.config.params.jog.max_vel = OLD_MAX_VEL;
                }
            }

            log_id = STEPPER_SET_JOGPARAMS;
            break;

        case MGMSG_MOT_REQ_JOGPARAMS: /* 0x0417*/
            /* Header */
            response_buffer[0] = (uint8_t)MGMSG_MOT_GET_JOGPARAMS;
            response_buffer[1] = MGMSG_MOT_GET_JOGPARAMS >> 8;
            response_buffer[2] = 22;
            response_buffer[3] = 0x00;
            response_buffer[4] =
                HOST_ID | 0x80; /* destination & has extended data*/
            response_buffer[5] =
                SLOT_1_ID + static_cast<uint8_t>(info->slot); /* source*/

            /*Chan Ident*/
            response_buffer[6] = info->slot;
            response_buffer[7] = 0x00;
            memcpy(&response_buffer[6 + 2], &info->save.config.params.jog, 20);

            need_to_reply = true;
            length = 6 + 22;
            break;

        case MGMSG_MOT_SET_DCPIDPARAMS: /*0x04A0*/

            memcpy(&info->save.config.params.pid.Kp,
                   &slave_message->extended_data_buf[2], 4);
            memcpy(&info->save.config.params.pid.Ki,
                   &slave_message->extended_data_buf[6], 4);
            memcpy(&info->save.config.params.pid.Kd,
                   &slave_message->extended_data_buf[10], 4);
            memcpy(&info->save.config.params.pid.imax,
                   &slave_message->extended_data_buf[14], 4);
            memcpy(&info->save.config.params.pid.FilterControl,
                   &slave_message->extended_data_buf[18], 2);

            log_id = STEPPER_SET_DCPIDPARAMS;
            break;

        case MGMSG_MOT_REQ_DCPIDPARAMS: /*0x04A1*/
            /* Header */
            response_buffer[0] = (uint8_t)MGMSG_MOT_GET_DCPIDPARAMS;
            response_buffer[1] = MGMSG_MOT_GET_DCPIDPARAMS >> 8;
            response_buffer[2] = 20;
            response_buffer[3] = 0x00;
            response_buffer[4] =
                HOST_ID | 0x80; /* destination & has extended data*/
            response_buffer[5] =
                SLOT_1_ID + static_cast<uint8_t>(info->slot); /* source*/

            /*Chan Ident*/
            response_buffer[6] = info->slot;
            response_buffer[7] = 0x00;
            memcpy(&response_buffer[6 + 2], &info->save.config.params.pid.Kp,
                   4);
            memcpy(&response_buffer[6 + 6], &info->save.config.params.pid.Ki,
                   4);
            memcpy(&response_buffer[6 + 10], &info->save.config.params.pid.Kd,
                   4);
            memcpy(&response_buffer[6 + 14], &info->save.config.params.pid.imax,
                   4);
            memcpy(&response_buffer[6 + 18],
                   &info->save.config.params.pid.FilterControl, 2);

            need_to_reply = true;
            length = 26;
            break;

        case MGMSG_SET_STORE_POSITION_DEADBAND: /*0x4025*/
            memcpy(&info->save.store.stored_pos_deadband,
                   &slave_message->extended_data_buf[2],
                   sizeof(info->save.store.stored_pos_deadband));

            eeprom_25LC1024_write(SLOT_EEPROM_ADDRESS(info->slot, 1),
                                  sizeof(info->save.store),
                                  (uint8_t *)&info->save.store);

            log_id = STEPPER_SET_STORE_POSITION_DEADBAND;
            break;

        case MGMSG_REQ_STORE_POSITION_DEADBAND: /*0x4026*/
            /* Header */
            response_buffer[0] = (uint8_t)MGMSG_GET_STORE_POSITION_DEADBAND;
            response_buffer[1] = MGMSG_GET_STORE_POSITION_DEADBAND >> 8;
            response_buffer[2] = 4;
            response_buffer[3] = 0x00;
            response_buffer[4] =
                HOST_ID | 0x80; /* destination & has extended data*/
            response_buffer[5] =
                SLOT_1_ID + static_cast<uint8_t>(info->slot); /* source*/

            /*Chan Ident*/
            response_buffer[6] = info->slot;
            response_buffer[7] = 0x00;

            memcpy(&response_buffer[8], &info->save.store.stored_pos_deadband,
                   sizeof(info->save.store.stored_pos_deadband));

            need_to_reply = true;
            length = 6 + 2 + 2;
            break;

        case MGMSG_SET_STORE_POSITION: /*0x4021*/
            memcpy(&info->save.store.stored_pos[0],
                   &slave_message->extended_data_buf[2],
                   sizeof(info->save.store.stored_pos));

            eeprom_25LC1024_write(SLOT_EEPROM_ADDRESS(info->slot, 1),
                                  sizeof(info->save.store),
                                  (uint8_t *)&info->save.store);
            log_id = STEPPER_SET_STORE_POSITION;
            break;

        case MGMSG_REQ_STORE_POSITION: /*0x4022*/
            /* Header */
            response_buffer[0] = (uint8_t)MGMSG_GET_STORE_POSITION;
            response_buffer[1] = MGMSG_GET_STORE_POSITION >> 8;
            response_buffer[2] = 42;
            response_buffer[3] = 0x00;
            response_buffer[4] =
                HOST_ID | 0x80; /* destination & has extended data*/
            response_buffer[5] =
                SLOT_1_ID + static_cast<uint8_t>(info->slot); /* source*/

            /*Chan Ident*/
            response_buffer[6] = info->slot;
            response_buffer[7] = 0x00;

            memcpy(&response_buffer[6 + 2], &info->save.store.stored_pos[0],
                   4 * NUM_OF_STORED_POS);

            need_to_reply = true;
            length = 6 + 2 + (4 * NUM_OF_STORED_POS);
            break;
        case MGMSG_MOT_SET_EEPROMPARAMS: // 0x04B9
        {
            uint16_t command;
            memcpy(&command, &slave_message->extended_data_buf[2],
                   sizeof(command));
            handle_save_command(info, command);
        } break;
        default:
            if (!slot_parse_generic_apt(info->slot, slave_message,
                                        (uint8_t *)&response_buffer, &length,
                                        &need_to_reply)) {
                usb_slave_command_not_supported(slave_message);
                log_id = STEPPER_UNSUPPORTED_COMMAND;
            }
            break;
        }
    }

    if (log_id != STEPPER_NO_LOG)
        setup_and_send_log(slave_message, info->slot, LOG_NO_REPEAT,
                           STEPPER_LOG_TYPE, log_id, info->enc.enc_pos, 0, 0);

    if (need_to_reply) {
        lock_guard(slave_message->xUSB_Slave_TxSemaphore);
        slave_message->write((uint8_t *)response_buffer, length);
    }
}

/**
 * Saves data in RAM into EEPROM depending on the command to save.
 * If the saved parameters were previously imported from the save constructor,
 * then all the parameters will be saved.
 * \param[in]       p_info The stepper structure.
 * \param[in]       command_to_save The command whose results should be saved.
 */
static void handle_save_command(Stepper_info *const p_info,
                                const uint16_t command_to_save) {
    // The SPI semaphore is already gotten when this is called.
    switch (command_to_save) {
    case MGMSG_MCM_MOT_SET_LIMSWITCHPARAMS:
        p_info->save.config.save_limit_params();
        break;
    case MGMSG_MCM_SET_SOFT_LIMITS:
        p_info->save.config.save_soft_limits();
        break;
    case MGMSG_MCM_SET_HOMEPARAMS:
        p_info->save.config.save_home_params();
        break;
    case MGMSG_MCM_SET_STAGEPARAMS:
        p_info->save.config.save_stage_params();
        break;
    case MGMSG_MOT_SET_JOGPARAMS:
        p_info->save.config.save_jog_params();
        break;
    case MGMSG_MOT_SET_DCPIDPARAMS:
        p_info->save.config.save_pid_params();
        break;
    default:
        break;
    }
}

static void service_joystick(Stepper_info *info,
                             service::itc::pipeline_hid_in_t::unique_queue& queue,
                             USB_Slave_Message *slave_message, bool active) {
    float temp = 0;
    stepper_log_ids log_id = STEPPER_NO_LOG;

    service::itc::message::hid_in_message msg;
    while (queue.try_pop(msg, 0)) {
        if (active) {
            switch (msg.usage_page) {
            case HID_USAGE_PAGE_GENERIC_DESKTOP:
                switch (msg.usage_id) {
                case HID_USAGE_X:
                case HID_USAGE_Y:
                case HID_USAGE_Z:
                case HID_USAGE_RX:
                case HID_USAGE_RY:
                case HID_USAGE_RZ:
                case HID_USAGE_WHEEL:
                    /*need to multiple by 1024 to get from max_speed to
                     * speed sent from drive commands*/
                    temp = abs(msg.control_data *
                           stepper_get_speed_channel_value(info, STEPPER_SPEED_CHANNEL_UNBOUND));

                    stepper_set_speed_channel_value(info, STEPPER_SPEED_CHANNEL_JOYSTICK, temp);
                    info->ctrl.current_speed_channel = STEPPER_SPEED_CHANNEL_JOYSTICK;

                    if (msg.control_data == 0) {
                        if (info->ctrl.mode != IDLE)
                            info->ctrl.mode = STOP;
                        log_id = STEPPER_JOYSTICK_AXIS_RUN_STOP;
                    } else {
                        bool is_cw = msg.control_data > 0;
                        info->ctrl.cmd_dir = is_cw ? DIR_FORWARD : DIR_REVERSE;
                        info->ctrl.mode = RUN;
                        log_id = STEPPER_JOYSTICK_AXIS_RUN_CW;
                    }
                    break;

                case HID_USAGE_DIAL:
                    // (sbenish) The USB HID description of a dial is very similar to 
                    // what we consider as wheels.
                    // I recommend treating wheels and dials as the same and add a new
                    // HID control mode to perform a jog compatible with wheels and dials.

                    /*Do a jog each time we get data in the direction sent
                     * from dial*/
                    if (msg.control_data > 0) {
                        info->ctrl.cmd_dir = DIR_FORWARD;
                        log_id = STEPPER_JOYSTICK_DIAL_RUN_CW;
                    } else if (msg.control_data < 0) {
                        info->ctrl.cmd_dir = DIR_REVERSE;
                        log_id = STEPPER_JOYSTICK_AXIS_RUN_CCW;
                    } else /* 0 */ {
                        info->ctrl.mode = STOP;
                        log_id = STEPPER_JOYSTICK_DIAL_RUN_STOP;
                    }

                    info->ctrl.current_speed_channel = STEPPER_SPEED_CHANNEL_UNBOUND;

                    move_stepper(info->slot, info->ctrl.cmd_dir, 20);
                    info->ctrl.cur_vel =
                        1; // just to active the move status bits
                    info->ctrl.cur_dir = info->ctrl.cmd_dir;
                    info->ctrl.mode = JOG;
                    info->ctrl.jog_mode = JOG_WAIT;
                    break;
                }
                break;
            case HID_USAGE_PAGE_BUTTON:
                switch (msg.mode) {
                case CTL_BTN_TOGGLE_DISABLE_DEST:
                    Tgl_bits(info->channel_enable, 1);
                    log_id = STEPPER_JOYSTICK_BTN_TOGGLE_DISABLE_DEST;
                    break;

                case CTL_BTN_DISABLE_DEST:
                    Clr_bits(info->channel_enable, 1);
                    log_id = STEPPER_JOYSTICK_BTN_DISABLE_DEST;
                    break;

                case CTL_BTN_JOG_CW:
                    start_jog(info, true);
                    break;

                case CTL_BTN_JOG_CCW:
                    start_jog(info, false);
                    break;

                case CTL_BTN_HOME:
                    try_start_home(info);
                    break;

                case CTL_BTN_POS_1:
                    stepper_goto_stored_pos(info, 0);
                    log_id = STEPPER_JOYSTICK_BTN_POS_1;
                    break;

                case CTL_BTN_POS_2:
                    stepper_goto_stored_pos(info, 1);
                    log_id = STEPPER_JOYSTICK_BTN_POS_2;
                    break;

                case CTL_BTN_POS_3:
                    stepper_goto_stored_pos(info, 2);
                    log_id = STEPPER_JOYSTICK_BTN_POS_3;
                    break;

                case CTL_BTN_POS_4:
                    stepper_goto_stored_pos(info, 3);
                    log_id = STEPPER_JOYSTICK_BTN_POS_4;
                    break;

                case CTL_BTN_POS_5:
                    stepper_goto_stored_pos(info, 4);
                    log_id = STEPPER_JOYSTICK_BTN_POS_5;
                    break;

                case CTL_BTN_POS_6:
                    stepper_goto_stored_pos(info, 5);
                    log_id = STEPPER_JOYSTICK_BTN_POS_6;
                    break;

                case CTL_BTN_POS_7:
                    stepper_goto_stored_pos(info, 6);
                    log_id = STEPPER_JOYSTICK_BTN_POS_7;
                    break;

                case CTL_BTN_POS_8:
                    stepper_goto_stored_pos(info, 7);
                    log_id = STEPPER_JOYSTICK_BTN_POS_8;
                    break;

                case CTL_BTN_POS_9:
                    stepper_goto_stored_pos(info, 8);
                    log_id = STEPPER_JOYSTICK_BTN_POS_9;
                    break;

                case CTL_BTN_POS_10:
                    stepper_goto_stored_pos(info, 9);
                    log_id = STEPPER_JOYSTICK_BTN_POS_10;
                    break;

                default:
                    return;
                }
                break;
            }

            if (log_id != STEPPER_NO_LOG) {
                setup_and_send_log(slave_message, info->slot, LOG_NO_REPEAT,
                                   STEPPER_LOG_TYPE, log_id,
                                   info->enc.enc_pos, 0, 0);
            }
        }
    }

    /*Send limit data to USB data out queue*/
    service::itc::message::hid_out_message tx =
        service::itc::message::hid_out_message::get_default(info->slot);

    /*set limit if on any limit*/
    tx.on_limit =
        active && ((bool)(info->limits.cw | info->limits.ccw |
                          info->limits.soft_cw | info->limits.soft_ccw));

    /*Set moving if stage is moving*/
    tx.is_moving =
        active && ((bool)Tst_bits(info->ctrl.status_bits, STATUS_CW_MOVE) |
                   (bool)Tst_bits(info->ctrl.status_bits, STATUS_CCW_MOVE));

    /*Stage disable status*/
    tx.is_enabled = active && info->channel_enable;

    /*Stage position status*/
    tx.on_stored_position_bitset = info->current_stored_position == 0xFF
                                 ? 0x00
                                 : (0x1 << info->current_stored_position);

    /*Pass the queue to all the USB devices that are available*/
    service::itc::pipeline_hid_out().broadcast(tx);
}

// MARK:  SPI Mutex Required
static bool service_cdc(Stepper_info *info, USB_Slave_Message *slave_message,
                        bool active, service::itc::pipeline_cdc_t::unique_queue& queue) {
    std::size_t counter = 0;
    while (queue.try_pop(*slave_message)) {
        parse(info, slave_message, active);
        ++counter;
    }
    return counter != 0;
}

// MARK:  SPI Mutex Required
static void service_encoder(Stepper_info *info) {
    info->timer = 0;

    get_position_stepper(info);

    /* Has an encoder*/
    if (Tst_bits(info->save.config.params.flags.flags, HAS_ENCODER) &&
        (info->enc.type > ENCODER_TYPE_NONE)) {
        get_encoder_counts(info->slot, info->save.config.params.flags.flags,
                           info->save.config.params.config.max_pos, &info->enc);
    } else /* No encoder, then copy stepper count to encoder counts with coef*/
    {
        info->enc.enc_pos = info->counter.step_pos /
                            info->save.config.params.config.counts_per_unit;
        info->enc.enc_pos_raw = info->enc.enc_pos;
    }

    /*For rotation encoders, we need to sync the step count to the encoder
     * counts when the encoder wraps around*/
    if ((info->enc.type == ENCODER_TYPE_ABS_MAGNETIC_ROTATION) ||
        (info->enc.type == ENCODER_TYPE_ABS_MAGNETIC_ROTATION_AUTO_HOME) ||
        (info->enc.type == ENCODER_TYPE_ABS_MAGNETIC_ROTATION_LW) ||
        (info->enc.type == ENCODER_TYPE_ABS_MAGNETIC_ROTATION_MANUAL)) {
        if (info->counter.step_pos /
                info->save.config.params.config.counts_per_unit >=
            info->save.config.params.config.max_pos) {
            sync_stepper_steps_to_encoder_counts(info);
        }
    }

    /* For the linear Fagor encoder check the error bit, if it is 1 everything
     * is ok, if it is zero, disable the stage.  If the piezo is connected to
     * this stage, stop it as well*/
    if (info->enc.type == ENCODER_TYPE_ABS_BISS_LINEAR) {
        // The system needs to be power cycled to reset error
        if (!info->enc.error) {
            /* The fagor encoder has errors while booting so we count them and
             * if the exceed a limit then the error exist, this counter should
             * be reset when the error is cleared    by the encoder*/
            info->enc.error_cnt++;
            if (info->enc.error_cnt > FAGOR_ENCODER_ERROR_LIMIT) {
                info->channel_enable = false;
                Set_bits(board.error, 1 << info->slot);
            }
        } else {
            info->enc.error_cnt = 0;
        }
    }
}

/**
 * @brief If a slot synchronized motion (sm) task is running, pass the data
 * to/from the slot queues to the synchronized motion task queues
 * @param info
 * @param sm_rx    pointer to the data being sent to the synchronized motion
 * task queues
 * @param sm_tx pointer to the data being received from the synchronized motion
 * task queues
 */
// static void service_synchronized_motion(Stepper_info *info,
//                                         stepper_sm_Rx_data *sm_rx,
//                                         stepper_sm_Tx_data *sm_tx) {
//     if (xStepper_SM_Rx_Queue[info->slot] != NULL) {
//         /*Copy the data to the queue*/
//         sm_rx->position = info->enc.enc_pos;
//         sm_rx->counts_per_unit =
//             info->save.config.params.config.counts_per_unit;
//         sm_rx->max_speed = stepper_get_speed_channel_value(info, STEPPER_SPEED_CHANNEL_SYNC_MOTION);
//         sm_rx->step_mode = info->save.config.params.drive.step_mode;
//         sm_rx->nm_per_count = info->save.config.params.encoder.nm_per_count;
//
//         /*Send the queue to the stepper sm task but don't wait*/
//         xQueueSend(xStepper_SM_Rx_Queue[info->slot], (void *)sm_rx, 0);
//     }
//     if (xStepper_SM_Tx_Queue[info->slot] != NULL) {
//         /*Receive the queue from the stepper sm task but don't wait*/
//         // TODO (sbenish):  Integrate this into the speed channel system.
//         if (xQueueReceive(xStepper_SM_Tx_Queue[info->slot], sm_tx, 0) ==
//             pdPASS) {
//             info->counter.cmnd_pos = sm_tx->target;
//             info->ctrl.mode = PID;
//             stepper_set_speed_channel_value(info, STEPPER_SPEED_CHANNEL_SYNC_MOTION, sm_tx->max_velocity);
//             info->ctrl.current_speed_channel = STEPPER_SPEED_CHANNEL_SYNC_MOTION;
//         }
//     }
// }

// static void service_piezo_synchronized_motion(Stepper_info *info,
//                                               piezo_sm_Rx_data *psm_rx,
//                                               piezo_sm_Tx_data *psm_tx) {
//     if (xPiezo_SM_Rx_Queue[info->slot] != NULL) {
//         /*Copy the data to the queue*/
//         psm_rx->stepper_moving =
//             (bool)Tst_bits(info->ctrl.status_bits, STATUS_CW_MOVE) |
//             (bool)Tst_bits(info->ctrl.status_bits, STATUS_CCW_MOVE);
//         /*Send the queue to the stepper task but don't wait*/
//         xQueueSend(xPiezo_SM_Rx_Queue[info->slot], (void *)psm_rx, 0);
//     }
//     if (xPiezo_SM_Tx_Queue[info->slot] != NULL) {
//         /*Receive the queue from the stepper sm task but don't wait*/
//         if (xQueueReceive(xPiezo_SM_Tx_Queue[info->slot], psm_tx, 0) ==
//             pdPASS) {
//             if (psm_tx->stepper_hold_pos) {
//                 /*put the stepper in PID mode without kickout save the previous
//                  * mode to return to*/
//                 info->ctrl.prev_flags = info->save.config.params.flags.flags;
//
//                 info->ctrl.mode = PID;
//                 Set_bits(info->save.config.params.flags.flags, PID_KICKOUT);
//             } else {
//                 // return values before the piezo sync
//                 info->ctrl.mode = STOP;
//                 info->save.config.params.flags.flags = info->ctrl.prev_flags;
//             }
//         }
//     }
// }

static bool try_start_home(Stepper_info* info) {
    if (info->save.config.params.limits.ccw_soft_limit != CCW_UNSET ||
        info->save.config.params.limits.cw_soft_limit != CW_UNSET) {
        return false;
    }
    info->ctrl.current_speed_channel = STEPPER_SPEED_CHANNEL_HOMING;
    info->ctrl.mode = HOMING;
    info->ctrl.homing_mode = HOME_START;
    info->enc.homed = HOMING_STATUS;
    info->limits.homed = HOMING_STATUS;
    return true;
}

static void start_jog(Stepper_info* const info, bool ccw_cw_flag) {
    info->ctrl.current_speed_channel = STEPPER_SPEED_CHANNEL_JOG;
    info->ctrl.cmd_dir = ccw_cw_flag;
    /*Use PID controller for goto*/
    if (Tst_bits(info->save.config.params.flags.flags, USE_PID)) {
        /*Calculate the goto position from the jog distance*/
        if (info->ctrl.cmd_dir == DIR_FORWARD) // CW = 1
        {
            info->counter.cmnd_pos =
                info->enc.enc_pos + info->save.config.params.jog.step_size;
        } else // DIR_REVERSE 0 // CCW
        {
            info->counter.cmnd_pos =
                info->enc.enc_pos - info->save.config.params.jog.step_size;
        }

        info->pid.max_velocity = 1;
        info->ctrl.mode = PID;
    } else /*Used normal jog*/
    {
        if (info->ctrl.mode != IDLE)
            info->ctrl.jog_mode = JOG_CLEAR;
        else
            info->ctrl.jog_mode = JOG_START;

        info->ctrl.mode = JOG;
    }
};

static void start_velocity_simple(Stepper_info& info, bool flag_reverse_forward) {
    // (sbenish) Reversal needs to be done here b/c stepper_control cannot
    // perform reversal
    // Specifically, the stepper_control.cc uses the "configure_movement" function
    // to perform reversal.
    // Configuration was done each loop, stepper reversal would just vibrate the stage.
    // Doing it here prevents this problem.
    if (info.save.config.params.flags.flags & STEPPER_REVERSED) {
        flag_reverse_forward = !flag_reverse_forward;
    }
    info.ctrl.cmd_dir = flag_reverse_forward;

    info.ctrl.mode = RUN;
    info.ctrl.current_speed_channel = STEPPER_SPEED_CHANNEL_VELOCITY;
}

static void start_velocity(Stepper_info& info, bool flag_reverse_forward, uint32_t speed) {
    if (speed == 0) {
        info.ctrl.mode = STOP;
    } else {
        stepper_set_speed_channel_value(
            &info,
            STEPPER_SPEED_CHANNEL_VELOCITY,
            (stepper_get_speed_channel_value(&info, STEPPER_SPEED_CHANNEL_UNBOUND) * speed) / 100);

        start_velocity_simple(info, flag_reverse_forward);
    }
}

static void reset_speed_channels(Stepper_info* p_info) {
    const uint32_t UNBOUNDED_VALUE = stepper_get_speed_channel_value(p_info, STEPPER_SPEED_CHANNEL_UNBOUND);
    stepper_set_speed_channel_value(p_info, STEPPER_SPEED_CHANNEL_ABSOLUTE, UNBOUNDED_VALUE);
    stepper_set_speed_channel_value(p_info, STEPPER_SPEED_CHANNEL_RELATIVE, UNBOUNDED_VALUE);
    stepper_set_speed_channel_value(p_info, STEPPER_SPEED_CHANNEL_VELOCITY, UNBOUNDED_VALUE);

    // (sbenish) This is a compatability option where old JOG params
    // would store the speed as 0.
    if (stepper_get_speed_channel_value(p_info, STEPPER_SPEED_CHANNEL_JOG) == 0) {
        stepper_set_speed_channel_value(p_info, STEPPER_SPEED_CHANNEL_JOG, UNBOUNDED_VALUE);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
/**
 * @brief Get the position value from the stepper drive, this is in 2's
 * complement
 * @param info
 */
// MARK:  SPI Mutex Required
extern "C++" void get_el_pos_stepper(Stepper_info *info) {

    uint8_t spi_tx_data[3];
    get_parameter(info->slot, (EL_POS | 0x20), 2, spi_tx_data);

    info->counter.el_pos = ((spi_tx_data[1] << 8) + (spi_tx_data[2]));
}

/**
 * Use this function to set a register parameter in the stepper drive.
 * @param slot
 * @param parameter
 * @param length
 * @param val
 */
// MARK:  SPI Mutex Required
void set_parameter(uint8_t slot, uint8_t parameter, uint8_t length,
                   uint32_t val) {

    uint8_t spi_tx_data[length + 1];

    spi_tx_data[0] = parameter;

    for (int i = 1; i < length + 1; i++) {
        spi_tx_data[i] = val >> ((length - i) * 8);
    }

    spi_transfer(SPI_ST_STEPPER_NO_READ, spi_tx_data, length + 1);
}

// MARK:  SPI Mutex Required
void get_parameter(uint8_t slot, uint8_t parameter, uint8_t length,
                   uint8_t *spi_tx_data) {
#if 0
    uint8_t spi_tx_data2[length + 1];

    spi_tx_data2[0] = parameter;

    for (int i = 1; i < length + 1; i++)
    {
        spi_tx_data2[i] = DUMMY_LOW;
    }

    spi_transfer(SPI_ST_STEPPER_READ, spi_tx_data2, length + 1);

#else
    spi_tx_data[0] = parameter;

    for (int i = 1; i < length + 1; i++) {
        spi_tx_data[i] = DUMMY_LOW;
    }

    spi_transfer(SPI_ST_STEPPER_READ, spi_tx_data, length + 1);
#endif
}

/**
 * @brief The Run command produces a motion at SPD speed; the direction is
 selected by the DIR bit: '1' forward or '0' reverse. The SPD value is expressed
 in step/tick.
 * @param slot
 * @param direction
 * @param speed
 */
// MARK:  SPI Mutex Required
void run_stepper(uint8_t slot, uint8_t direction, uint32_t speed) {
    set_parameter(slot, RUN_STEPPER + direction, 3, speed);
}

/**
 * @brief The StepClock command switches the device in Step-clock mode and
 * imposes the forward (DIR = '1') or reverse (DIR = '0') direction. When the
 * device is in Step-clock mode, the SCK_MOD flag in the STATUS register is
 * raised and the motor is always considered stopped. The device exits
 * Step-clock mode when a constant speed, absolute positioning or motion command
 * is sent through SPI. Motion direction is imposed by the respective StepClock
 * command argument and can by changed by a new StepClock command without
 * exiting Step-clock mode.
 * @param slot
 * @param direction
 */
// MARK:  SPI Mutex Required
void step_clock_stepper(uint8_t slot, bool direction) {
    set_parameter(slot, STEP_CLOCK_STEPPER + direction, 0, 0);
}

/**
 * @brief The move command produces a motion of N_STEP microsteps; the direction
 * is selected by the DIR bit ('1' forward or '0' reverse).
 *
 * The N_STEP value is always in agreement with the selected step mode; the
 * parameter value unit is equal to the selected step mode (full, half, quarter,
 * etc.).
 *
 * This command keeps the BUSY flag low until the target number of steps is
 * performed. This command can only be performed when the motor is stopped. If a
 * motion is in progress the motor must be stopped and it is then possible to
 * perform a move command.
 * @param slot
 * @param direction
 * @param n_step
 */
// MARK:  SPI Mutex Required
void move_stepper(uint8_t slot, bool direction, uint32_t n_step) {
    set_parameter(slot, MOVE_STEPPER + direction, 3, n_step);
}

/**
 * @brief The GoTo command produces a motion to ABS_POS absolute position
 * through the shortest path. The ABS_POS value is always in agreement with the
 * selected step mode; the parameter value unit is equal to the selected step
 * mode (full, half, quarter, etc.).
 *
 * The GoTo command keeps the BUSY flag low until the target position is
 * reached.
 * @param slot
 * @param abs_pos
 */
// MARK:  SPI Mutex Required
void goto_stepper(uint8_t slot, uint32_t abs_pos) {
    set_parameter(slot, GOTO_STEPPER, 3, abs_pos);
}

/**
 * @brief The GoTo_DIR command produces a motion to ABS_POS absolute position
 * imposing a forward (DIR = '1') or a reverse (DIR = '0') rotation. The ABS_POS
 * value is always in agreement with the selected step mode; the parameter value
 * unit is equal to the selected step mode (full, half, quarter, etc.).
 *
 * The GoTo_DIR command keeps the BUSY flag low until the target speed is
 * reached. This command can be given only when the previous motion command has
 * been completed (BUSY flag released).
 * @param slot
 * @param direction
 * @param abs_pos
 */
// MARK:  SPI Mutex Required
void goto_dir_stepper(uint8_t slot, bool direction, uint32_t abs_pos) {
    set_parameter(slot, GOTO_DIR_STEPPER + direction, 3, abs_pos);
}

/**
 * @brief The GoUntil command produces a motion at SPD speed imposing a forward
 * (DIR = '1') or a reverse (DIR = '0') direction. When an external switch
 * turn-on event occurs , the ABS_POS register is reset (if ACT = '0') or the
 * ABS_POS register value is copied into the MARK register (if ACT = '1'); the
 * system then performs a SoftStop command.
 *
 * The SPD value is expressed in step/tick (format unsigned fixed point 0.28)
 * that is the same format as the SPEED register.
 *
 * The SPD value should be lower than MAX_SPEED and greater than MIN_SPEED,
 * otherwise the target speed is imposed at MAX_SPEED or MIN_SPEED respectively.
 *
 * If the SW_MODE bit of the CONFIG register is set low, the external switch
 * turn-on event causes a HardStop interrupt instead of the SoftStop one
 * @param slot
 * @param act
 * @param direction
 * @param abs_pos
 */
// MARK:  SPI Mutex Required
void goto_until_stepper(uint8_t slot, bool act, bool direction,
                        uint32_t abs_pos) {
    set_parameter(slot, GO_UNTIL_STEPPER + (act << 3) + direction, 3, abs_pos);
}

/**
 * @brief The GoHome command produces a motion to the HOME position (zero
 * position) via the shortest path.
 *
 * Note that, this command is equivalent to the �GoTo(0�0)� command. If a motor
 * direction is mandatory, the GoTo_DIR command must be used.
 *
 * The GoHome command keeps the BUSY flag low until the home position is
 * reached. This command can be given only when the previous motion command has
 * been completed. Any attempt to perform a GoHome command when a previous
 * command is under execution (BUSY low) causes the command to be ignored and
 * the CMD_ERROR to rise.
 * @param slot
 */
// MARK:  SPI Mutex Required
void goto_home_stepper(uint8_t slot) {
    set_parameter(slot, GO_HOME_STEPPER, 0, 0);
}

/**
 * @brief The GoMark command produces a motion to the MARK position performing
 * the minimum path. Note that, this command is equivalent to the �GoTo (MARK)�
 * command. If a motor direction is mandatory, the GoTo_DIR command must be
 * used.
 *
 * The GoMark command keeps the BUSY flag low until the MARK position is
 * reached. This command can be given only when the previous motion command has
 * been completed (BUSY flag released).
 * @param slot
 */
// MARK:  SPI Mutex Required
void goto_mark_stepper(uint8_t slot) {
    set_parameter(slot, GO_MARK_STEPPER, 0, 0);
}

/**
 * @brief The ResetPos command resets the ABS_POS register to zero. The zero
 * position is also defined as the HOME position.
 * @param slot
 */
// MARK:  SPI Mutex Required
void reset_pos_stepper(uint8_t slot) {
    set_parameter(slot, RESET_POS_STEPPER, 0, 0);
}

/**
 * @brief The ResetDevice command resets the device to power-up conditions. The
 * command can be performed only when the device is in high impedance state.
 * @param slot
 */
// MARK:  SPI Mutex Required
void reset_device_stepper(uint8_t slot) {
    set_parameter(slot, RESET_DEVICE_STEPPER, 0, 0);
}

/**
 * @brief The SoftStop command causes an immediate deceleration to zero speed
 * and a consequent motor stop; the deceleration value used is the one stored in
 * the DEC register.
 *
 * When the motor is in high impedance state, a SoftStop command forces the
 * bridges to exit from high impedance state; no motion is performed.
 *
 * This command can be given anytime and is immediately executed. This command
 * keeps the BUSY flag low until the motor is stopped.
 * @param slot
 */
// MARK:  SPI Mutex Required
void soft_stop_stepper(uint8_t slot) {
    set_parameter(slot, SOFT_STOP_STEPPER, 0, 0);
}

/**
 * @brief The HardStop command causes an immediate motor stop with infinite
 * deceleration.
 *
 * When the motor is in high impedance state, a HardStop command forces the
 * bridges to exit high impedance state; no motion is performed.
 *
 * This command can be given anytime and is immediately executed. This command
 * keeps the BUSY flag low until the motor is stopped.
 * @param slot
 */
// MARK:  SPI Mutex Required
void hard_stop_stepper(uint8_t slot) {
    set_parameter(slot, HARD_STOP_STEPPER, 0, 0);
}

/**
 * @brief The SoftHiZ command disables the power bridges (high impedance state)
 * after a deceleration to zero; the deceleration value used is the one stored
 * in the DEC register. When bridges are disabled, the HiZ flag is raised.
 *
 * When the motor is stopped, a SoftHiZ command forces the bridges to enter high
 * impedance state.
 *
 * This command can be given anytime and is immediately executed. This command
 * keeps the BUSY flag low until the motor is stopped.
 * @param slot
 */
// MARK:  SPI Mutex Required
void soft_hiz_stepper(uint8_t slot) {
    set_parameter(slot, SOFT_HiZ_STEPPER, 0, 0);
}

/**
 * @brief The HardHiZ command immediately disables the power bridges (high
 * impedance state) and raises the HiZ flag.
 *
 * When the motor is stopped, a HardHiZ command forces the bridges to enter high
 * impedance state.
 *
 * This command can be given anytime and is immediately executed.
 *
 * This command keeps the BUSY flag low until the motor is stopped.
 * @param slot
 */
// MARK:  SPI Mutex Required
void hard_hiz_stepper(uint8_t slot) {
    set_parameter(slot, SOFT_HiZ_STEPPER, 0, 0);
}

/**
 * @brief The GetStatus command returns the Status register value. The GetStatus
 * command resets the STATUS register warning flags. The command forces the
 * system to exit from any error state. The GetStatus command DOES NOT reset the
 * HiZ flag.
 *
 * Bit         15              14          13      12 & 11        10           9
 * 8 STEP_LOSS_B     STEP_LOSS_A     OCD     TH_STATUS     UVLO_ADC     UVLO
 * STCK_MOD
 *
 *     Bit        7                 6 & 5           4        3              2 1
 * 0 CMD_ERROR         MOT_STATUS         DIR     SW_EVN          SW_F BUSY HiZ
 *
 * @param slot
 * @return
 */
// MARK:  SPI Mutex Required
uint16_t get_status_stepper(uint8_t slot) {
    uint8_t spi_tx_data[3];
    get_parameter(slot, GET_STATUS_STEPPER, 2, spi_tx_data);
    //    debug_print("Status %d/r/n",((spi_tx_data[1] << 8) + spi_tx_data[2]));

    return ((spi_tx_data[1] << 8) + spi_tx_data[2]);
}

/**
 * @brief Sync the stepper steps to the equivalent encoder counts
 * @param slot_num
 */
// MARK:  SPI Mutex Required
extern "C++" void sync_stepper_steps_to_encoder_counts(Stepper_info *info) {
    int32_t temp = 0x3FFFFF;

    // set the step_pos_32bit pos to the equivalent encoder counts
    info->counter.step_pos_32bit =
        info->enc.enc_pos * info->save.config.params.config.counts_per_unit;

    if (info->counter.step_pos_32bit < 0) {
        temp |= 0x400000;
    }

    // set the 22 bit set_pos
    info->counter.step_pos = info->counter.step_pos_32bit & temp;
    set_position_stepper(info);
}

extern "C++" bool stepper_busy(Stepper_info *info) {
    /* read the busy status of the L6470 or L6480 low means device is busy*/
    switch ((uint8_t)info->slot) {
    case SLOT_1:
        info->ctrl.busy = get_pin_level(PIN_SMIO_1);
        break;
    case SLOT_2:
        info->ctrl.busy = get_pin_level(PIN_SMIO_2);
        break;
    case SLOT_3:
        info->ctrl.busy = get_pin_level(PIN_SMIO_3);
        break;
    case SLOT_4:
        info->ctrl.busy = get_pin_level(PIN_SMIO_4);
        break;
    case SLOT_5:
        info->ctrl.busy = get_pin_level(PIN_SMIO_5);
        break;
    case SLOT_6:
        info->ctrl.busy = get_pin_level(PIN_SMIO_6);
        break;
    case SLOT_7:
        info->ctrl.busy = get_pin_level(PIN_SMIO_7);
        break;
    }

    if (info->ctrl.busy) /* not busy*/
        return 0;
    else
        /* busy*/
        return 1;
}

/**
 * @brief Check to see if a limit violation exist.
 * @param info : pointer to stepper structure
 * @return : true if there is a violation and sets stepper control to STOP
 */
// MARK:  SPI Mutex Required
extern "C++" bool check_run_limits(Stepper_info *info) {
    bool ret_val = false;
    bool temp_cmd_dir = info->ctrl.cmd_dir;

    if (info->save.config.params.flags.flags & STEPPER_REVERSED) {
        if (temp_cmd_dir == DIR_FORWARD)
            temp_cmd_dir = DIR_REVERSE;
        else
            temp_cmd_dir = DIR_FORWARD;
    }

    if ((info->limits.cw || info->limits.soft_cw) && temp_cmd_dir) {
        hard_stop_stepper(info->slot);
        info->ctrl.mode = STOP;
        ret_val = true;
    }
    if ((info->limits.ccw || info->limits.soft_ccw) && !temp_cmd_dir) {
        hard_stop_stepper(info->slot);
        info->ctrl.mode = STOP;
        ret_val = true;
    }

    /*Only check index if we are homing and if stage has an index and we are
     * homing to index*/
    if (info->ctrl.mode == HOMING) {
        if (info->save.config.params.home.limit_switch == HOME_TO_INDEX) {
            if (Tst_bits(info->save.config.params.flags.flags,
                         ENCODER_HAS_INDEX)) {
                if (info->limits.index) {
                    hard_stop_stepper(info->slot);
                    info->ctrl.mode = STOP;
                    ret_val = true;
                }
            }
        }
    }

    if (info->limits.hard_stop) {
        info->limits.hard_stop = false;
        hard_stop_stepper(info->slot);
        if (info->ctrl.mode != HOMING) {
            info->ctrl.mode = STOP;
        }
        ret_val = true;
    }

    /*Clear the index*/
    info->limits.index = false;

    // update logging flag
    if (!ret_val)
        info->limits.limit_flag_for_logging = false;
    else
        info->limits.limit_flag_for_logging = true;

    return ret_val;
}

/**
 * @brief Check to see if the start of a  goto move is in a limit violation.
 * @param info : pointer to stepper structure
 * @return : true if there is a violation and sets stepper control to idle
 */
extern "C++" bool check_goto_limits(Stepper_info *info) {
    if (((info->limits.cw || info->limits.soft_cw) &&
         (info->counter.cmnd_pos > info->enc.enc_pos)) ||
        ((info->limits.ccw || info->limits.soft_ccw) &&
         (info->counter.cmnd_pos < info->enc.enc_pos))) {
        info->ctrl.mode = STOP;
        info->ctrl.goto_mode = GOTO_IDLE;
        return true;
    } else
        return false;
}

extern "C++" void update_status_bits(Stepper_info *info) {
    /*Reset status bits*/
    info->ctrl.status_bits = 0;

    if (info->limits.cw)
        Set_bits(info->ctrl.status_bits, STATUS_CW_LIMIT);

    if (info->limits.ccw)
        Set_bits(info->ctrl.status_bits, STATUS_CCW_LIMIT);

    if (info->limits.soft_cw)
        Set_bits(info->ctrl.status_bits, STATUS_CW_SOFT_LIMIT);

    if (info->limits.soft_ccw)
        Set_bits(info->ctrl.status_bits, STATUS_CCW_SOFT_LIMIT);

    /* if stepper counting is reversed reverse status indicators*/
    if (info->save.config.params.flags.flags & STEPPER_REVERSED) {
        if (info->ctrl.cur_vel > 0 && info->ctrl.cur_dir)
            Set_bits(info->ctrl.status_bits, STATUS_CCW_MOVE);

        if (info->ctrl.cur_vel > 0 && !info->ctrl.cur_dir)
            Set_bits(info->ctrl.status_bits, STATUS_CW_MOVE);
    } else {
        if (info->ctrl.cur_vel > 0 && info->ctrl.cur_dir)
            Set_bits(info->ctrl.status_bits, STATUS_CW_MOVE);

        if (info->ctrl.cur_vel > 0 && !info->ctrl.cur_dir)
            Set_bits(info->ctrl.status_bits, STATUS_CCW_MOVE);
    }

    if (xGatePass(slots[info->slot].device.connection_gate, 0) == pdPASS)
        Set_bits(info->ctrl.status_bits, STATUS_MOTOR_CONN);
    else
        Clr_bits(info->ctrl.status_bits, STATUS_MOTOR_CONN);

    if (info->ctrl.mode == HOMING)
        Set_bits(info->ctrl.status_bits, STATUS_HOMING);

    if (info->ctrl.selected_stage == true)
        Set_bits(info->ctrl.status_bits, STATUS_SHUTTLE_ACTIVE);

    if (info->limits.homed == IS_HOMED_STATUS)
        Set_bits(info->ctrl.status_bits, STATUS_HOMED);

    if (info->limits.interlock)
        Set_bits(info->ctrl.status_bits, STATUS_INTERLOCK);
    else
        Clr_bits(info->ctrl.status_bits, STATUS_INTERLOCK);

    if (info->channel_enable == true)
        Set_bits(info->ctrl.status_bits, STATUS_CHAN_EN);

    /*Magnetic rotational encoder*/
    if (info->enc.mhi)
        Set_bits(info->ctrl.status_bits, STATUS_ENC_HI);
    else
        Clr_bits(info->ctrl.status_bits, STATUS_ENC_HI);

    if (info->enc.mlo)
        Set_bits(info->ctrl.status_bits, STATUS_ENC_LOW);
    else
        Clr_bits(info->ctrl.status_bits, STATUS_ENC_LOW);

    if (!info->enc.m_ready)
        Set_bits(info->ctrl.status_bits, STATUS_ENC_READY);
    else
        Clr_bits(info->ctrl.status_bits, STATUS_ENC_READY);
}

extern "C++" uint32_t stepper_get_speed_channel_value(const Stepper_info* info, stepper_speed_channel_e channel) {
    switch(channel) {
        case stepper_speed_channel_e::STEPPER_SPEED_CHANNEL_UNBOUND:
        default:
            return info->save.config.params.drive.max_speed;

        case stepper_speed_channel_e::STEPPER_SPEED_CHANNEL_ABSOLUTE:
            return info->ctrl.speed_control.absolute;
            
        case stepper_speed_channel_e::STEPPER_SPEED_CHANNEL_RELATIVE:
            return info->ctrl.speed_control.relative;

        case stepper_speed_channel_e::STEPPER_SPEED_CHANNEL_JOG:
            return info->save.config.params.jog.max_vel;

        case stepper_speed_channel_e::STEPPER_SPEED_CHANNEL_VELOCITY:
            return info->ctrl.speed_control.velocity;

        case stepper_speed_channel_e::STEPPER_SPEED_CHANNEL_HOMING:
            return (info->save.config.params.home.home_velocity *
                stepper_get_speed_channel_value(info, STEPPER_SPEED_CHANNEL_UNBOUND)) / 100;

        case stepper_speed_channel_e::STEPPER_SPEED_CHANNEL_JOYSTICK:
            return info->ctrl.speed_control.joystick;

        case stepper_speed_channel_e::STEPPER_SPEED_CHANNEL_SYNC_MOTION:
            return info->ctrl.speed_control.sync_motion;
    }
}

extern "C++" void stepper_set_speed_channel_value(Stepper_info *info, stepper_speed_channel_e channel, uint32_t value) {
    switch(channel) {
        case stepper_speed_channel_e::STEPPER_SPEED_CHANNEL_UNBOUND:
        default:
            // (sbenish) Not permitted.
            break;

        case stepper_speed_channel_e::STEPPER_SPEED_CHANNEL_ABSOLUTE:
            info->ctrl.speed_control.absolute = value;
            break;
            
        case stepper_speed_channel_e::STEPPER_SPEED_CHANNEL_RELATIVE:
            info->ctrl.speed_control.relative = value;
            break;

        case stepper_speed_channel_e::STEPPER_SPEED_CHANNEL_JOG:
            info->save.config.params.jog.max_vel = value;
            break;

        case stepper_speed_channel_e::STEPPER_SPEED_CHANNEL_VELOCITY:
            info->ctrl.speed_control.velocity = value;
            break;

        case stepper_speed_channel_e::STEPPER_SPEED_CHANNEL_HOMING:
            info->save.config.params.home.home_velocity = (value * 100)
                / stepper_get_speed_channel_value(info, STEPPER_SPEED_CHANNEL_UNBOUND);
            break;

        case stepper_speed_channel_e::STEPPER_SPEED_CHANNEL_JOYSTICK:
            info->ctrl.speed_control.joystick = value;
            break;

        case stepper_speed_channel_e::STEPPER_SPEED_CHANNEL_SYNC_MOTION:
            info->ctrl.speed_control.sync_motion = value;
            break;
    }
}

/// \note Requires the xSPI_Semaphore to be taken.
void enable_stepper_card(Stepper_info *p_info) {
    xSemaphoreTake(slots[p_info->slot].xSlot_Mutex, portMAX_DELAY);
    const slot_types st = slots[p_info->slot].card_type;
    xSemaphoreGive(slots[p_info->slot].xSlot_Mutex);

    switch (st) {
    case ST_Stepper_type:
    case ST_HC_Stepper_type:
    case High_Current_Stepper_Card_HD:
    case MCM_Stepper_L6470_MicroDB15:
    case MCM_Stepper_LC_HD_DB15:
        set_reg(C_SET_ENABLE_STEPPER_CARD, p_info->slot, 0, ENABLED_MODE);
        break;

    case ST_Invert_Stepper_BISS_type:
    case MCM_Stepper_Internal_BISS_L6470:
        set_reg(C_SET_ENABLE_STEPPER_CARD, p_info->slot, 0, STEP_MODE_ENC_BISS);
        break;

    case ST_Invert_Stepper_SSI_type:
    case MCM_Stepper_Internal_SSI_L6470:
        set_reg(C_SET_ENABLE_STEPPER_CARD, p_info->slot, 0, STEP_MODE_MAG);
        break;
    default:
        /// \todo May want to warn of this error.
        break;
    }
}

Stepper_info::Stepper_info(const slot_nums slot)
    : slot(slot), save(SLOT_EEPROM_ADDRESS(slot, 0)) {}

/****************************************************************************
 * Task
 ****************************************************************************/
static void task_stepper(Stepper_info *const p_info) {
    // BEGIN    Task Initialization

    /*Wait here until power is good*/
    while (board.power_good == POWER_NOT_GOOD) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    auto cdc_queue = *service::itc::pipeline_cdc().create_queue(
        static_cast<asf_destination_ids>((int)p_info->slot + (int)SLOT_1_ID));

    // Create the structure for holding the USB data received from the USB slave
    // task
    USB_Slave_Message slave_message;

    // Create the structure for holding the USB data received from the USB host
    // Create the structure for holding the slot synchronized motion info
    // stepper_sm_Rx_data sm_rx;
    // stepper_sm_Tx_data sm_tx;

    // Create the structure for holding the slot piezo synchronized motion info
    // piezo_sm_Rx_data psm_rx;
    // piezo_sm_Tx_data psm_tx;

    TickType_t xLastWakeTime;
    static const TickType_t xFrequency = STEPPER_UPDATE_INTERVAL;

    bool last_enabled = true;

    // Read saved data for this stage.
    // This needs to be done at startup.
    {
        lock_guard lg(xSPI_Semaphore);
        read_eeprom_store(p_info);
    }

    service::itc::pipeline_hid_in_t::unique_queue hid_in_queue =
        std::move(*service::itc::pipeline_hid_in().create_queue(p_info->slot));

    // Create and assign stepper watchdog to supervisor.
    heartbeat_watchdog watchdog(STEPPER_HEARTBEAT_INTERVAL);
    //
    // supervisor_add_watchdog(&watchdog);

    // END      Task Initialization
    for (;;) {
        xLastWakeTime = xTaskGetTickCount();
        // Communicate until the gate is opened (valid device is connected).
        while (xGatePass(slots[p_info->slot].device.connection_gate, 0) ==
               pdFAIL) {
            // Perform routine tasks.
            {
                lock_guard lg(xSPI_Semaphore);
                update_status_bits(p_info);

                service_cdc(p_info, &slave_message, false, cdc_queue);
                service_joystick(p_info, hid_in_queue, &slave_message, false);
            }

            watchdog.beat();
            vTaskDelayUntil(&xLastWakeTime, xFrequency);
        }

        // BEGIN    Device Initalization

        watchdog.set_heartbeat_interval(STEPPER_CONFIGURING_INTERVAL);
        watchdog.beat();

        // Initialize variables not saved in the EEPROM.
        set_vars(p_info);

        // Status flags.
        pnp_status_t status_flags = pnp_status::NO_ERRORS;
        StepperSavedConfigs &r_cfg = p_info->save.config;

        // Check to see if the saved data is valid.
        uint64_t SN;
        {
            lock_guard(slots[p_info->slot].xSlot_Mutex);
            SN = slots[p_info->slot].device.info.serial_num;

            // Disallowing device detection automatically assumes that the slot
            // is configured.
            if (!slots[p_info->slot].save.allow_device_detection) {
                // (sbenish)  When device detection is disabled, the serial number is wildcarded
                // in terms of configuration.
                // This allows systems without device detection to rely on what is stored in memory.
                SN = OW_SERIAL_NUMBER_WILDCARD;

                // Need to load in data if it is not memory mapped.
                if (!r_cfg.is_mapped()) {
                    lock_guard spi(xSPI_Semaphore);
                    r_cfg.load();
                }

                // Need to mark configured after because loading the memory
                // mapper invalidates.
                p_info->save.config.mark_configured(SN);
            }
        }

        if (!r_cfg.is_params_configured(SN)) {
            // If the configuration is not memory-mapped, load in the memory.
            if (!r_cfg.is_mapped()) {
                lock_guard spi(xSPI_Semaphore);
                r_cfg.load();
            }

            // Check to see that the data in EEPROM is not corrupt.
            if (r_cfg.is_save_valid(SN)) {
                r_cfg.mark_configured(SN);
            }
        }

        if (!r_cfg.is_params_configured(SN)) {
            // If the parameters are still not configured, then let's load from
            // the LUT.
            Device *const p_device = &slots[p_info->slot].device;

            bool device_configured = false;

            // Get configs from the device lut.
            Stepper_Save_Union cfgs;
            lut_manager::device_lut_key_t key;

            {
                lock_guard lg(slots[p_info->slot].xSlot_Mutex);
                key.running_slot_card = slots[p_info->slot].card_type;
                key.connected_device =
                    slots[p_info->slot].device.info.signature;
            }

            LUT_ERROR err;
            if (p_device->p_slot_config != nullptr) {
                // The custom config is available, so use it.
                memcpy(&cfgs.array, p_device->p_slot_config, sizeof(cfgs));
                err = LUT_ERROR::OKAY;
            } else {
                err =
                    lut_manager::load_data(LUT_ID::DEVICE_LUT, &key, &cfgs.cfg);
            }

            bool order_valid =
                cfgs.cfg.enc_save.struct_id ==
                    static_cast<struct_id_t>(Struct_ID::ENCODER) &&
                cfgs.cfg.flags_save.struct_id ==
                    static_cast<struct_id_t>(Struct_ID::STEPPER_FLAGS) &&
                cfgs.cfg.home_params.struct_id ==
                    static_cast<struct_id_t>(Struct_ID::STEPPER_HOME) &&
                cfgs.cfg.jog_params.struct_id ==
                    static_cast<struct_id_t>(Struct_ID::STEPPER_JOG) &&
                cfgs.cfg.limits_save.struct_id ==
                    static_cast<struct_id_t>(Struct_ID::LIMIT) &&
                cfgs.cfg.pid_save.struct_id ==
                    static_cast<struct_id_t>(Struct_ID::STEPPER_PID) &&
                cfgs.cfg.stepper_config.struct_id ==
                    static_cast<struct_id_t>(Struct_ID::STEPPER_CONFIG) &&
                cfgs.cfg.stepper_drive_params.struct_id ==
                    static_cast<struct_id_t>(Struct_ID::STEPPER_DRIVE) &&
                cfgs.cfg.stepper_store.struct_id ==
                    static_cast<struct_id_t>(Struct_ID::STEPPER_STORE);
            if (err == LUT_ERROR::OKAY && !order_valid) {
                err = LUT_ERROR::ORDER_VIOLATION;
            } else if (err != LUT_ERROR::OKAY) {
                status_flags |= pnp_status::CONFIGURATION_SET_MISS;
            }

            if (err == LUT_ERROR::OKAY) {
                // Construct the save structure.
                if (p_device->p_config_entries == nullptr) {
                    // Construct without any custom configs.
                    device_configured = save_constructor::construct(
                        cfgs.array, STEPPER_CONFIGS_IN_SAVE,
                        &p_info->save.config.params);
                } else {
                    // Construct with a custom config.params.
                    device_configured = save_constructor::construct(
                        cfgs.array, STEPPER_CONFIGS_IN_SAVE,
                        &p_info->save.config.params, p_device->p_config_entries,
                        p_device->p_config_entries_size);
                }
                status_flags |= device_configured
                                    ? pnp_status::NO_ERRORS
                                    : pnp_status::CONFIGURATION_STRUCT_MISS;
            }

            // Free memory if needed
            if (p_device->p_slot_config != nullptr) {
                p_device->p_slot_config_size = 0;
                vPortFree(p_device->p_slot_config);
                p_device->p_slot_config = nullptr;
            }
            if (p_device->p_config_entries != nullptr) {
                p_device->p_config_entries_size = 0;
                vPortFree(p_device->p_config_entries);
                p_device->p_config_entries = nullptr;
            }

            // Check to see if the store needs to be reloaded.
            const bool SHOULD_KEEP_STORE =
                stepper_store_not_default(&p_info->save.store);
            if (device_configured && !SHOULD_KEEP_STORE) {
                // Construct the store.
                device_configured = save_constructor::construct(
                    &cfgs.cfg.stepper_store, 1, &p_info->save.store);
                status_flags |= device_configured
                                    ? pnp_status::NO_ERRORS
                                    : pnp_status::CONFIGURATION_STRUCT_MISS;
            }

            if (err != LUT_ERROR::OKAY) {
                status_flags |= pnp_status::GENERAL_CONFIG_ERROR;
            }

            // If not errors occured, mark the configuration as valid.
            if (device_configured) {
                r_cfg.mark_configured(SN);
            }
        }

        // Update the status bits
        {
            lock_guard(slots[p_info->slot].xSlot_Mutex);
            slots[p_info->slot].pnp_status =
                (slots[p_info->slot].pnp_status & PNP_STATUS_MASK) |
                status_flags;
        }

        if (r_cfg.is_params_configured(SN)) {
            // Configuration was successfully loaded into memory.
            lock_guard lg(xSPI_Semaphore);

            // Enable the stepper.
            enable_stepper_card(p_info);

            if (last_enabled) {
                p_info->channel_enable |= 0x01;
            }

            setup_io(p_info);

            // TODO if the encoder count is reversed we need to reverse the
            // loaded counts after loading in the values from eeprom
            load_params_to_drive(p_info);

            /*Copy the stored encoder p_info to the encoder structure*/
            p_info->stage_position_saved = true;
            p_info->current_stored_position = CURRENT_STORE_POS_NONE;
            p_info->enc.enc_pos = p_info->save.store.enc_pos;
            p_info->enc.enc_zero = p_info->save.store.enc_zero;
            p_info->enc.type = p_info->save.config.params.encoder.encoder_type;
            p_info->enc.homed = NOT_HOMED_STATUS;
#if ENABLE_STEPPER_LOG_STATUS_FLAGS
            p_info->stepper_status_read = false;
#endif

            reset_speed_channels(p_info);

            set_el_pos_stepper(
                p_info); /*move to the electrical position saved in eeprom*/
            setup_encoder(p_info->slot, p_info->save.config.params.flags.flags,
                          p_info->save.config.params.config.max_pos,
                          &p_info->enc);

            // (sbenish) The pointer is maintained over a long period of time.
            // Let this warning exist.
            setup_limits(p_info->slot, &p_info->limits, &p_info->enc.enc_pos_raw,
                         (bool)Tst_bits(p_info->save.config.params.flags.flags,
                                        ENCODER_REVERSED));

            service_encoder(p_info);
            sync_stepper_steps_to_encoder_counts(p_info);

// TEMP to test the index clearing
#if 0
            //    set_reg(C_SET_HOMING, p_info->slot, 0, (uint32_t)C_HOMING_MULT_INDEX);
            uint32_t history_index_counts = 0;
            bool first_index = true;
#endif
        } else {
            // Close the gate so it has to wait again.
            // A reset of this slot's device-detect FSM will be needed to try
            // again. Use the reset function or disconnect-reconnect.
            xGateClose(slots[p_info->slot].device.connection_gate);
        }

        watchdog.beat();
        watchdog.set_heartbeat_interval(STEPPER_HEARTBEAT_INTERVAL);
        xLastWakeTime = xTaskGetTickCount();
        // END      Device Initialzation

        // Run while the gate is open (device is connected)
        while (xGatePass(slots[p_info->slot].device.connection_gate, 0) ==
               pdPASS) {
            // BEGIN    Operation Loop
            {
                lock_guard lg(xSPI_Semaphore);

                /*Read the emergency stop flag from the CPLD, if it is high it
                 * means to uC locked up, had an error or was halted by
                 * debugging so we need to enable the card again and reload the
                 * drive prams*/
                if (Tst_bits(board.clpd_em_stop, 1 << p_info->slot)) {
                    /// \todo Verify that is works with the new code.
                    p_info->ctrl.mode = STOP;
                    p_info->ctrl.homing_mode = HOME_IDLE;
                    set_reg(C_SET_ENABLE_STEPPER_CARD, p_info->slot, 0,
                            DISABLED_MODE);
                    load_params_to_drive(p_info);
                    sync_stepper_steps_to_encoder_counts(p_info);
                    Clr_bits(board.clpd_em_stop, 1 << p_info->slot);
                    debug_print("em %d/r/n", p_info->slot);
                }

#if 0 // Oscillate stage 50Hz, enc 100nm
            if(p_info->slot == SLOT_1)
            {
                if(cnt == 2)
                {
                    move_stepper(SLOT_1, 1, 60);
                    cnt = 0;
                }
                if(cnt == 1)
                {
                    move_stepper(SLOT_1, 0, 60);
                }
                cnt++;
            }

#endif
                service_cdc(p_info, &slave_message, true, cdc_queue);
                service_joystick(p_info, hid_in_queue, &slave_message, true);
                // service_synchronized_motion(p_info, &sm_rx, &sm_tx);
                // service_piezo_synchronized_motion(p_info, &psm_rx, &psm_tx);
                service_encoder(p_info);
                service_limits(p_info->slot, &p_info->save.config.params.limits,
                               &p_info->limits, p_info->enc.type);

                magnetic_sensor_auto_homing_check(p_info);
                /*Service the main control loop for the stepper task*/
                service_stepper(p_info, &slave_message);

#if ENABLE_STEPPER_LOG_STATUS_FLAGS
                if (!p_info->stepper_status_read) {
                    p_info->stepper_status = get_status_stepper(p_info->slot);
                    p_info->stepper_status_read = true;
                }
#endif

#if DEBUG_PID_JSCOPE

                if (p_info->slot == SLOT_3) {
                    JS_RTT_Data data;
                    data.encoder = p_info->enc.enc_pos;
                    data.command = p_info->counter.cmnd_pos;
                    SEGGER_RTT_Write(JS_RTT_Channel, &data, sizeof(data));
                }
#endif
            }

            update_status_bits(p_info);

            /* Wait for the next cycle.*/
            watchdog.beat();
            vTaskDelayUntil(&xLastWakeTime, xFrequency);
            // END      Operation Loop
        }

        // BEGIN    Device Cleanup

        {
            lock_guard lg(xSPI_Semaphore);
            if (r_cfg.is_params_configured(SN)) {
                // Tell the device to stop moving, and disable the stepper card.
                p_info->ctrl.mode = STOP;
                p_info->ctrl.homing_mode = HOME_IDLE;
                p_info->stage_moved_elapse_count =
                    STEPPER_SAVE_POSITION_IDLE_TIME + 1; // Forces a store save.
                sync_stepper_steps_to_encoder_counts(p_info);
                service_stepper(p_info, &slave_message);
            }
            set_reg(C_SET_ENABLE_STEPPER_CARD, p_info->slot, 0, ONE_WIRE_MODE);
        }

        // Set disabled status.
        last_enabled = p_info->channel_enable & 0x01;
        Clr_bits(p_info->channel_enable, 1);
        update_status_bits(p_info);

        // END      Device Cleanup
    }
}

extern "C" void stepper_init(uint8_t slot, uint8_t type) {
    Stepper_info *pxStepper_info;

    /* Allocate a stepper structure for each task.*/
    pxStepper_info = new Stepper_info(
        static_cast<slot_nums>(slot)); // Uses cppmem.hh's new callback.

    char pcName[6] = {'S', 't', 'e', 'p', 'X'};
    pcName[4] = slot + 0x30;

    if ((type == ST_Stepper_type) ||
        (type == MCM_Stepper_Internal_BISS_L6470) ||
        (type == MCM_Stepper_Internal_SSI_L6470) ||
        (type == MCM_Stepper_L6470_MicroDB15) ||
        (type == MCM_Stepper_LC_HD_DB15)) {
        pxStepper_info->ctrl.which_stepper_drive = LOW_CURRENT_DRIVE;
    } else {
        pxStepper_info->ctrl.which_stepper_drive = HIGH_CURRENT_DRIVE;
    }

    /* Create task for each stepper card */
    if (xTaskCreate((TaskFunction_t)task_stepper, pcName,
                    TASK_STEPPER_STACK_SIZE, (Stepper_info *)pxStepper_info,
                    TASK_STEPPER_STACK_PRIORITY,
                    &xSlotHandle[slot]) != pdPASS) {
        /* free pxStepper_info*/
        delete pxStepper_info;
    }
}
