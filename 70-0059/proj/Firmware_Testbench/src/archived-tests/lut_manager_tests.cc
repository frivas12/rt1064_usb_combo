// lut_manager_tests.cc

/**************************************************************************//**
 * \file lut_manager_tests.cc
 * \author Sean Benish
 * \brief Unit tests for lut_manager.hh
 *****************************************************************************/

#include <catch_amalgamated.hpp>
#include <lut_manager.hh>
#include "25lc1024.h"

#include "lut_example_configs.hh"
#include "lut.tcc"

/*****************************************************************************
 * Unit Tests
 *****************************************************************************/

TEST_CASE("Erased LUTs can be inquired.")
{
    eeprom_init();
    lut_manager::init();

    SECTION("Device LUT can be inquired.")
    {
        LUT_ERROR err = lut_manager::begin_inquiry(LUT_ID::DEVICE_LUT);

        REQUIRE(err == LUT_ERROR::OKAY);

        // Cleanup
        lut_manager::cancel_inquiry(LUT_ID::DEVICE_LUT);
    }

    SECTION("Device LUT can be canceled.")
    {
        LUT_ERROR err = lut_manager::begin_inquiry(LUT_ID::DEVICE_LUT);

        REQUIRE(err == LUT_ERROR::OKAY);

        err = lut_manager::cancel_inquiry(LUT_ID::DEVICE_LUT);

        REQUIRE(err == LUT_ERROR::OKAY);
    }

    SECTION("Device LUT can check for inquiry.")
    {
        LUT_ERROR err;

        REQUIRE_FALSE(lut_manager::has_inquiry(LUT_ID::DEVICE_LUT));

        err = lut_manager::begin_inquiry(LUT_ID::DEVICE_LUT);

        CHECK(err == LUT_ERROR::OKAY);
        REQUIRE(lut_manager::has_inquiry(LUT_ID::DEVICE_LUT));

        err = lut_manager::cancel_inquiry(LUT_ID::DEVICE_LUT);

        CHECK(err == LUT_ERROR::OKAY);
        REQUIRE_FALSE(lut_manager::has_inquiry(LUT_ID::DEVICE_LUT)); 
    }
}

