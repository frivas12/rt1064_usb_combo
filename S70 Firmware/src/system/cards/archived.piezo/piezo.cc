/*
 * piezo.c
 *
 *  Created on: Sep 21, 2020
 *      Author: frivas
 */

#include "piezo.h"

#include "ad5683.h"
#include "ads8689.h"
#include <Debugging.h>
#include "itc-service.hh"
#include "sys_task.h"
#include "usb_slave.h"
#include "delay.h"
#include "usb_device.h"
#include "apt.h"
#include <string.h>
#include <encoder_abs_biss_linear.h>
#include <cpld.h>
#include <apt_parse.h>
#include "25lc1024.h"
#include <pins.h>
#include "../../drivers/log/log.h"
#include "math.h"
#include "../../drivers/adc/ads8689.h"

#include <lut_manager.hh>
#include <save_constructor.hh>
#include <lock_guard.hh>
#include <save_util.hh>


/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/
static void set_vars(Piezo_info *info);
static void setup_io(uint8_t slot);
static void power(uint8_t slot, bool state);
static void write_eeprom_data(Piezo_info *info);
static void read_eeprom_data(Piezo_info *info);
static void parse(Piezo_info *info, USB_Slave_Message *slave_message);
static bool service_cdc(Piezo_info *info, USB_Slave_Message *message,
                        service::itc::pipeline_cdc_t::unique_queue& queue);
static float calculate_controller_gain(Piezo_info *info, int32_t delta_pos);
static void calculate_error_window(Piezo_info *info);
static void run_displacement_calculations(Piezo_info *info);
static bool calculate_displacement_time(Piezo_info *info);
static bool calculate_overshoot(Piezo_info *info);
static void calculate_move(Piezo_info *info);
static bool service_encoder(Piezo_info *info);
static void service_analog_input(Piezo_info *info, piezo_sm_Rx_data *psm_rx, piezo_sm_Tx_data *psm_tx);
static void service_piezo(Piezo_info *info, piezo_sm_Rx_data *psm_rx, piezo_sm_Tx_data *psm_tx);
static void send_plot_data(Piezo_info *info, USB_Slave_Message *slave_message);
static void create_queues(void);
static void read_stepper_queue(Piezo_info *info, piezo_sm_Rx_data *psm_rx);
static void send_stepper_queue(Piezo_info *info, piezo_sm_Tx_data *psm_tx);
static void service_stepper_queue(Piezo_info *info, piezo_sm_Rx_data *psm_rx, piezo_sm_Tx_data *psm_tx);
#if HYSTERESIS_TRACKING
static void track_hysetresis(Piezo_info *info);
#endif
static void pid_controller(Piezo_info *info);
static void controller(Piezo_info *info);
// static void output_pos_on_dac(Piezo_info *info);
static bool dac_ramp_voltage(Piezo_info *info, uint16_t dac_slewrate);
static void task_piezo (Piezo_info * p_info);


/****************************************************************************
 * Private Functions
 ****************************************************************************/
static void set_vars(Piezo_info *info)
{
    // These are default value, real values will be loaded when reading EEPROM if it is of correct type
    info->save.config.saved_params.params.slot_encoder_on = NO_SLOT;
    info->mode = PZ_STOP_MODE;
    info->enable_plot = false;

    /*DAC*/
    info->dac.val = DAC_ZERO_VALUE;
    info->dac.ramp_voltage_state = DAC_RAMP_START;

    /*ADC*/
//    info->adc.input_mode = ANALOG_INPUT_MODE_DISABLED;
    info->adc.input_state = PZ_AIN_START;

    /* P Controller*/
    info->cntlr.error = 0;
    info->cntlr.filter_cnt = 0;
    info->cntlr.kp_factor = 1.0;

    info->queue_send_flag = PIEZO_QUEUE_NO_SEND;

//    info->save.slot_encoder_on = SLOT_1;
//    info->save.gain_factor = 700;

#if HYSTERESIS_TRACKING
    info->hyst.dir = HYSTERESIS_UP;
    info->hyst.max = 0;
    info->hyst.min = 0;
    info->hyst.dir_change = 0;
    info->hyst.change_delta = 0;
#endif
}

static void setup_io(uint8_t slot)
{
    switch (slot)
    {
    case SLOT_1:
        set_pin_as_output(PIN_SMIO_1, PIN_STATE_OFF); // POWER_EN
        break;
    case SLOT_2:
        set_pin_as_output(PIN_SMIO_2, PIN_STATE_OFF);
        break;
    case SLOT_3:
        set_pin_as_output(PIN_SMIO_3, PIN_STATE_OFF);
        break;
    case SLOT_4:
        set_pin_as_output(PIN_SMIO_4, PIN_STATE_OFF);
        break;
    case SLOT_5:
        set_pin_as_output(PIN_SMIO_5, PIN_STATE_OFF);
        break;
    case SLOT_6:
        set_pin_as_output(PIN_SMIO_6, PIN_STATE_OFF);
        break;
    case SLOT_7:
        set_pin_as_output(PIN_SMIO_7, PIN_STATE_OFF);
        break;
    }
}

static void power(uint8_t slot, bool state)
{
    switch (slot)
    {
    case SLOT_1:
        pin_state(PIN_SMIO_1, state); // POWER_EN
        break;
    case SLOT_2:
        pin_state(PIN_SMIO_2, state);
        break;
    case SLOT_3:
        pin_state(PIN_SMIO_3, state);
        break;
    case SLOT_4:
        pin_state(PIN_SMIO_4, state);
        break;
    case SLOT_5:
        pin_state(PIN_SMIO_5, state);
        break;
    case SLOT_6:
        pin_state(PIN_SMIO_6, state);
        break;
    case SLOT_7:
        pin_state(PIN_SMIO_7, state);
        break;
    }
}

static void write_eeprom_data(Piezo_info *info)
{
    const uint32_t BASE_ADDRESS = SLOT_EEPROM_ADDRESS(info->slot, 0);

    save_util::update_save(info->slot, BASE_ADDRESS, info->save.config);
}

static void read_eeprom_data(Piezo_info *info)
{
    const uint32_t BASE_ADDRESS = SLOT_EEPROM_ADDRESS(info->slot, 0);


    info->save.config.read_eeprom(BASE_ADDRESS);
}

