/**
 * @file
 *
 * @brief Manage slot identification and initialization
 *
  *  *@dot
 digraph G {
 node [shape=diamond]; "get slot type";
 node [shape=ellipse];
 "initialize slots" -> "get slot type";
 "get slot type" -> "stepper";
 "get slot type" -> "servo";
 "get slot type" -> "shutter";
 "get slot type" -> "OTM DAC";
 "stepper" -> "create structures" -> "setup motor parameters" -> "start stepper thread" -> "end init";
 "shutter" -> "do some stuff" -> "do some more stuff" -> "endinit";
 "OTM DAC" -> "startup peripheral" -> "end init";
 { rank=same; stepper, servo, shutter, OTM DAC }
 }
 * @enddot
 */
// slots.c
#include "slots.h"
// include "slots_for_cards.h" /// \todo Is this real?

#include "m24c08.h"
#include "Debugging.h"
#include "board.h"
#include "pins.h"
#include "slot_types.h"
#include "usb_device.h"
#include "device_detect.h"
#include "25lc1024.h"
#include "apt.h"
#include "lut_manager.h"

/*Cards*/
#include "stepper.h"
#include "delay.h"
#include "flipper-shutter-thread.h"

/****************************************************************************
 * Data
 ****************************************************************************/

Slots slots[NUMBER_OF_BOARD_SLOTS];

#define SLOT_SAVE_MAX_ALLOWED_DEVICES           ((EEPROM_25LC1024_PAGE_SIZE - sizeof(Slot_Save)) / sizeof(device_signature_t))

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/
static void set_default_params(slot_nums slot);

/****************************************************************************
 * Interrupt Handler
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/
// MARK:  SPI Mutex Required
static void slot_save_info_eeprom_init(slot_nums slot)
{
    slots[slot].save.slot_data_type = SLOT_DATA_TYPE;

    /*Calculate the address*/
    uint32_t eeprom_page_address = SLOT_EEPROM_ADDRESS(slot, SLOT_SAVE_PAGE);

    // save it to eeprom
    eeprom_25LC1024_write(eeprom_page_address, sizeof(Slot_Save),
            (uint8_t*) &slots[slot].save);

    if (slots[slot].save.num_allowed_devices > 0) {
        // Calculate upper bound for save slot, and save up to the upper bound
        const uint16_t MAX_DEVICES_ALLOWED = (slots[slot].save.num_allowed_devices > SLOT_SAVE_MAX_ALLOWED_DEVICES) ? SLOT_SAVE_MAX_ALLOWED_DEVICES : slots[slot].save.num_allowed_devices;
        const uint16_t tmp = slots[slot].save.num_allowed_devices;
        slots[slot].save.num_allowed_devices = MAX_DEVICES_ALLOWED;

        // Save supported devices to EEPROM
        eeprom_25LC1024_write
        (
            eeprom_page_address + sizeof(Slot_Save),
            sizeof(device_signature_t)*slots[slot].save.num_allowed_devices,
            (uint8_t*)slots[slot].p_allowed_devices
        );

        // Restore allowed devices in memory.
        slots[slot].save.num_allowed_devices = tmp;
    }
}