TEST_CASE("Device LUT can have devices added.")
{
    eeprom_init();
    lut_manager::init();

    SECTION("Device LUT can have valid slot cards added.")
    {
        constexpr int LEN = 2 * sizeof(slot_types) + sizeof(uint32_t);
        char buffer[LEN];
        slot_types st_tmp;
        uint32_t count;

        // Try steppers
        st_tmp = ST_Stepper_type;
        count = 1;

        memcpy(buffer, &st_tmp, sizeof(st_tmp));
        memcpy(buffer + sizeof(slot_types), &st_tmp, sizeof(st_tmp));
        memcpy(buffer + 2*sizeof(slot_types), &count, sizeof(uint32_t));

        LUT_ERROR err = lut_manager::begin_inquiry(LUT_ID::DEVICE_LUT);

        REQUIRE(err == LUT_ERROR::OKAY);

        err = lut_manager::stream_inquiry(LUT_ID::DEVICE_LUT, buffer, LEN);

        REQUIRE(err == LUT_ERROR::OKAY);

        // Cleanup
        lut_manager::cancel_inquiry(LUT_ID::DEVICE_LUT);
    }

    SECTION("Device LUT cannot take slot cards without a save structure.")
    {
        constexpr int LEN = 2 * sizeof(slot_types) + sizeof(uint32_t);
        char buffer[LEN];
        slot_types st_tmp;
        uint32_t count;

        // Try steppers
        st_tmp = Piezo_Elliptec_type;
        count = 1;

        memcpy(buffer, &st_tmp, sizeof(st_tmp));
        memcpy(buffer + sizeof(slot_types), &st_tmp, sizeof(st_tmp));
        memcpy(buffer + 2*sizeof(slot_types), &count, sizeof(uint32_t));

        LUT_ERROR err = lut_manager::begin_inquiry(LUT_ID::DEVICE_LUT);

        REQUIRE(err == LUT_ERROR::OKAY);

        err = lut_manager::stream_inquiry(LUT_ID::DEVICE_LUT, buffer, LEN);

        REQUIRE(err == LUT_ERROR::BAD_STRUCTURE_KEY);

        // Cleanup
        lut_manager::cancel_inquiry(LUT_ID::DEVICE_LUT);
    }

    SECTION("Device LUT cannot take invalid ordering.")
    {
        constexpr int LEN = 2 * sizeof(slot_types) + sizeof(uint32_t);
        char buffer[LEN];
        slot_types st_tmp;
        uint32_t count;

        // Try steppers
        st_tmp = ST_HC_Stepper_type;
        count = 1;

        memcpy(buffer, &st_tmp, sizeof(st_tmp));
        memcpy(buffer + sizeof(slot_types), &st_tmp, sizeof(st_tmp));
        memcpy(buffer + 2*sizeof(slot_types), &count, sizeof(uint32_t));
        LUT_ERROR err = lut_manager::begin_inquiry(LUT_ID::DEVICE_LUT);

        REQUIRE(err == LUT_ERROR::OKAY);

        err = lut_manager::stream_inquiry(LUT_ID::DEVICE_LUT, buffer, LEN);

        REQUIRE(err == LUT_ERROR::OKAY);

        // Check same signature ordering.
        err = lut_manager::stream_inquiry(LUT_ID::DEVICE_LUT, buffer, LEN);

        REQUIRE(err == LUT_ERROR::ORDER_VIOLATION);

        // Check differnet signature ordering.
        st_tmp = ST_Stepper_type;
        memcpy(buffer + sizeof(slot_types), &st_tmp, sizeof(st_tmp));
        err = lut_manager::stream_inquiry(LUT_ID::DEVICE_LUT, buffer, LEN);

        REQUIRE(err == LUT_ERROR::ORDER_VIOLATION);

        
        // Check different indirection key ordering.
        memcpy(buffer, &st_tmp, sizeof(st_tmp));
        err = lut_manager::stream_inquiry(LUT_ID::DEVICE_LUT, buffer, LEN);

        REQUIRE(err == LUT_ERROR::ORDER_VIOLATION);

        // Cleanup
        lut_manager::cancel_inquiry(LUT_ID::DEVICE_LUT);
    }

    SECTION("Device LUT handles indirection key switching.")
    {
        constexpr int LEN = 2 * sizeof(slot_types) + sizeof(uint32_t);
        char buffer[LEN];
        slot_types st_tmp;
        uint32_t count;

        // Try steppers
        st_tmp = ST_Stepper_type;
        count = 1;

        memcpy(buffer, &st_tmp, sizeof(st_tmp));
        memcpy(buffer + sizeof(slot_types), &st_tmp, sizeof(st_tmp));
        memcpy(buffer + 2*sizeof(slot_types), &count, sizeof(uint32_t));
        LUT_ERROR err = lut_manager::begin_inquiry(LUT_ID::DEVICE_LUT);

        REQUIRE(err == LUT_ERROR::OKAY);

        err = lut_manager::stream_inquiry(LUT_ID::DEVICE_LUT, buffer, LEN);

        REQUIRE(err == LUT_ERROR::OKAY);

        st_tmp = ST_HC_Stepper_type;
        memcpy(buffer, &st_tmp, sizeof(st_tmp));
        err = lut_manager::stream_inquiry(LUT_ID::DEVICE_LUT, buffer, LEN);

        REQUIRE(err == LUT_ERROR::OKAY);

        // Cleanup
        lut_manager::cancel_inquiry(LUT_ID::DEVICE_LUT);
    }

    SECTION("Device LUT acknowledges misalignment.")
    {
        constexpr int LEN = 2 * sizeof(slot_types) + sizeof(uint32_t);
        char buffer[LEN];
        slot_types st_tmp;
        uint32_t count;

        st_tmp = ST_Stepper_type;
        count = 1;

        for (uint32_t i = 0; i < 2; ++i)
        {
            memcpy(buffer + i*sizeof(slot_types), &st_tmp, sizeof(slot_types));
        }
        memcpy(buffer + 2*sizeof(slot_types), &count, sizeof(count));

        LUT_ERROR err = lut_manager::begin_inquiry(LUT_ID::DEVICE_LUT);

        REQUIRE(err == LUT_ERROR::OKAY);

        err = lut_manager::stream_inquiry(LUT_ID::DEVICE_LUT, buffer, 2);

        REQUIRE(err == LUT_ERROR::ALIGNMENT);

        err = lut_manager::stream_inquiry(LUT_ID::DEVICE_LUT, buffer, LEN - 2);

        REQUIRE(err == LUT_ERROR::OKAY);

        // Cleanup
        lut_manager::cancel_inquiry(LUT_ID::DEVICE_LUT);
    }

    SECTION ("Device LUT rejects configs when not inquiring.")
    {
        constexpr int LEN = 2 * sizeof(slot_types) + sizeof(uint32_t);
        char buffer[LEN];
        slot_types st_tmp;
        uint32_t count;

        // Try steppers
        st_tmp = ST_Stepper_type;
        count = 1;

        memcpy(buffer, &st_tmp, sizeof(st_tmp));
        memcpy(buffer + sizeof(slot_types), &st_tmp, sizeof(st_tmp));
        memcpy(buffer + 2*sizeof(slot_types), &count, sizeof(uint32_t));

        LUT_ERROR err = lut_manager::stream_inquiry(LUT_ID::DEVICE_LUT, buffer, LEN);

        REQUIRE(err == LUT_ERROR::NO_INQUIRY);

        // Cleanup
        lut_manager::cancel_inquiry(LUT_ID::DEVICE_LUT);
    }
}