/**
 * @brief Parse APT commands that are specific to piezo card
 *
 * Takes xUSB_Slave_TxSemaphore - sending data on the USB bus
 *
 * @param info : The piezo info structure.
 * @param slave_message : This is the massage structure pass from the USB salve task
 */
static void parse(Piezo_info *info, USB_Slave_Message *slave_message)
{
    uint8_t response_buffer[ADC_BUFFER_SIZE + 10] =    { 0 };
    uint8_t length = 0;
    bool need_to_reply = false;
    int32_t temp_32_1;
    piezo_log_ids log_id = PIEZO_NO_LOG;
    float temp_f = 0;
    uint32_t temp_u32_1;

    switch (slave_message->ucMessageID)
    {
    case MGMSG_MCM_ERASE_DEVICE_CONFIGURATION: /* 0x4020 */
    // Invalidate the configuration on the slot card identified
    if (slave_message->ExtendedData_len >= 2 && info->slot == slave_message->extended_data_buf[0])
    {
        const uint32_t BASE_ADDRESS = SLOT_EEPROM_ADDRESS(info->slot, 0);

        info->save.config.invalidate(BASE_ADDRESS);

        device_detect_reset(&slots[info->slot].device);
    }
    break;

    case MGMSG_MCM_PIEZO_SET_PRAMS: /* 0x40DC */
        info->save.config.saved_params.params.slot_encoder_on = static_cast<slot_nums>(slave_message->extended_data_buf[2]);
        info->save.config.saved_params.params.gain_factor = slave_message->extended_data_buf[3];
        write_eeprom_data(info);
        log_id = PIEZO_SET_PRAMS;
        break;

    case MGMSG_MCM_PIEZO_REQ_PRAMS: /* 0x40DD */
        /* Header */
        response_buffer[0] = MGMSG_MCM_PIEZO_GET_PRAMS;
        response_buffer[1] = MGMSG_MCM_PIEZO_GET_PRAMS >> 8;
        response_buffer[2] = 4;
        response_buffer[3] = 0x00;
        response_buffer[4] = HOST_ID | 0x80; /* destination & has extended data*/
        response_buffer[5] = SLOT_1_ID + static_cast<uint8_t>(info->slot); /* source*/

        /*Chan Ident*/
        response_buffer[6] = info->slot;
        response_buffer[7] = 0x00;

        /*Piezo encoder slot*/
        response_buffer[8] = info->save.config.saved_params.params.slot_encoder_on;

        /*Piezo gain factor*/
        response_buffer[9] = info->save.config.saved_params.params.gain_factor;

        need_to_reply = true;
        length = 6 + response_buffer[2];
        break;

    case MGMSG_MCM_PIEZO_REQ_VALUES: /* 0x40DF */
        /* Header */
        response_buffer[0] = MGMSG_MCM_PIEZO_GET_VALUES;
        response_buffer[1] = MGMSG_MCM_PIEZO_GET_VALUES >> 8;
        response_buffer[2] = 12;
        response_buffer[3] = 0x00;
        response_buffer[4] = HOST_ID | 0x80; /* destination & has extended data*/
        response_buffer[5] = SLOT_1_ID + static_cast<uint8_t>(info->slot); /* source*/

        /*Chan Ident (2 bytes)*/
        response_buffer[6] = info->slot;
        response_buffer[7] = 0x00;

        /*Piezo kp (4 bytes)*/
        temp_u32_1 = (uint32_t) (info->cntlr.kp * 1000);
        memcpy(&response_buffer[8], &temp_u32_1, sizeof(info->cntlr.kp));

        /*Piezo cntlr.displacement_time (2 bytes)*/
        memcpy(&response_buffer[12], &info->cntlr.displacement_time, sizeof(info->cntlr.displacement_time));

        /*Piezo cntlr.overshoot (2 bytes)*/
        memcpy(&response_buffer[14], &info->cntlr.overshoot, sizeof(info->cntlr.overshoot));

        /*Piezo cntlr.overshoot_time (2 bytes)*/
        memcpy(&response_buffer[16], &info->cntlr.overshoot_time,
                sizeof(info->cntlr.overshoot_time));

        need_to_reply = true;
        length = 6 + response_buffer[2];
        break;

    case MGMSG_MCM_PIEZO_SET_ENABLE_PLOT: /* 0x40E3 */
        info->enable_plot = slave_message->param2;
        break;

    case MGMSG_MCM_PIEZO_SET_DAC_VOLTS: /* 0x40E4 */
        temp_f = slave_message->extended_data_buf[2]
                + (slave_message->extended_data_buf[3] << 8);

        // voltage is in mV, convert volts.
        temp_f /= 1000;

        // Convert volts into dac counts
        temp_f /= DAC_VOLTS_PER_COUNT;
        info->dac.cmd_val = temp_f;
        info->mode = SET_DAC_VOLTS;
        break;

    case MGMSG_MCM_PIEZO_SET_PID_PARMS: /* 0x40E5 */
        memcpy(&info->save.config.saved_params.params.Kp, &slave_message->extended_data_buf[2], 4);
        memcpy(&info->save.config.saved_params.params.Ki, &slave_message->extended_data_buf[6], 4);
        memcpy(&info->save.config.saved_params.params.Kd, &slave_message->extended_data_buf[10], 4);
        memcpy(&info->save.config.saved_params.params.imax, &slave_message->extended_data_buf[14], 4);
        write_eeprom_data(info);
        break;

    case MGMSG_MCM_PIEZO_REQ_PID_PARMS: /* 0x40E5 */
        /* Header */
        response_buffer[0] = MGMSG_MCM_PIEZO_GET_PID_PARMS;
        response_buffer[1] = MGMSG_MCM_PIEZO_GET_PID_PARMS >> 8;
        response_buffer[2] = 18;
        response_buffer[3] = 0x00;
        response_buffer[4] = HOST_ID | 0x80; /* destination & has extended data*/
        response_buffer[5] = SLOT_1_ID + static_cast<uint8_t>(info->slot); /* source*/

        /*Chan Ident (2 bytes)*/
        response_buffer[6] = info->slot;
        response_buffer[7] = 0x00;

        memcpy(&response_buffer[8], &info->save.config.saved_params.params.Kp, 4);
        memcpy(&response_buffer[12], &info->save.config.saved_params.params.Ki, 4);
        memcpy(&response_buffer[16], &info->save.config.saved_params.params.Kd, 4);
        memcpy(&response_buffer[20], &info->save.config.saved_params.params.imax, 4);
        need_to_reply = true;
        length = 6 + response_buffer[2];
        break;

    case MGMSG_MCM_PIEZO_SET_MOVE_BY: /* 0x40E6 */
        // distance to move in encoder counts
        memcpy(&temp_32_1, &slave_message->extended_data_buf[2], 4);
        info->cntlr.delta_pos = abs(temp_32_1);

        // set the commanded position
        info->cntlr.cmnd_pos = temp_32_1 + info->enc.pos;

        // set the mode
        if(slave_message->extended_data_buf[1] == 0)
            info->mode = PZ_CONTROLER_MODE;
        else
            info->mode = PZ_PID_MODE;

        calculate_move(info);
        break;

    case MGMSG_MCM_PIEZO_SET_MODE: /* 0x40E9 */
        if(info->save.config.saved_params.params.slot_encoder_on < SLOT_8)
        {
            info->mode = slave_message->param1;
            if(info->mode == PZ_ANALOG_INPUT_MODE)
                info->adc.input_state = PZ_AIN_START;
        }
        else
        {
            info->mode = PZ_STOP_MODE;
            // set board error
            Set_bits(board.error, 1 << info->slot);
        }
        break;

    case MGMSG_MCM_PIEZO_REQ_MODE: /* 0x40EA */
        // param1 is the mode
        /* Header */
        response_buffer[0] = MGMSG_MCM_PIEZO_GET_MODE;
        response_buffer[1] = MGMSG_MCM_PIEZO_GET_MODE >> 8;
        response_buffer[2] = info->mode;
        response_buffer[3] = 0;
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
            log_id = PIEZO_UNSUPPORTED_COMMAND;
        }
        break;
    }

    if (log_id != PIEZO_NO_LOG)
        setup_and_send_log(slave_message, info->slot, LOG_NO_REPEAT,
                PIEZO_LOG_TYPE, log_id, 0, 0, 0);

    if (need_to_reply)
    {
        slave_message->write((uint8_t*) response_buffer, length);
    }
}

