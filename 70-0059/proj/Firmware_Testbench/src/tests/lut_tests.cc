#include "catch_amalgamated.hpp"

#include "indirect_lut.tcc"
#include "defs.hh"
#include "slot_types.h"
#include "lut_types.hh"
#include "efs.hh"
#include "save_constructor.hh"
#include "structure_id.hh"
#include "stepper_saves.h"

#include <fstream>
#include <filesystem>
#include <memory>

TEST_CASE("Device LUT")
{
    eeprom_init();
    efs::init();

    const std::filesystem::path FILE_TO_LOAD = "device.bin";
    const uintmax_t FILE_SIZE = std::filesystem::file_size(FILE_TO_LOAD);

    const auto hw_info = efs::get_header_info();
    const std::size_t PAGE_SIZE = hw_info.page_size;

    const auto ID = efs::create_id(0, efs::file_types_e::LUT);
    const uint16_t PAGES_TO_OCCUPY = static_cast<uint16_t>((FILE_SIZE + PAGE_SIZE - 1) / PAGE_SIZE);
    REQUIRE(efs::create_file(ID, PAGES_TO_OCCUPY));

    efs::handle handy = efs::get_handle(ID);
    REQUIRE(handy.is_valid());

    {
        std::unique_ptr<char> p_data{new char[PAGE_SIZE * PAGES_TO_OCCUPY]};

        std::fstream file;
        file.open(FILE_TO_LOAD, std::ios::in | std::ios::binary);
        REQUIRE(file.is_open());
        file.read(p_data.get(), static_cast<std::streamsize>(FILE_SIZE));

        handy.write(0, p_data.get(), FILE_SIZE);
    }

    IndirectLUT<slot_types, slot_types, device_id_t> lut(LUT_ID::DEVICE_LUT, &save_constructor::get_slot_save_size);


    Stepper_Save_Union u;
    LUT_ERROR error = lut.load_data(handy, slot_types::MCM_Stepper_LC_HD_DB15, slot_types::MCM_Stepper_LC_HD_DB15,
            0, reinterpret_cast<char*>(&u));
    REQUIRE(error == LUT_ERROR::OKAY);

}

TEST_CASE("Config LUT")
{
    eeprom_init();
    efs::init();

    const std::filesystem::path FILE_TO_LOAD = "config.bin";
    const uintmax_t FILE_SIZE = std::filesystem::file_size(FILE_TO_LOAD);

    const auto hw_info = efs::get_header_info();
    const std::size_t PAGE_SIZE = hw_info.page_size;

    const auto ID = efs::create_id(1, efs::file_types_e::LUT);
    const uint16_t PAGES_TO_OCCUPY = static_cast<uint16_t>((FILE_SIZE + PAGE_SIZE - 1) / PAGE_SIZE);
    REQUIRE(efs::create_file(ID, PAGES_TO_OCCUPY));

    efs::handle handy = efs::get_handle(ID);
    REQUIRE(handy.is_valid());

    {
        std::unique_ptr<char> p_data{new char[PAGE_SIZE * PAGES_TO_OCCUPY]};

        std::fstream file;
        file.open(FILE_TO_LOAD, std::ios::in | std::ios::binary);
        REQUIRE(file.is_open());
        file.read(p_data.get(), static_cast<std::streamsize>(FILE_SIZE));
        REQUIRE(file.good());

        handy.write(0, p_data.get(), FILE_SIZE);
    }

    LUT<struct_id_t, config_id_t> lut(LUT_ID::CONFIG_LUT, &save_constructor::get_config_size);


    Stepper_drive_params u;
    LUT_ERROR error = lut.load_data(handy, static_cast<struct_id_t>(Struct_ID::STEPPER_DRIVE),
        0, reinterpret_cast<char*>(&u));
    REQUIRE(error == LUT_ERROR::OKAY);
}