TEST_CASE("Device LUT can start and stop programming.")
{
    eeprom_init();
    lut_manager::init();

    LUT_ERROR err = lut_manager::begin_inquiry(LUT_ID::DEVICE_LUT);
    REQUIRE(err == LUT_ERROR::OKAY);

    // Add configs to test
    /* ST_stepper_type, ST_stepper_type x3
     * ST_HC_Stepper_type, ST_stepper_type x3
     * ST_HC_Stepper_type, ST_HC_Stepper_type x1
     */
    const int LEN = 2 * sizeof(slot_types) + sizeof(uint32_t);
    char buff[LEN];
    uint32_t num = 3;
    slot_types st = ST_Stepper_type;

    memcpy(buff, &st, sizeof(st));
    memcpy(buff + sizeof(st), &st, sizeof(st));
    memcpy(buff + 2*sizeof(slot_types), &num, sizeof(uint32_t));

    err = lut_manager::stream_inquiry(LUT_ID::DEVICE_LUT, buff, LEN);
    REQUIRE(err == LUT_ERROR::OKAY);

    st = ST_HC_Stepper_type;
    memcpy(buff, &st, sizeof(st));
    
    err = lut_manager::stream_inquiry(LUT_ID::DEVICE_LUT, buff, LEN);
    REQUIRE(err == LUT_ERROR::OKAY);

    memcpy(buff + sizeof(st), &st, sizeof(st));
    num = 1;
    memcpy(buff + 2*sizeof(slot_types), &num, sizeof(uint32_t));
    err = lut_manager::stream_inquiry(LUT_ID::DEVICE_LUT, buff, LEN);
    REQUIRE(err == LUT_ERROR::OKAY);

    err = lut_manager::begin_programming(LUT_ID::DEVICE_LUT);
    CHECK_FALSE(lut_manager::has_inquiry(LUT_ID::DEVICE_LUT));
    REQUIRE(err == LUT_ERROR::OKAY);

    err = lut_manager::abort_programming(LUT_ID::DEVICE_LUT);
    // CHECK(lut_manager::get_lut(LUT_ID::DEVICE_LUT)->_state == LUTState::ERASED);
    REQUIRE(err == LUT_ERROR::OKAY);
}

TEST_CASE("Device LUT rejects programming when misaligned")
{
    eeprom_init();
    lut_manager::init();

    LUT_ERROR err = lut_manager::begin_inquiry(LUT_ID::DEVICE_LUT);
    REQUIRE(err == LUT_ERROR::OKAY);

    // Add configs to test
    /* ST_stepper_type, ST_stepper_type x3
     * ST_HC_Stepper_type, ST_stepper_type x3
     * ST_HC_Stepper_type, ST_HC_Stepper_type x1
     */
    const int LEN = 2 * sizeof(slot_types) + sizeof(uint32_t);
    char buff[LEN];
    uint32_t num = 3;
    slot_types st = ST_Stepper_type;

    memcpy(buff, &st, sizeof(st));
    memcpy(buff + sizeof(st), &st, sizeof(st));
    memcpy(buff + 2*sizeof(slot_types), &num, sizeof(uint32_t));

    err = lut_manager::stream_inquiry(LUT_ID::DEVICE_LUT, buff, LEN);
    REQUIRE(err == LUT_ERROR::OKAY);

    st = ST_HC_Stepper_type;
    memcpy(buff + sizeof(st), &st, sizeof(st));
    err = lut_manager::stream_inquiry(LUT_ID::DEVICE_LUT, buff, 4);
    REQUIRE(err == LUT_ERROR::ALIGNMENT);

    err = lut_manager::begin_programming(LUT_ID::DEVICE_LUT);
    CHECK(lut_manager::has_inquiry(LUT_ID::DEVICE_LUT));
    REQUIRE(err == LUT_ERROR::ALIGNMENT);

    err = lut_manager::stream_inquiry(LUT_ID::DEVICE_LUT, buff + 4, LEN - 4);
    REQUIRE(err == LUT_ERROR::OKAY);

    err = lut_manager::begin_programming(LUT_ID::DEVICE_LUT);
    CHECK_FALSE(lut_manager::has_inquiry(LUT_ID::DEVICE_LUT));
    REQUIRE(err == LUT_ERROR::OKAY);

    err = lut_manager::abort_programming(LUT_ID::DEVICE_LUT);
    // CHECK(lut_manager::get_lut(LUT_ID::DEVICE_LUT)->_state == LUTState::ERASED);
    REQUIRE(err == LUT_ERROR::OKAY);
}