static bool service_cdc(Piezo_info *info, USB_Slave_Message *slave_message,
                           service::itc::pipeline_cdc_t::unique_queue& queue)
{
    std::size_t counter = 0;
    while (queue.try_pop(*slave_message)) {
        parse(info, slave_message);
        ++counter;
    }
    return counter > 0;
}

static int32_t median_filter(Piezo_info *info, int32_t new_counts)
{
    int temp;
    int i, j;

    if (info->cntlr.filter_cnt >= FILTER_VAL_CNT)
        info->cntlr.filter_cnt = 0;
    else
        info->cntlr.filter_cnt++;

    // put new value in array
    info->cntlr.pos_filter_vals[info->cntlr.filter_cnt] = new_counts;

    // the following two loops sort the array x in ascending order
    for (i = 0; i < FILTER_VAL_CNT - 1; i++)
    {
        for (j = i + 1; j < FILTER_VAL_CNT; j++)
        {
            if (info->cntlr.pos_filter_vals[j] < info->cntlr.pos_filter_vals[i])
            {
                // swap elements
                temp = info->cntlr.pos_filter_vals[i];
                info->cntlr.pos_filter_vals[i] = info->cntlr.pos_filter_vals[j];
                info->cntlr.pos_filter_vals[j] = temp;
            }
        }
    }

    if (FILTER_VAL_CNT % 2 == 0)
    {
        // if there is an even number of elements, return mean of the two elements in the middle
        return ((info->cntlr.pos_filter_vals[FILTER_VAL_CNT / 2]
                + info->cntlr.pos_filter_vals[FILTER_VAL_CNT / 2 - 1]) / 2.0);
    }
    else
    {
        // else return the element in the middle
        return info->cntlr.pos_filter_vals[FILTER_VAL_CNT / 2];
    }
}

static float calculate_controller_gain(Piezo_info *info, int32_t delta_pos)
{
#if 1
    /*Adjust kp with the power function below to adjust kp for different displacements
     * y = 1884.7x^-0.337 => y = kp, x displacement (delta_pos)
    */

    float Kp = 1884.7 * pow(delta_pos, -0.337);

    /* gain factor moves the model equation, 1 is default, 1.1 (110) will make the response faster, .9 (90) will make it slower*/
    Kp = Kp * ((float) info->save.config.saved_params.params.gain_factor/100);
    return Kp;
#else
    // tested and works well y=-47.76ln(x)+368.57

    float Kp = (-47.761*log(delta_pos)*((float) info->save.gain_factor/100)) +368.57;
    return Kp;
#endif
}

static void calculate_error_window(Piezo_info *info)
{
    // figure out 5% of displacement error on position in encoder counts for dynamically tuning kp
    info->cntlr.error_window = info->cntlr.delta_pos * .05;
}

static void run_displacement_calculations(Piezo_info *info)
{
    // increment the counter
    info->cntlr.displacement_cnt++;

    switch (info->cntlr.displacement_state)
    {
    case DISPLACEMENT_STATE_START:
        /*Wait until we have reach 5% on the target position*/
        if (calculate_displacement_time(info))
        {
            info->cntlr.displacement_state = DISPLACEMENT_STATE_WAIT;
            // reset the counter for calculating the cntlr.overshoot time
            info->cntlr.displacement_cnt = 0;
        }
        break;

    case DISPLACEMENT_STATE_WAIT:
        if (info->cntlr.displacement_cnt >= 4) // wait 1 ms so we cross over the error = 0
        {
            info->cntlr.displacement_state = DISPLACEMENT_STATE_COMPLETE;
            // reset the counter for calculating the cntlr.overshoot time
            info->cntlr.displacement_cnt = 0;
            info->cntlr.overshoot = 0;
        }
        break;

    case DISPLACEMENT_STATE_COMPLETE:
        /*Now lets calculate the cntlr.overshoot*/
        if (calculate_overshoot(info))
        {
            info->cntlr.displacement_state = DISPLACEMENT_STATE_OVERSHOOT_DONE;
        }
        break;

    case DISPLACEMENT_STATE_OVERSHOOT_DONE:
        // wait here until an new move is commanded
        break;

    default:
        break;
    }
}