// MARK:  SPI Mutex Required
static void set_default_params(slot_nums slot)
{
    slots[slot].save.slot_data_type = SLOT_DATA_TYPE;
    slots[slot].save.allow_device_detection = true;
    slots[slot].save.num_allowed_devices = 0;
    slots[slot].save.static_card_type = NO_CARD_IN_SLOT;
    slots[slot].save.allowed_device_serial_number = (uint64_t)OW_SERIAL_NUMBER_WILDCARD;

    slot_save_info_eeprom_init(slot);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void slot_save_info_eeprom(slot_nums slot)
{
    xSemaphoreTake(slots[slot].xSlot_Mutex, portMAX_DELAY);

    slot_save_info_eeprom_init(slot);
    
    xSemaphoreGive(slots[slot].xSlot_Mutex);
}

// MARK:  SPI Mutex Required
void slot_get_info_eeprom(slot_nums slot)
{
    /* Reset the structure*/
    memset(&slots[slot].save, 0, sizeof(Slot_Save));

    /*Calculate the address*/
    uint32_t eeprom_page_address = SLOT_EEPROM_ADDRESS(slot, SLOT_SAVE_PAGE);

    eeprom_25LC1024_read(eeprom_page_address, sizeof(Slot_Save),
            (uint8_t*) &slots[slot].save);

    /* Validate the data read from eeprom by looking at the first byte which holds data type*/
    if (slots[slot].save.slot_data_type != SLOT_DATA_TYPE)
    {
        set_default_params(slot);
    }

    // Check if allowed devices is erased and, if so, save it to 0.
    if (slots[slot].save.num_allowed_devices > SLOT_SAVE_MAX_ALLOWED_DEVICES)
    {
        slots[slot].save.num_allowed_devices = 0;
    }

    // Make sure allowed devices is non-zero.
    if (slots[slot].save.num_allowed_devices > 0)
    {
        // If memory existed there, free it
        if (slots[slot].p_allowed_devices) {
            vPortFree(slots[slot].p_allowed_devices);
        }

        // Allocate memory for supported devices.
        slots[slot].p_allowed_devices = pvPortMalloc(sizeof(device_signature_t)*slots[slot].save.num_allowed_devices);

        if (slots[slot].p_allowed_devices == NULL) {
            /// \todo Error
        }

        // Read in the number of supported devices from EEPROM
        eeprom_25LC1024_read
        (
            eeprom_page_address + sizeof(Slot_Save),
            sizeof(device_signature_t)*slots[slot].save.num_allowed_devices,
            (uint8_t*)slots[slot].p_allowed_devices
        );
    } else {
        slots[slot].p_allowed_devices = NULL;
    }
}


/**
 * @brief Get all the types of slot boards in each slot.  There is a I2C multiplexer
 *         which selects the slot we talk to.  If a slot dosn't respond with an ack we
 *         assume their is no slot card install.  If the card responds with 0xFFFF we
 *         assume the card has not been set and should return as NO_CARD_IN_SLOT.
 *
 *         Some boards have static    hardware on the slot so we don't read each slot
 *         EEPROM just initialize the static hardware.  If this is the case, this function
 *         should return all the static hardware for each slot.
 */
// MARK:  SPI Mutex Required
void get_slot_types(void)
{
#if 1
    /*Slots 1 - 7 have I2C half slot 8 don't so keep out of loop and handle differently*/
    for (uint8_t slot = 0; slot < NUMBER_OF_BOARD_SLOTS; ++slot)
    {
        /*Read the value in on board eeprom */
        slot_get_info_eeprom(slot);
        if (slot < SLOT_8)
        {

#if 1
            /*If it is a static card set the card_type to the static value from on board eeprom*/
            if (Tst_bits(slots[slot].save.static_card_type, STATIC_CARD_SLOT_MASK))
            {
                slots[slot].card_type = slots[slot].save.static_card_type;
            }
            else
            {
                /* Read from the slot card eeprom. If it return EEPROM_FAIL, this means no card is installed
                 * or the card has no eeprom*/
                m24c08_read_page(slot, EEPROM_ADDRESS_M24C08_BLOCK0, 0, 2,
                        (uint8_t *) &slots[slot].card_type);
                {
                    if (slots[slot].save.static_card_type == END_CARD_TYPES)
                    {
                        slots[slot].card_type = CARD_IN_SLOT_NOT_PROGRAMMED;
                    }
//                    /*If it is a static card set the card_type  to the static value from on board eeprom*/
//                    else if (Tst_bits(slots[slot].save.static_card_type, STATIC_CARD_SLOT_MASK))
//                    {
//                        slots[slot].card_type = slots[slot].save.static_card_type;
//                    }
//                    else
//                    {
//                        //            debug_print("WARNING: No card in slot./r/n");
//                        slots[slot].card_type = NO_CARD_IN_SLOT;
//                    }
                }
            }




#else
            /* Read from the slot card eeprom. If it return EEPROM_FAIL, this means no card is installed
             * or the card has no eeprom*/
            if (m24c08_read_page(slot, EEPROM_ADDRESS_M24C08_BLOCK0, 0, 2,
                    (uint8_t *) &slots[slot].card_type))
            {
                if (slots[slot].save.static_card_type == END_CARD_TYPES)
                {
                    slots[slot].card_type = NO_CARD_IN_SLOT;
                }
                /*If it is a static card set the card_type  to the static value from on board eeprom*/
                else if (Tst_bits(slots[slot].save.static_card_type, STATIC_CARD_SLOT_MASK))
                {
                    slots[slot].card_type = slots[slot].save.static_card_type;
                }
                else
                {
                    //            debug_print("WARNING: No card in slot./r/n");
                    slots[slot].card_type = NO_CARD_IN_SLOT;
                }
            }
            else
            {
                if(slots[slot].card_type == END_CARD_TYPES)
                    slots[slot].card_type = CARD_IN_SLOT_NOT_PROGRAMMED;
            }
#endif
        }
        /*Slot 8 has no EEPROM so always static type*/
        else // for slot 8 assume NO_CARD_IN_SLOT until we read eeprom below
        {
            slots[slot].card_type = slots[slot].save.static_card_type;
            // remove the top bit that indicated it's static hardware but not used in card_type var
//            Clr_bits(slots[slot].card_type, STATIC_CARD_SLOT_MASK);
        }

        /*If the card_type in memory is not equal to the eeprom value save it*/
        if (slots[slot].card_type != slots[slot].save.static_card_type)
        {
            slots[slot].save.static_card_type = slots[slot].card_type;
            slot_save_info_eeprom_init(slot);
        }

        if (Tst_bits(slots[slot].card_type, STATIC_CARD_SLOT_MASK))
        {
            // remove the top bit that indicated it's static hardware but not used in card_type var
            Clr_bits(slots[slot].card_type, STATIC_CARD_SLOT_MASK);
        }

    }



#else

    for (uint8_t slot = 0; slot < NUMBER_OF_BOARD_SLOTS; ++slot)
    {
        slot_get_info_eeprom(slot);

        if (slots[slot].save.static_card_type == END_CARD_TYPES)
        {
            debug_print("WARNING: Card in slot not programmed.");
            slots[slot].card_type = CARD_IN_SLOT_NOT_PROGRAMMED;
            slots[slot].save.static_card_type = CARD_IN_SLOT_NOT_PROGRAMMED;
            slot_save_info_eeprom(slot);
        }

        else if(Tst_bits(slots[slot].save.static_card_type, STATIC_CARD_SLOT_MASK))    // static card slot
        {
            slots[slot].card_type = Clr_bits(slots[slot].save.static_card_type, STATIC_CARD_SLOT_MASK);
        }
        else // flex card slot
        {
            if (m24c08_read_page(slot, EEPROM_ADDRESS_M24C08_BLOCK0, 0, 2,
                    (uint8_t *) &slots[slot].card_type))
            {
    //            debug_print("WARNING: No card in slot./r/n");
                slots[slot].card_type = NO_CARD_IN_SLOT;
            }
            else
            {
                slot_get_info_eeprom(slot);
                if(slots[slot].save.static_card_type != slots[slot].card_type)
                {
                    slots[slot].save.static_card_type = slots[slot].card_type;
                    slot_save_info_eeprom(slot);
                }
                slots[slot].save.static_card_type = slots[slot].card_type;

            }
            if (slots[slot].card_type == END_CARD_TYPES)
            {
                debug_print("WARNING: Card in slot not programmed.");
                slots[slot].card_type = CARD_IN_SLOT_NOT_PROGRAMMED;
                slots[slot].save.static_card_type = CARD_IN_SLOT_NOT_PROGRAMMED;
            }

            if (slots[slot].card_type >= MAX_CARD_TYPE)
            {
                debug_print("ERROR: Card type detected out of range.");

                slots[slot].card_type = CARD_IN_SLOT_NOT_PROGRAMMED;
                slots[slot].save.static_card_type = CARD_IN_SLOT_NOT_PROGRAMMED;
            }
        }
    }
#endif
}


void em_stop(uint8_t em_slots)
{
    for (slot_nums slot = 0; slot < NUMBER_OF_BOARD_SLOTS; ++slot)
    {
        slots[slot].em_stop = (bool) Tst_bits(em_slots, 1<<slot);
    }
}

void init_slots_synchronization(void)
{
    for (slot_nums slot = 0; slot < NUMBER_OF_BOARD_SLOTS; ++slot)
    {
        slots[slot].xSlot_Mutex = xSemaphoreCreateMutex();
        if (slots[slot].xSlot_Mutex == NULL)
        {
            /// \todo Throw an error
        }
#ifdef USE_RTOS_DEBUGGING
        char name[8] = {'S', 'L', 'O', 'T', ' ', '1' + slot, 'L', '\0'};
        NameQueueObject(slots[slot].xSlot_Mutex, name);
#endif
    }
}

/**
 * @brief Get the type of slot cards installed then run their init functions to
 * start task and initialize the hardware
 */
void init_slots()
{
    bool card_in_slot = false;
    device_detect_init();
    lut_manager_init();

    for (slot_nums slot = 0; slot < NUMBER_OF_BOARD_SLOTS; ++slot)
    {
        if(card_in_slot)
        {
            /*Insert a delay to sequence power to each slot with a bit of time for the rails to come
             * up slowly and not all at once*/
            delay_ms(SLOT_INIT_DELAY);
        }

        /* Read the slot save struct from EEPROM to ram*/
        slot_get_info_eeprom(slot);

        slots[slot].em_stop = false;
        slots[slot].pnp_status = PNP_NO_ERROR;

        card_in_slot = true;

        switch (slots[slot].card_type)
        {
            case NO_CARD_IN_SLOT:
                card_in_slot = false;
            break;

            case ST_Stepper_type:
            case ST_HC_Stepper_type:
            case High_Current_Stepper_Card_HD:
            case ST_Invert_Stepper_BISS_type:
            case ST_Invert_Stepper_SSI_type:
            case MCM_Stepper_Internal_BISS_L6470:
            case MCM_Stepper_Internal_SSI_L6470:
            case MCM_Stepper_L6470_MicroDB15:
            case MCM_Stepper_LC_HD_DB15:
                stepper_init(slot, slots[slot].card_type);
                break;

            case MCM_Flipper_Shutter:
            case MCM_Flipper_Shutter_REVA:
                flipper_shutter_init(slot, slots[slot].card_type);
            break;

            // (sbenish)  I was seeing that something was not behaving properly with the
            // piezo's CPLD code when the shutter 4 module was enabled on slot 7.  The
            // slot 7 piezo module would mess with the rx_slot signal, but I would not
            // figure out why.  I have disabled this until we need it.  See DevOps
            // bug #320 (70-0059) for more details.
            default:
                card_in_slot = false;
            break;
        }
    }
}

/**
 * @brief Sets the type of slot card we are using.
 * @param slot            Integer value of the slot to be written
 * @param slot_type        A list of slot cards is in slots.h.  Any new cards or revisions should be added there
 *                         If the card has no eeprom card detection and the slot is set to static then the
 *                         top most bit (0x8000) will be set to identify as static slot hardware
 * @return : 0 everything went well, 1 if could not set board type
 */
uint8_t set_slot_type(slot_nums slot, slot_types slot_type)
{
    uint8_t ret_val = 0;

    // make sure board_type is less than to NO_BOARD 0x8000
    if ((board_type < NO_BOARD) || (slot_type > END_CARD_TYPES))
        return FAIL;


    // if already set to the same then just get out
    if(slots[slot].save.static_card_type == slot_type)
        return PASS;

    slots[slot].card_type = Rd_bits(slot_type, ~STATIC_CARD_SLOT_MASK);
    slots[slot].save.static_card_type = slot_type;

    if(!Tst_bits(slot_type, STATIC_CARD_SLOT_MASK))    // flex card slot
    {
        ret_val = m24c08_write_page(slot, EEPROM_ADDRESS_M24C08_BLOCK0, 0, 2, (uint8_t*) &slots[slot].card_type);
    }

    slot_save_info_eeprom(slot);
    return ret_val;
}

bool slot_parse_generic_apt(const slot_nums slot_num, USB_Slave_Message * const p_msg, 
	uint8_t * const p_response_buffer, uint8_t * const p_length, bool * const p_send)
{
    bool processed = true;
    
    switch(p_msg->ucMessageID)
    {
    default:
        processed = false;
    break;
    }

    return processed;
}

bool slot_is_compatible(slot_types card_type, slot_types signatures_card_type)
{
    // Removing the static bit.
    card_type &= 0x7FFF;
    signatures_card_type &= 0x7FFF;

    bool rt;
    switch (card_type) {
        case MCM_Flipper_Shutter:
        case MCM_Flipper_Shutter_REVA:
            rt = signatures_card_type == MCM_Flipper_Shutter
                || signatures_card_type == MCM_Flipper_Shutter_REVA
                || signatures_card_type == Shutter_4_type
                || signatures_card_type == Shutter_4_type_REV6
                || signatures_card_type == Slider_IO_type;
        break;

        default:
            rt = card_type == signatures_card_type;
        break;
    }
    return rt;
}


//EOF