static void robust_device_inquiry()
{
    LUT_ERROR err = lut_manager::begin_inquiry(LUT_ID::DEVICE_LUT);
    REQUIRE(err == LUT_ERROR::OKAY);

    const int LEN = 2 * sizeof(slot_types) + sizeof(uint32_t);
    char buff[LEN];

    /**
     * Tree:
     * Root
     *  Stepper
     *      Stepper
     *          x13
     *  HC Stepper
     *      Stepper
     *          x13
     *      HC Stepper
     *          x13
     * 
     * Page count:
     * 2 + 2*9 + 1 = 21 bytes \therefore 12 entries per page.
     * Stepper, Stepper -> 2 data pages + 1 header page = 3 pages
     * 
     * HC Stepper, Stepper -> 2 data pages + 1 header page = 3 pages
     * HC Stepper, HC Stepper -> 2 data pages + 1 header page = 3 pages
     * 
     * 
     * Total: 10 pages
     */

    {
        slot_types st = ST_Stepper_type;
        const uint32_t num = NUM_STEPPER_STEPPER_DEVICES;

        memcpy(buff, &st, sizeof(st));
        memcpy(buff + sizeof(st), &st, sizeof(st));
        memcpy(buff + 2*sizeof(slot_types), &num, sizeof(uint32_t));
        err = lut_manager::stream_inquiry(LUT_ID::DEVICE_LUT, buff, LEN);
        REQUIRE(err == LUT_ERROR::OKAY);
    }

    {
        slot_types st = ST_Stepper_type;
        slot_types st2 = ST_HC_Stepper_type;
        uint32_t num = NUM_HC_STEPPER_STEPPER_DEVICES;

        memcpy(buff, &st2, sizeof(st2));
        memcpy(buff + sizeof(st), &st, sizeof(st));
        memcpy(buff + 2*sizeof(slot_types), &num, sizeof(uint32_t));
        err = lut_manager::stream_inquiry(LUT_ID::DEVICE_LUT, buff, LEN);
        REQUIRE(err == LUT_ERROR::OKAY);

        num = NUM_HC_STEPPER_HC_STEPPER_DEVICES;
        memcpy(buff, &st2, sizeof(st2));
        memcpy(buff + sizeof(st), &st2, sizeof(st2));
        memcpy(buff + 2*sizeof(slot_types), &num, sizeof(uint32_t));
        err = lut_manager::stream_inquiry(LUT_ID::DEVICE_LUT, buff, LEN);
        REQUIRE(err == LUT_ERROR::OKAY);
    }

    err = lut_manager::begin_programming(LUT_ID::DEVICE_LUT);
    REQUIRE(err == LUT_ERROR::OKAY);
}