static bool calculate_displacement_time(Piezo_info *info)
{
    // if we are within 5% of the triggered displacement
    if (abs(info->cntlr.error) < info->cntlr.error_window)
    {
        // mark displacement complete
        info->cntlr.displacement_complete = 1;
        // convert cntlr.displacement_cnt to ms
        info->cntlr.displacement_time = info->cntlr.displacement_cnt / 4; // task running at 4kHz or .25ms
        return 1;
    }
    return 0;
}

static bool calculate_overshoot(Piezo_info *info)
{
    uint32_t error = abs(info->cntlr.error);

    /* after we get to the desired position, we can start measuring the cntlr.overshoot
     * Run this for until the error decrease to zero  */
    if (error == 0)
    {
        // mark cntlr.overshoot complete
        info->cntlr.overshoot_complete = 1;
        // convert cntlr.displacement_cnt to ms
        info->cntlr.overshoot_time = info->cntlr.displacement_cnt / 4; // task running at 4kHz or .25ms
        info->cntlr.overshoot_zero_crossing_cnt++;
        if (info->cntlr.overshoot_zero_crossing_cnt
                > OVERSHOOT_ZERO_CROSSING_CNT)
            return 1;
    }
    else if (error > info->cntlr.overshoot)
    {
        info->cntlr.overshoot = error;
    }
    return 0;
}

static void calculate_move(Piezo_info *info)
{
    // put in pid mode
    info->cntlr.kp = calculate_controller_gain(info, info->cntlr.delta_pos);
    calculate_error_window(info);

    /*reset move parameters */
    info->cntlr.displacement_state = DISPLACEMENT_STATE_START;
    info->cntlr.displacement_cnt = 0;
    info->cntlr.displacement_complete = 0;
    info->cntlr.overshoot_complete = 0;
    info->cntlr.factor_adjustment_complete = 0;
    info->cntlr.overshoot_zero_crossing_cnt = 0;
}

static bool service_encoder(Piezo_info *info)
{
    uint8_t temp;
    uint32_t counts;

    if (info->save.config.saved_params.params.slot_encoder_on == NO_SLOT)
        return 1;    // error, slot to read not set

    /*read the encoder*/
    read_reg(C_READ_BISS_ENC, info->save.config.saved_params.params.slot_encoder_on, &temp, &counts);

    info->enc.pos = (int32_t) counts;

    /*The error bit is active low: �1� indicates that the transmitted
     * position information has been verified by the readhead�s internal
     * safety checking algorithm and is correct; �0� indicates that the
     * internal check has failed and the position information should not
     *  be trusted. The error bit is also set to �0� if the temperature
     *   exceeds the maximum specified for the product.*/
    info->enc.error = Tst_bits(temp, 0x80);        // 0 fail

    /*The warning bit is active low: �0� indicates that the encoder
     * scale (and/or reading window) should be cleaned.*/
    info->enc.warn = Tst_bits(temp, 0x40);    // 0 fail

    /*If the error bit is low then encoder has an error, stop the piezo controller
     * and set the slot error bit.  The system needs to be power cycled to reset error. */
    if (!info->enc.error)
    {
        // The system needs to be power cycled to reset error
        if(!info->enc.error)
        {
            /* The fagor encoder has errors while booting so we count them and if the exceed a limit
             * then the error exist, this counter should be reset when the error is cleared    by the encoder*/
            info->enc.error_cnt++;
            if(info->enc.error_cnt > FAGOR_ENCODER_ERROR_LIMIT)
            {
                info->mode = PZ_STOP_MODE;
                Set_bits(board.error, 1 << info->slot);
            }
        }
        else
        {
            info->enc.error_cnt = 0;
        }
    }

    // only filter after we make a move just before the cntlr.overshoot
    if (info->cntlr.displacement_complete)
        info->enc.pos_filtered = median_filter(info, (int32_t) counts);
    else
    {
        // continue filling array but use the real encoder counts until our move is complete
        median_filter(info, (int32_t) counts);
        info->enc.pos_filtered = info->enc.pos;
    }
    return 0;
}

// MARK:  SPI Mutex Required
static void pid_controller(Piezo_info *info)
{
    float Kp = info->save.config.saved_params.params.Kp/ (float) KP_SCALE;
    float Ki = info->save.config.saved_params.params.Ki/ (float) KP_SCALE;
    float Kd = info->save.config.saved_params.params.Kd/ (float) KP_SCALE;

    float input = info->enc.pos_filtered;

    // Compute error
    float error = info->cntlr.cmnd_pos - info->enc.pos_filtered;
    info->cntlr.error = error;

    // Compute integral
    info->cntlr.iterm += Ki * error;
    if (info->cntlr.iterm > info->save.config.saved_params.params.imax)
        info->cntlr.iterm = info->save.config.saved_params.params.imax;
    else if (info->cntlr.iterm < (-1.0 * (float) info->save.config.saved_params.params.imax))
        info->cntlr.iterm = -1.0 * (float) info->save.config.saved_params.params.imax;

    // Compute differential on input
    float dinput = input - info->cntlr.lastin;
    // Keep track of some variables for next execution
    info->cntlr.lastin = input;

    // Compute PID output
    float out = Kp * error + info->cntlr.iterm
            - Kd * dinput;

    /*Scale down the output to give higher granularity for higher resolution encoders*/
//    out /= 1000;

    // Apply limit to output value
    if ((uint32_t) (info->dac.val + out) > DAC_MAX_VALUE)
        info->dac.val = DAC_MAX_VALUE;
    else if (info->dac.val < DAC_MIN_VALUE)
        info->dac.val = DAC_MIN_VALUE;
    else
        info->dac.val += out;

    dac_write(info->slot, DAC_WRITE_DAC_AND_INPUT_REG, info->dac.val);

    run_displacement_calculations(info);
}

// MARK:  SPI Mutex Required
static void controller(Piezo_info *info)
{
    float ctlr_gain, out;

    // Compute error
    float error = info->cntlr.cmnd_pos - info->enc.pos_filtered;
    float abs_error = abs(error);
    info->cntlr.error = error;

    if (info->mode == PZ_ANALOG_INPUT_MODE)
    {
        info->cntlr.displacement_complete = 0; // use unfiltered encoder value

#if 0
        if(abs_error >= 400)                        // >= 60um
            ctlr_gain = info->cntlr.gain_60um;
        else if(abs_error >= 300)                    // >= 30um
            ctlr_gain = info->cntlr.gain_30um;
        else if(abs_error >= 200)                    // >= 3um
            ctlr_gain = info->cntlr.gain_3um;
        else                                        // < 3um
            ctlr_gain = info->cntlr.gain_1um;

        if (abs_error < 6)
            info->cntlr.displacement_complete = 1; // use filtered encoder value
#elif 1
        if (abs_error > 700)
        {
            ctlr_gain = info->cntlr.gain_10um;
        }
        else
        {
            if (abs(error) < 6)
            {
                info->cntlr.displacement_complete = 1; // use filtered encoder value
            }

            ctlr_gain = info->cntlr.gain_3um;
        }
#else
        if (abs_error > 700)
        {
            ctlr_gain = info->cntlr.gain_60um;
        }
        else
        {
            if (abs(error) < 6)
            {
                info->cntlr.displacement_complete = 1; // use filtered encoder value
            }

            ctlr_gain = info->cntlr.gain_1um;
        }
#endif
    }
    else
    {
        ctlr_gain = (info->cntlr.kp / (float) KP_SCALE);
    }

    out = ctlr_gain * error;

    // Apply limit to output value
    if ((uint32_t) (info->dac.val + out) > DAC_MAX_VALUE)
        info->dac.val = DAC_MAX_VALUE;
    else if (info->dac.val < DAC_MIN_VALUE)
        info->dac.val = DAC_MIN_VALUE;
    else
        info->dac.val += out;

    dac_write(info->slot, DAC_WRITE_DAC_AND_INPUT_REG, info->dac.val);
    if (info->mode != PZ_ANALOG_INPUT_MODE)
        run_displacement_calculations(info);
}

// MARK:  SPI Mutex Required
// static void output_pos_on_dac(Piezo_info *info)
// {
// #define DAC_SCALE    13.107

//     // calculate delta from the zero offset
//     info->enc.pos_delta = info->enc.pos_filtered - info->enc.pos_zero;
//     info->dac.val = DAC_SCALE * info->enc.pos_delta;

//     dac_write(info->slot, DAC_WRITE_DAC_AND_INPUT_REG, info->dac.val);
// }

/*Ramps the voltage on the dac.
 * Returns 1 when complete*/
// MARK:  SPI Mutex Required
static bool dac_ramp_voltage(Piezo_info *info, uint16_t dac_slewrate)
{
    bool complete = 0;
    uint16_t abs_delta = abs(info->dac.cmd_val - info->dac.val);

    switch (info->dac.ramp_voltage_state)
    {
    case DAC_RAMP_START:
        info->dac.ramp_voltage_state = DAC_RAMP;
        break;

    case DAC_RAMP:
        if (abs_delta == 0) // done
        {
            info->dac.ramp_voltage_state = DAC_RAMP_DONE;
        }
        else if (info->dac.val < info->dac.cmd_val) // ramp up
        {
            // check limits
            if (abs_delta < dac_slewrate)    // this will get us to the value
            {
                info->dac.val += abs_delta;
            }
            else if (DAC_MAX_VALUE - info->dac.val > dac_slewrate)
                info->dac.val += dac_slewrate;
            else
            {
                info->dac.val = dac_slewrate;
            }
        }
        else // ramp down
        {
            // check limits
            if (abs_delta < dac_slewrate)    // this will get us to the value
            {
                info->dac.val -= abs_delta;
            }
            else if (info->dac.val - DAC_MIN_VALUE > dac_slewrate)
                info->dac.val -= dac_slewrate;
            else
            {
                info->dac.val = DAC_MIN_VALUE;
            }
        }
        dac_write(info->slot, DAC_WRITE_DAC_AND_INPUT_REG, info->dac.val);

        break;

    case DAC_RAMP_DONE:
        //setup for next time
        info->dac.ramp_voltage_state = DAC_RAMP_START;
        complete = 1;
        break;

    default:
        break;
    }

    return complete;
}