static void robust_program(bool ignore_verify = false)
{
    LUT_ERROR err;

    // Inquire and begin programming.
    robust_device_inquiry();

    
    // Program the objects.

    char buff[256]; // Just making it really big.
    const slot_types st1 = ST_Stepper_type;
    const slot_types st2 = ST_HC_Stepper_type;

    // Inquiry Repeat
    uint32_t count = NUM_STEPPER_STEPPER_DEVICES;
    memcpy(buff, &st1, sizeof(slot_types));
    memcpy(buff + sizeof(slot_types), &st1, sizeof(slot_types));
    memcpy(buff + 2 * sizeof(slot_types), &count, sizeof(uint32_t));
    err = lut_manager::stream_programming(LUT_ID::DEVICE_LUT, buff, 2 * sizeof(slot_types) + sizeof(uint32_t));
    REQUIRE(err == LUT_ERROR::OKAY);

    count = NUM_HC_STEPPER_STEPPER_DEVICES;
    memcpy(buff, &st2, sizeof(slot_types));
    memcpy(buff + sizeof(slot_types), &st1, sizeof(slot_types));
    memcpy(buff + 2 * sizeof(slot_types), &count, sizeof(uint32_t));
    err = lut_manager::stream_programming(LUT_ID::DEVICE_LUT, buff, 2 * sizeof(slot_types) + sizeof(uint32_t));
    REQUIRE(err == LUT_ERROR::OKAY);

    count = NUM_HC_STEPPER_HC_STEPPER_DEVICES;
    memcpy(buff, &st2, sizeof(slot_types));
    memcpy(buff + sizeof(slot_types), &st2, sizeof(slot_types));
    memcpy(buff + 2 * sizeof(slot_types), &count, sizeof(uint32_t));
    err = lut_manager::stream_programming(LUT_ID::DEVICE_LUT, buff, 2 * sizeof(slot_types) + sizeof(uint32_t));
    REQUIRE(err == LUT_ERROR::OKAY);

    // Programming
    LUTEntry<device_id_t, Stepper_Save_Configs> configs = { 0 };
    for (uint16_t i = 0; i < NUM_STEPPER_STEPPER_DEVICES; ++i)
    {
        const uint8_t CRC_START = CRC_split(reinterpret_cast<const char*>(&st1), sizeof(st1), 
            CRC_split(reinterpret_cast<const char*>(&st1), sizeof(st1), 0));
        configs.key = static_cast<device_id_t>(i);
        configs.crc = (CRC_split(reinterpret_cast<char*>(&configs), sizeof(configs) - sizeof(CRC8_t), CRC_START)) + (ignore_verify ? 1 : 0);
        memcpy(buff, &st1, sizeof(slot_types));
        memcpy(buff + sizeof(slot_types), &st1, sizeof(slot_types));
        memcpy(buff + 2 * sizeof(slot_types), &configs, sizeof(configs));
        err = lut_manager::stream_programming(LUT_ID::DEVICE_LUT, buff, 2 * sizeof(slot_types) + sizeof(configs));
        REQUIRE(err == LUT_ERROR::OKAY);
    }

    for (uint16_t i = 0; i < NUM_HC_STEPPER_STEPPER_DEVICES; ++i)
    {
        const uint8_t CRC_START = CRC_split(reinterpret_cast<const char*>(&st1), sizeof(st1), 
            CRC_split(reinterpret_cast<const char*>(&st2), sizeof(st2), 0));
        configs.key = static_cast<device_id_t>(i);
        configs.crc = (CRC_split(reinterpret_cast<char*>(&configs), sizeof(configs) - sizeof(CRC8_t), CRC_START)) + (ignore_verify ? 1 : 0);
        memcpy(buff, &st2, sizeof(slot_types));
        memcpy(buff + sizeof(slot_types), &st1, sizeof(slot_types));
        memcpy(buff + 2 * sizeof(slot_types), &configs, sizeof(configs));
        err = lut_manager::stream_programming(LUT_ID::DEVICE_LUT, buff, 2 * sizeof(slot_types) + sizeof(configs));
        REQUIRE(err == LUT_ERROR::OKAY);
    }

    for (uint16_t i = 0; i < NUM_HC_STEPPER_HC_STEPPER_DEVICES; ++i)
    {
        const uint8_t CRC_START = CRC_split(reinterpret_cast<const char*>(&st2), sizeof(st2), 
            CRC_split(reinterpret_cast<const char*>(&st2), sizeof(st2), 0));
        configs.key = static_cast<device_id_t>(i);
        configs.crc = (CRC_split(reinterpret_cast<char*>(&configs), sizeof(configs) - sizeof(CRC8_t), CRC_START)) + (ignore_verify ? 1 : 0);
        memcpy(buff, &st2, sizeof(slot_types));
        memcpy(buff + sizeof(slot_types), &st2, sizeof(slot_types));
        memcpy(buff + 2 * sizeof(slot_types), &configs, sizeof(configs));
        err = lut_manager::stream_programming(LUT_ID::DEVICE_LUT, buff, 2 * sizeof(slot_types) + sizeof(configs));
        REQUIRE(err == LUT_ERROR::OKAY);
    }

    // Finish programming
    err = lut_manager::finish_programming(LUT_ID::DEVICE_LUT, nullptr, ignore_verify);
    REQUIRE(err == LUT_ERROR::OKAY);
}

TEST_CASE("Device LUT can process data.")
{
    eeprom_init();
    lut_manager::init();

    SECTION("Valid programming followed")
    {
        robust_program();
    }
}