// MARK:  SPI Mutex Required
static void service_analog_input(Piezo_info *info, piezo_sm_Rx_data *psm_rx, piezo_sm_Tx_data *psm_tx)
{
    /*Vin range for the adc is 0 - 10V.  Mapping where 0V = 0um and 10V = 100um*/

    int32_t adc_val_in_enc_counts = 0;

    switch (info->adc.input_state)
    {
    case PZ_AIN_START:
        /*calculate the controller gain schedules*/
#if 1
        info->cntlr.gain_1um = calculate_controller_gain(info, 100) / KP_SCALE;
        info->cntlr.gain_3um = calculate_controller_gain(info, 300) / KP_SCALE;
        info->cntlr.gain_10um = calculate_controller_gain(info, 1000) / KP_SCALE;
        info->cntlr.gain_30um = calculate_controller_gain(info, 3000) / KP_SCALE;
        info->cntlr.gain_60um = calculate_controller_gain(info, 6000) / KP_SCALE;
#else
        info->cntlr.gain_1um = calculate_controller_gain(info, 100) / KP_SCALE;
        info->cntlr.gain_60um = calculate_controller_gain(info, 2000) / KP_SCALE;

#endif
        info->adc.input_state = PZ_AIN_STEPPER_HOLD_POS;
        break;

    case PZ_AIN_STEPPER_HOLD_POS:
        /*Temporarily put the stepper in position hold*/
        psm_tx[info->save.config.saved_params.params.slot_encoder_on].stepper_hold_pos = 1;
        info->queue_send_flag = PIEZO_QUEUE_SEND_DATA;
        info->adc.input_state = PZ_AIN_STEPPER_WAIT;
        break;

    case PZ_AIN_STEPPER_WAIT:
        /*Wait until the message was sent*/
        if(info->queue_send_flag == PIEZO_QUEUE_NO_SEND)
        {
            /*Set the cmd dac voltage to the adc value plus .5V at the bottom (this is the
             * negative voltage and we need to leave head room here */
            info->dac.cmd_val = info->adc.val + DAC_LOWER_HEADROOM;
            info->adc.input_state = PZ_AIN_PIEZO_SYNC;
        }
        break;

    case PZ_AIN_PIEZO_SYNC:
        /*Slowly move the piezo until the dac matches the adc input ratio while the stepper holds the position,
         * this will move the stepper while we adjust the dac to adc ratio, then adjust the values for the
         * piezo displacement.  When the piezo is where it needs to be, send the stepper a meassage to take
         * out of position hold mode*/
        if(dac_ramp_voltage(info, DAC_SLOW_SLEW_RATE))
        {
            psm_tx[info->save.config.saved_params.params.slot_encoder_on].stepper_hold_pos = 0;
            info->queue_send_flag = PIEZO_QUEUE_SEND_DATA;
            info->adc.input_state = PZ_AIN_STEPPER_WAIT_2;
        }
        break;

    case PZ_AIN_STEPPER_WAIT_2:
        /*Wait until the message was sent to the stepper to stop the position hold*/
        if(info->queue_send_flag == PIEZO_QUEUE_NO_SEND)
        {
            info->adc.input_state = PZ_AIN_STEPPER_WAIT_3;
            info->analog_in_wait_cnt = 0;
        }
        break;

    case PZ_AIN_STEPPER_WAIT_3:
        /*Wait for the stepper to settle*/
        if(info->analog_in_wait_cnt >= 400)
        {
            adc_val_in_enc_counts = (float) info->adc.val / ANALOG_INPUT_COUNTS_PER_ENC_COUNTS;
            info->enc.start_val = info->enc.pos - adc_val_in_enc_counts;
            info->adc.input_state = PZ_AIN_RUN;
        }
        info->analog_in_wait_cnt++;
        break;

    case PZ_AIN_RUN:
        if(!psm_rx->stepper_moving)
        {
            // update the controller with the scaled analog input
            adc_val_in_enc_counts = (float) info->adc.val / ANALOG_INPUT_COUNTS_PER_ENC_COUNTS;
            info->cntlr.cmnd_pos = adc_val_in_enc_counts + info->enc.start_val;
            /*Run the controller*/
            controller(info);
        }
        else
        {
            /*If the stepper is moved, save the encoder position*/
            info->enc.before_stepper_move_val =  info->enc.pos;
            info->adc.input_state = PZ_AIN_STEPPER_MOVING;
        }
        break;

    case PZ_AIN_STEPPER_MOVING:
        /*Stepper had moved, wait until it stops*/
        if(!psm_rx->stepper_moving)
        {
            info->analog_in_wait_cnt = 0;
            info->adc.input_state = PZ_AIN_STEPPER_WAIT_4;
        }
        break;

    case PZ_AIN_STEPPER_WAIT_4:
        /*Wait for the stepper to settle, then calculate the new offset for the piezo start position*/
        if(info->analog_in_wait_cnt >= 400)
        {
            if(info->enc.pos > info->enc.before_stepper_move_val)
                info->enc.start_val += info->enc.pos - info->enc.before_stepper_move_val;
            else
                info->enc.start_val -=info->enc.before_stepper_move_val -  info->enc.pos;

            info->adc.input_state = PZ_AIN_RUN;
        }
        info->analog_in_wait_cnt++;
        break;

    default:
        break;
    }
}

// MARK:  SPI Mutex Required
static void service_piezo(Piezo_info *info, piezo_sm_Rx_data *psm_rx, piezo_sm_Tx_data *psm_tx)
{
#if HYSTERESIS
    static bool up = 1;    // use for PZ_HYSTERESIS_MODE
    int32_t adc_val_in_enc_counts = 0;
#endif

    switch (info->mode)
    {
    case PZ_STOP_MODE:
        break;

    case PZ_ANALOG_INPUT_MODE:
        service_analog_input(info, psm_rx, psm_tx);
        break;

    case SET_DAC_VOLTS: // input from PC to set DAC voltage to move piezo using a ramp
        if (dac_ramp_voltage(info, DAC_MAX_SLEW_RATE))
            info->mode = PZ_STOP_MODE;
        break;

    case PZ_CONTROLER_MODE:    // input from PC id sent to the controller to move piezo
        controller(info);
        break;

    case PZ_PID_MODE:    //  input from PC id sent to the pid controller to move piezo
        pid_controller(info);
        break;

    case PZ_HYSTERESIS_MODE:
#if HYSTERESIS    // only for testing
        #define HYSTERESIS_DELTA    20
        if (up)
        {
            if (info->dac.val < info->save.Kp)
            {
                if ((DAC_MAX_VALUE - HYSTERESIS_DELTA) < info->dac.val)
                    info->dac.val = DAC_MAX_VALUE;
                else
                    info->dac.val += HYSTERESIS_DELTA;
            }
            else
            {
                up = 0;
            }
        }
        else
        {
            if (info->dac.val > info->save.Ki)
            {
                if (info->dac.val > HYSTERESIS_DELTA)
                    info->dac.val -= HYSTERESIS_DELTA;
                else
                    info->dac.val = 0;
            }
            else // done
            {
                up = 1;
                info->mode = PZ_STOP_MODE;
            }
        }
        dac_write(info, DAC_WRITE_DAC_AND_INPUT_REG, info->dac.val);
#endif
        break;

    default:
        break;
    }
}

static void send_plot_data(Piezo_info *info, USB_Slave_Message *slave_message)
{
#define  num_extended_data  (2 + 4 + 4 + 2 + 4 + 4)

    uint8_t response_buffer[6 + num_extended_data] =
    { 0 };

    /* Header */
    response_buffer[0] = MGMSG_MCM_PIEZO_GET_LOG;
    response_buffer[1] = MGMSG_MCM_PIEZO_GET_LOG >> 8;
    response_buffer[2] = num_extended_data;
    response_buffer[3] = 0x00;
    response_buffer[4] = HOST_ID | 0x80; /* destination & has extended data*/
    response_buffer[5] = SLOT_1_ID + static_cast<uint8_t>(info->slot); /* source*/

    // adc value (2 bytes)
    memcpy(&response_buffer[6], &info->adc.val, sizeof(info->adc.val));

    // encoder value (4 bytes)
    memcpy(&response_buffer[8], &info->enc.pos, sizeof(info->enc.pos));

    // encoder filtered value (4 bytes)
    memcpy(&response_buffer[12], &info->enc.pos_filtered,
            sizeof(info->enc.pos_filtered));

    // dac value (2 bytes)
    memcpy(&response_buffer[16], &info->dac.val, sizeof(info->adc.val));

    // pid error value (4 bytes)
    memcpy(&response_buffer[18], &info->cntlr.error, sizeof(info->cntlr.error));

    // pid setpoint value (4 bytes)
    memcpy(&response_buffer[22], &info->cntlr.cmnd_pos,
            sizeof(info->cntlr.cmnd_pos));

    slave_message->write((uint8_t*) response_buffer, 6 + num_extended_data);
}

static void create_queues(void)
{
    for (uint8_t slot = 0; slot < NUMBER_OF_BOARD_SLOTS ; ++slot)
    {
        /* Create a queue for receiving data from the slot task. */
        xPiezo_SM_Rx_Queue[slot] = xQueueCreate(1, sizeof(xPiezo_SM_Rx_Queue));
        if (xPiezo_SM_Rx_Queue[slot] == NULL)
        {
            debug_print(
                    "ERROR: The Slot %d piezo synchronized_moition Rx queue could not be created./r/n",
                    slot);
        }

        /* Create a queue for sending data to the slot task. */
        xPiezo_SM_Tx_Queue[slot] = xQueueCreate(1, sizeof(xPiezo_SM_Tx_Queue));
        if (xPiezo_SM_Tx_Queue[slot] == NULL)
        {
            debug_print(
                    "ERROR: The Slot %d piezo synchronized_moition Tx queue could not be created./r/n",
                    slot);
        }
    }
}

static void read_stepper_queue(Piezo_info *info, piezo_sm_Rx_data *psm_rx)
{
    /*Check if new queue data from the stepper slot piezo connected to the piezo is available*/
    if (xPiezo_SM_Rx_Queue[info->save.config.saved_params.params.slot_encoder_on] != NULL)
    {
        if ( xQueueReceive(xPiezo_SM_Rx_Queue[info->save.config.saved_params.params.slot_encoder_on],
                &psm_rx[info->save.config.saved_params.params.slot_encoder_on], 0) == pdPASS)
        {
            // nothing to do here
        }
    }
}

static void send_stepper_queue(Piezo_info *info, piezo_sm_Tx_data *psm_tx)
{
    /*Send the queue to the stepper task the piezo is connected to but don't wait*/
    xQueueSend(xPiezo_SM_Tx_Queue[info->save.config.saved_params.params.slot_encoder_on],
            (void* ) &psm_tx[info->save.config.saved_params.params.slot_encoder_on], 0);

    info->queue_send_flag = PIEZO_QUEUE_NO_SEND;
}

static void service_stepper_queue(Piezo_info *info, piezo_sm_Rx_data *psm_rx, piezo_sm_Tx_data *psm_tx)
{
#if 1
    read_stepper_queue(info, psm_rx);

    if(info->queue_send_flag == PIEZO_QUEUE_SEND_DATA)
        send_stepper_queue(info, psm_tx);

    info->queue_service_cnt = 0;

#else

    /*Stepper task runs at 10ms, this task runs at .25ms so we need to wait 40 cycles
     * before we service the queues*/
    if(info->queue_service_cnt >= 40)
    {
        read_stepper_queue(info, psm_rx);

        if(info->queue_send_flag == PIEZO_QUEUE_SEND_DATA)
            send_stepper_queue(info, psm_tx);

        info->queue_service_cnt = 0;
    }
    else
        info->queue_service_cnt++;
#endif
}