TEST_CASE("Device LUT can load data")
{
    eeprom_init();
    lut_manager::init();

    LUT_ERROR err;

    SECTION("Device LUT can load data.")
    {
        robust_program();

        Stepper_Save_Configs configs;

        lut_manager::device_lut_key_t key;

        key = lut_manager::device_lut_key_t{
            .running_slot_card=slot_types::ST_Stepper_type,
            .connected_device=device_signature_t{
                .slot_type = slot_types::ST_Stepper_type,
                .device_id = 0x0000
            }
        };
        err = lut_manager::load_data(
            LUT_ID::DEVICE_LUT,
            &key,
            reinterpret_cast<char*>(&configs)
        );
        REQUIRE(err == LUT_ERROR::OKAY);


        key = lut_manager::device_lut_key_t{
            .running_slot_card=slot_types::ST_Stepper_type,
            .connected_device=device_signature_t{
                .slot_type = slot_types::ST_Stepper_type,
                .device_id = 0x000C
            }
        };
        err = lut_manager::load_data(
            LUT_ID::DEVICE_LUT,
            &key,
            reinterpret_cast<char*>(&configs)
        );
        REQUIRE(err == LUT_ERROR::OKAY);


        key = lut_manager::device_lut_key_t{
            .running_slot_card=slot_types::ST_HC_Stepper_type,
            .connected_device=device_signature_t{
                .slot_type = slot_types::ST_Stepper_type,
                .device_id = 0x0007
            }
        };
        err = lut_manager::load_data(
            LUT_ID::DEVICE_LUT,
            &key,
            reinterpret_cast<char*>(&configs)
        );
        REQUIRE(err == LUT_ERROR::OKAY);


        key = lut_manager::device_lut_key_t{
            .running_slot_card=slot_types::ST_HC_Stepper_type,
            .connected_device=device_signature_t{
                .slot_type = slot_types::ST_HC_Stepper_type,
                .device_id = 0x0007
            }
        };
        err = lut_manager::load_data(
            LUT_ID::DEVICE_LUT,
            &key,
            reinterpret_cast<char*>(&configs)
        );
        REQUIRE(err == LUT_ERROR::OKAY);
    }

    SECTION("Device LUT rejects unknown indirection keys")
    {
        robust_program();

        lut_manager::device_lut_key_t key;

        key = lut_manager::device_lut_key_t{
            .running_slot_card = slot_types::Piezo_type,
            .connected_device = device_signature_t{
                .slot_type = slot_types::ST_HC_Stepper_type,
                .device_id = 0x0000
            }
        };
        err = lut_manager::load_data(
            LUT_ID::DEVICE_LUT,
            &key,
            nullptr
        );
        REQUIRE(err == LUT_ERROR::MISSING_KEY);
    }

    SECTION("Device LUT rejects unknown structure keys")
    {
        robust_program();

        lut_manager::device_lut_key_t key = lut_manager::device_lut_key_t{
            .running_slot_card = slot_types::ST_Stepper_type,
            .connected_device = device_signature_t{
                .slot_type = slot_types::ST_HC_Stepper_type,
                .device_id = 0x0000
            }
        };
        err = lut_manager::load_data(
            LUT_ID::DEVICE_LUT,
            &key,
            nullptr
        );
        REQUIRE(err == LUT_ERROR::MISSING_KEY);
    }

    SECTION("Device LUT rejects unknown device ids")
    {
        robust_program();

        lut_manager::device_lut_key_t key = lut_manager::device_lut_key_t{
            .running_slot_card = slot_types::ST_HC_Stepper_type,
            .connected_device = device_signature_t{
                .slot_type = slot_types::ST_HC_Stepper_type,
                .device_id = 0x0010
            }
        };
        err = lut_manager::load_data(
            LUT_ID::DEVICE_LUT,
            &key,
            nullptr
        );
        REQUIRE(err == LUT_ERROR::BAD_DISCRIMINATOR_KEY);
    }

    SECTION("Device LUT rejects non-saving device types")
    {
        robust_program();
        
        lut_manager::device_lut_key_t key = lut_manager::device_lut_key_t{
            .running_slot_card = slot_types::ST_Stepper_type,
            .connected_device = device_signature_t{
                .slot_type = slot_types::OTM_Dac_type,
                .device_id = 0x0000
            }
        };
        err = lut_manager::load_data(
            LUT_ID::DEVICE_LUT,
            &key,
            nullptr
        );
        REQUIRE(err == LUT_ERROR::BAD_STRUCTURE_KEY);
    }

    SECTION("Device LUT rejects invalid CRCs.")
    {
        robust_program(true);
        Stepper_Save_Configs entries;

        lut_manager::device_lut_key_t key = lut_manager::device_lut_key_t{
            .running_slot_card = slot_types::ST_Stepper_type,
            .connected_device = device_signature_t{
                .slot_type = slot_types::ST_Stepper_type,
                .device_id = 0x0000
            }
        };
        err = lut_manager::load_data(
            LUT_ID::DEVICE_LUT,
            &key,
            &entries
        );
        REQUIRE(err == LUT_ERROR::INVALID_ENTRY);
    }
}

TEST_CASE("Device LUT has valid inquiry checks")
{
    eeprom_init();
    lut_manager::init();

    LUT_ERROR err;

    bool result = lut_manager::has_inquiry(LUT_ID::DEVICE_LUT);
    REQUIRE_FALSE(result);

    err = lut_manager::begin_inquiry(LUT_ID::DEVICE_LUT);
    REQUIRE(err == LUT_ERROR::OKAY);

    result = lut_manager::has_inquiry(LUT_ID::DEVICE_LUT);
    REQUIRE(result);

    err = lut_manager::cancel_inquiry(LUT_ID::DEVICE_LUT);
    REQUIRE(err == LUT_ERROR::OKAY);

    result = lut_manager::has_inquiry(LUT_ID::DEVICE_LUT);
    REQUIRE_FALSE(result);
}

TEST_CASE("Device LUT checks out-of-memory properly")
{
    eeprom_init();
    lut_manager::init();

    LUT_ERROR err;

    constexpr uint32_t ENTRY_SIZE = sizeof(device_id_t) + sizeof(CRC8_t) + sizeof(Stepper_Save_Configs);
    constexpr uint32_t ENTRIES_PER_PAGE = LUTM_PAGE_SIZE / ENTRY_SIZE;
    constexpr uint8_t LUTM_HEADER_PAGES = 1;
    constexpr uint16_t LUTM_TOTAL_PAGES = (LUTM_EEPROM_END - LUTM_EEPROM_START) / LUTM_PAGE_SIZE;
    constexpr uint8_t LUTM_IPAGES = 1 + ((LUTM_TOTAL_PAGES - 1) / 8 / LUTM_PAGE_SIZE);
    constexpr uint16_t LUTM_DATA_PAGES = LUTM_TOTAL_PAGES - LUTM_IPAGES - LUTM_HEADER_PAGES;
    constexpr uint32_t AVAILABLE_ENTRY_PAGES = LUTM_DATA_PAGES - 2; // Minus two for the two header pages of Device LUT

    constexpr int LEN = sizeof(uint32_t) + 2*sizeof(slot_types);
    char buff[LEN];

    const slot_types st = ST_Stepper_type;
    const uint32_t MAX_ENTRIES = AVAILABLE_ENTRY_PAGES * ENTRIES_PER_PAGE;
    memcpy(buff, &st, sizeof(st));
    memcpy(buff + sizeof(st), &st, sizeof(st));

    SECTION("Device LUT programs at the max limit.")
    {
        err = lut_manager::begin_inquiry(LUT_ID::DEVICE_LUT);
        REQUIRE(err == LUT_ERROR::OKAY);

        memcpy(buff + 2*sizeof(st), &MAX_ENTRIES, sizeof(MAX_ENTRIES));
        err = lut_manager::stream_inquiry(LUT_ID::DEVICE_LUT, buff, LEN);
        REQUIRE(err == LUT_ERROR::OKAY);

        err = lut_manager::begin_programming(LUT_ID::DEVICE_LUT);
        REQUIRE(err == LUT_ERROR::OKAY);

        lut_manager::abort_programming(LUT_ID::DEVICE_LUT);
    }

    SECTION("Device LUT does not program above the max limit.")
    {
        err = lut_manager::begin_inquiry(LUT_ID::DEVICE_LUT);
        REQUIRE(err == LUT_ERROR::OKAY);

        const uint32_t ABOVE_MAX = MAX_ENTRIES + 1;
        memcpy(buff + 2*sizeof(st), &ABOVE_MAX, sizeof(ABOVE_MAX));
        err = lut_manager::stream_inquiry(LUT_ID::DEVICE_LUT, buff, LEN);
        REQUIRE(err == LUT_ERROR::OKAY);

        err = lut_manager::begin_programming(LUT_ID::DEVICE_LUT);
        REQUIRE(err == LUT_ERROR::OUT_OF_MEMORY);
    }
}

void setup_device_inquiry_buffer(char * const p_buff, const slot_types st, const uint32_t entries)
{
    memcpy(p_buff, &st, sizeof(st));
    memcpy(p_buff + sizeof(st), &st, sizeof(st));
    memcpy(p_buff + 2 * sizeof(st), &entries, sizeof(entries));
}

void setup_config_inquiry_buffer(char * const p_buff, const struct_id_t id, const uint32_t entries)
{
    memcpy(p_buff, &id, sizeof(id));
    memcpy(p_buff + sizeof(id), &entries, sizeof(entries));
}