#if HYSTERESIS_TRACKING
static void track_hysetresis(Piezo_info *info)
{
    // 325 counts is about .5um

    // going up
    if(info->hyst.dir == HYSTERESIS_UP)
    {
        // save new max value if larger
        if(info->dac.val > info->hyst.max)
        {
            info->hyst.max = info->dac.val;
        }
        else if(info->dac.val < info->hyst.max)
        {
            info->hyst.dir_change = 1;
            info->hyst.change_delta = info->hyst.max - info->hyst.min;
            info->hyst.min = info->dac.val;
            info->hyst.dir = HYSTERESIS_DOWN;
        }
    }
    else // going down
    {
        if(info->dac.val > info->hyst.min)
        {
            info->hyst.dir_change = 1;
            info->hyst.change_delta = info->hyst.max - info->hyst.min;
            info->hyst.max = info->dac.val;
            info->hyst.dir = HYSTERESIS_UP;
        }
        else if(info->dac.val < info->hyst.min)
        {
            info->hyst.min = info->dac.val;
        }
    }
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Task
 ****************************************************************************/

static void task_piezo (Piezo_info * p_info)
{
    // BEGIN    Task Initialization

    /*Wait here until power is good*/
    while (board.power_good == POWER_NOT_GOOD)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    /* Create the structure for holding the USB data received from the USB slave task*/
    USB_Slave_Message slave_message;

    /* Create the structure for holding the USB data received from the USB host task*/
    // USB_Host_Rx_Message host_message;

    // Create queues for sync motion with stepper
    piezo_sm_Rx_data psm_rx[NUMBER_OF_BOARD_SLOTS];
    piezo_sm_Tx_data psm_tx[NUMBER_OF_BOARD_SLOTS];
    create_queues();

    auto cdc_queue = *service::itc::pipeline_cdc().create_queue(
        static_cast<asf_destination_ids>((int)p_info->slot + (int)SLOT_1_ID));

    TickType_t xLastWakeTime;
    const TickType_t xFrequency = 1; // 1= 4kHz

    // Initialize device-independent IO.
    setup_io(p_info->slot);
    
    // END      Task Initialization
    
    for (;;)
    {
        // Wait until the gate is opened (valid device is connected)
        xGatePass(slots[p_info->slot].device.connection_gate, portMAX_DELAY);
        
        // BEGIN    Device Initialization

        // Initialize variables not saved in the EEPROM.
        set_vars(p_info);

        // Read saved data for this stage.
        {
            lock_guard lg (xSPI_Semaphore);
            read_eeprom_data(p_info);
        }
        bool save_data_valid = p_info->save.config.valid();

        if (! save_data_valid)
        {
            // Get configs from the device lut.
            Piezo_Save_Union cfgs;
            lut_manager::device_lut_key_t key;
            xSemaphoreTake(slots[p_info->slot].xSlot_Mutex, portMAX_DELAY);
            key.running_slot_card = slots[p_info->slot].card_type;
            key.connected_device = slots[p_info->slot].device.info.signature;
            xSemaphoreGive(slots[p_info->slot].xSlot_Mutex);
            LUT_ERROR err = lut_manager::load_data(LUT_ID::DEVICE_LUT, &key, &cfgs.cfg);

            bool order_valid = cfgs.cfg.params.struct_id == static_cast<struct_id_t>(Struct_ID::PIEZO_PARAMS);

            if (err == LUT_ERROR::OKAY && ! order_valid)
            {
                err = LUT_ERROR::ORDER_VIOLATION;
            }

            if (err == LUT_ERROR::OKAY)
            {
                // Construct the save structure.
                save_data_valid = save_constructor::construct(cfgs.array,
                    PIEZO_CONFIGS_IN_SAVE, &p_info->save.config.saved_params);
            }
        }

        if (save_data_valid) {
            // Configuration was sucessfully loaded into memory.

            xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);

            if(p_info->save.config.saved_params.params.slot_encoder_on > SLOT_7)
            {
                p_info->mode = PZ_STOP_MODE;
                // set board error
                Set_bits(board.error, 1 << p_info->slot);
            }

            // initialize queues between piezo and stepper task
            psm_tx[p_info->save.config.saved_params.params.slot_encoder_on].stepper_hold_pos = 0;

            if (adc_init(p_info->slot))
            {
                power(p_info->slot, PIN_STATE_OFF);
                // set board error
                Set_bits(board.error, 1 << p_info->slot);
                setup_and_send_log(&slave_message, p_info->slot, LOG_NO_REPEAT,
                        PIEZO_LOG_TYPE, PIEZO_ENCODER_SLOT_NOT_SET, 0, 0, 0);
            }

            dac_init(p_info->slot, 0);
            power(p_info->slot, PIN_STATE_ON);

            xSemaphoreGive(xSPI_Semaphore);
        } else {
            // Close the gate so it has to wait again.
            // A reset of this slot's device-detect FSM will be needed to try again.
            // Use the reset function or disconnect-reconnect.
            xGateClose(slots[p_info->slot].device.connection_gate);
        }
        
        // END      Device Initialization

        // Initialize the xLastWakeTime variable with the current time.
        xLastWakeTime = xTaskGetTickCount();
        
        // Run while the gate is open (device is connected)
        while (xGatePass(slots[p_info->slot].device.connection_gate, 0) == pdPASS)
        {
            // BEGIN    Operation Loop
            
            xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);

            // if the slot_encoder_on is not set only parse apt commands so this slot
            // can be configure, else jump over everything
            if(p_info->save.config.saved_params.params.slot_encoder_on > SLOT_7)
            {
                p_info->mode = PZ_STOP_MODE;
                // set board error
                Set_bits(board.error, 1 << p_info->slot);
                service_cdc(p_info, &slave_message, cdc_queue);
            }
            else
            {
                /*Stepper task runs at 10ms, this task runs at .25ms so we need to wait 40 cycles
                * before we service the queues*/
                if(p_info->queue_service_cnt >= 40)
                {
                    service_stepper_queue(p_info,
                          reinterpret_cast<piezo_sm_Rx_data*>(&psm_rx),
                          reinterpret_cast<piezo_sm_Tx_data*>(&psm_tx));
                    service_cdc(p_info, &slave_message, cdc_queue);
                    p_info->queue_service_cnt = 0;
                }
                else
                    p_info->queue_service_cnt++;


                p_info->adc.val = adc_read(p_info->slot);
                service_encoder(p_info);
                service_piezo(p_info,
                          reinterpret_cast<piezo_sm_Rx_data*>(&psm_rx),
                          reinterpret_cast<piezo_sm_Tx_data*>(&psm_tx));

                if (p_info->enable_plot)
                {
                    send_plot_data(p_info, &slave_message);
                }
            }

            xSemaphoreGive(xSPI_Semaphore);

            /* Wait for the next cycle.*/
            vTaskDelayUntil(&xLastWakeTime, xFrequency);

            // END      Operation Loop
        }

        // BEGIN    Device Cleanup

        // Tell piezo to stop.
        /// \todo Verify that this works
        p_info->mode = PZ_STOP_MODE;
        xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);
        service_piezo(p_info,
                      reinterpret_cast<piezo_sm_Rx_data*>(&psm_rx),
                      reinterpret_cast<piezo_sm_Tx_data*>(&psm_tx));
        xSemaphoreGive(xSPI_Semaphore);

        // END      Device Cleanup
    }
}

extern "C" void piezo_init(uint8_t slot)
{
    Piezo_info *pxPiezo_info;

    /* Allocate a Piezo structure for each task.*/
    pxPiezo_info = new Piezo_info;

    char pcName[6] =
    { 'P', 'i', 'e', 'z', 'X' };
    pcName[4] = slot + 0x30;

    pxPiezo_info->slot = static_cast<slot_nums>(slot);

    /* Create task for each Piezo card */
    if (xTaskCreate((TaskFunction_t) task_piezo, pcName, TASK_PIEZO_STACK_SIZE,
            (Piezo_info*) pxPiezo_info, TASK_PIEZO_STACK_SIZE,
            &xSlotHandle[slot]) != pdPASS)
    {
        debug_print("Failed to create Piezo task for slot %d\r\n", slot + 1);

        /* free pxServo_info*/
        delete pxPiezo_info;
    }
}