TEST_CASE("Multi-LUT allocation")
{
    eeprom_init();
    lut_manager::init();

    LUT_ERROR err;

    constexpr uint32_t DEVICE_ENTRY_SIZE = sizeof(device_id_t) + sizeof(CRC8_t) + sizeof(Stepper_Save_Configs);

    constexpr uint32_t CONFIG_ENTRY_SIZE = sizeof(config_id_t) + sizeof(CRC8_t) + sizeof(Stepper_Config);

    constexpr uint8_t LUTM_HEADER_PAGES = 1;
    constexpr uint16_t LUTM_TOTAL_PAGES = (LUTM_EEPROM_END - LUTM_EEPROM_START) / LUTM_PAGE_SIZE;
    constexpr uint8_t LUTM_IPAGES = 1 + ((LUTM_TOTAL_PAGES - 1) / 8 / LUTM_PAGE_SIZE);
    constexpr uint16_t LUTM_DATA_PAGES = LUTM_TOTAL_PAGES - LUTM_IPAGES - LUTM_HEADER_PAGES;

    constexpr int DEVICE_LEN = 2 * sizeof(slot_types) + sizeof(uint32_t);
    constexpr int CONFIG_LEN = sizeof(struct_id_t) + sizeof(uint32_t);
    constexpr int LEN = (DEVICE_LEN > CONFIG_LEN) ? DEVICE_LEN : CONFIG_LEN;

    char buff[LEN];

    SECTION("Config first, Device second, fast allocation mode")
    {
        err = lut_manager::begin_inquiry(LUT_ID::CONFIG_LUT);
        REQUIRE(err == LUT_ERROR::OKAY);
        err = lut_manager::begin_inquiry(LUT_ID::DEVICE_LUT);
        REQUIRE(err == LUT_ERROR::OKAY);

        setup_config_inquiry_buffer(buff, 2, 30);
        err = lut_manager::stream_inquiry(LUT_ID::CONFIG_LUT, buff, CONFIG_LEN);
        REQUIRE(err == LUT_ERROR::OKAY);

        setup_device_inquiry_buffer(buff, ST_Stepper_type, 30);
        err = lut_manager::stream_inquiry(LUT_ID::DEVICE_LUT, buff, DEVICE_LEN);
        REQUIRE(err == LUT_ERROR::OKAY);

        err = lut_manager::begin_programming(LUT_ID::CONFIG_LUT);
        REQUIRE(err == LUT_ERROR::OKAY);

        err = lut_manager::begin_programming(LUT_ID::DEVICE_LUT);
        REQUIRE(err == LUT_ERROR::OKAY);

        lut_manager::clear();
    }

 
    SECTION("Device first, Config second, fast allocation mode")
    {
        err = lut_manager::begin_inquiry(LUT_ID::CONFIG_LUT);
        REQUIRE(err == LUT_ERROR::OKAY);
        err = lut_manager::begin_inquiry(LUT_ID::DEVICE_LUT);
        REQUIRE(err == LUT_ERROR::OKAY);

        setup_config_inquiry_buffer(buff, 2, 30);
        err = lut_manager::stream_inquiry(LUT_ID::CONFIG_LUT, buff, CONFIG_LEN);
        REQUIRE(err == LUT_ERROR::OKAY);

        setup_device_inquiry_buffer(buff, ST_Stepper_type, 30);
        err = lut_manager::stream_inquiry(LUT_ID::DEVICE_LUT, buff, DEVICE_LEN);
        REQUIRE(err == LUT_ERROR::OKAY);

        err = lut_manager::begin_programming(LUT_ID::DEVICE_LUT);
        REQUIRE(err == LUT_ERROR::OKAY);

        err = lut_manager::begin_programming(LUT_ID::CONFIG_LUT);
        REQUIRE(err == LUT_ERROR::OKAY);

        lut_manager::clear();
    }

    SECTION("Device first, Config second, device smaller resize")
    {
        robust_program();

        err = lut_manager::begin_inquiry(LUT_ID::CONFIG_LUT);
        REQUIRE(err == LUT_ERROR::OKAY);

        setup_config_inquiry_buffer(buff, 2, 30);
        err = lut_manager::stream_inquiry(LUT_ID::CONFIG_LUT, buff, CONFIG_LEN);
        REQUIRE(err == LUT_ERROR::OKAY);

        err = lut_manager::begin_programming(LUT_ID::CONFIG_LUT);
        REQUIRE(err == LUT_ERROR::OKAY);

        err = lut_manager::begin_inquiry(LUT_ID::DEVICE_LUT);
        REQUIRE(err == LUT_ERROR::OKAY);

        setup_device_inquiry_buffer(buff, ST_Stepper_type, 1);
        err = lut_manager::stream_inquiry(LUT_ID::DEVICE_LUT, buff, DEVICE_LEN);
        REQUIRE(err == LUT_ERROR::OKAY);

        err = lut_manager::begin_programming(LUT_ID::DEVICE_LUT);
        REQUIRE(err == LUT_ERROR::OKAY);

        lut_manager::clear();
    }
}

//EOF
