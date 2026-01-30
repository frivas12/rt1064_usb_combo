// efs_tests.cc

/**************************************************************************//**
 * \file efs_tests.cc
 * \author Sean Benish
 * \brief Unit tests for efs.hh
 *****************************************************************************/

#include <catch_amalgamated.hpp>

#include "efs-cache.hh"
#include "efs.hh"
#include "25lc1024.h"

#include "emulation_help.hh"

constexpr std::size_t DATA_PAGE_START = 2;

/*****************************************************************************
 * Private Function Prototypes
 *****************************************************************************/

/*****************************************************************************
 * Unit Tests
 *****************************************************************************/
TEST_CASE("EFS Header Information is Valid")
{
    const efs::efs_header HEADER = efs::get_header_info();

    CHECK(memcmp(HEADER.identifier, "EFS", 3) == 0);
    CHECK(HEADER.version == 0);
    CHECK(HEADER.eeprom_start_address == EFS_EEPROM_START);
    CHECK(HEADER.header_pages == 1);
    CHECK(HEADER.page_count == 256);
    CHECK(HEADER.page_size == EEPROM_25LC1024_PAGE_SIZE);
}

TEST_CASE("EFS File Creation and Existance")
{
    eeprom_init();
    efs::init();

    bool passed_test = false;

    // Creates a file in data pages: [0, 1)
    efs::file_identifier_t id = static_cast<efs::file_identifier_t>(0);
    CHECK(efs::create_file(id, 1));
    efs::file_metadata metadata = efs::get_file_metadata(id);
    CHECK(metadata.id == id);
    CHECK(metadata.attr == efs::DEFAULT_ATTRIBUTES);
    CHECK(metadata.length == 1);
    CHECK(metadata.start == DATA_PAGE_START);
    passed_test = true;
    CHECK(efs::does_file_exist(id));

    // Creates a file in data pages: [1, 5)
    efs::file_attributes_t attr = efs::FILE_ATTRIBUTES_EXTERNAL_FULL_ACCESS;
    id = static_cast<efs::file_identifier_t>(1);
    CHECK(efs::create_file(id, 4, attr));
    metadata = efs::get_file_metadata(id);
    CHECK(metadata.id == id);
    CHECK(metadata.attr == attr);
    CHECK(metadata.length == 4);
    CHECK(metadata.start == DATA_PAGE_START + 1);
    CHECK(efs::does_file_exist(id));

    // Page count is too large.
    CHECK_FALSE(efs::create_file(static_cast<efs::file_identifier_t>(2), 0xFFF0));
    CHECK_FALSE(efs::does_file_exist(static_cast<efs::file_identifier_t>(2)));

    // File already exists.
    REQUIRE(passed_test);
    CHECK_FALSE(efs::create_file(static_cast<efs::file_identifier_t>(0), 10));
}

TEST_CASE("EFS Handles")
{
    eeprom_init();
    efs::init();

    const efs::file_identifier_t ID = static_cast<efs::file_identifier_t>(0);
    const uint16_t PAGES = 10 * EEPROM_25LC1024_PAGE_SIZE / EFS_PAGE_SIZE;
    const efs::file_attributes_t ATTR = efs::FILE_ATTRIBUTES_EXTERNAL_FULL_ACCESS | efs::FILE_ATTRIBUTES_MASK_FIRMWARE_READ;
    REQUIRE(efs::create_file(ID, PAGES, ATTR));

    efs::handle handle = efs::get_handle(ID, portMAX_DELAY, efs::user_e::SUPER);
    REQUIRE(handle.is_valid());

    // Check SUPER access rights.
    CHECK(handle.can_read());
    CHECK(handle.can_write());
    CHECK(handle.can_delete());

    char buffer[256];
    cycler(buffer, 256);

    const uint32_t ADDRESSES[] = {0, EEPROM_25LC1024_PAGE_SIZE + 1, PAGES*EFS_PAGE_SIZE - 10};

    /**
     * Write data to handle.
     * Testing...
     * 0.   Normal writing
     * 1.   Writing across an EEPROM page boundary.
     * 2.   Writing to the end of the file.
     */
    REQUIRE(handle.write(ADDRESSES[0], buffer, 256) == 256);
    REQUIRE(handle.write(ADDRESSES[1], buffer, 256) == 256);
    REQUIRE(handle.write(ADDRESSES[2], buffer, 256) == 10);

    /**
     * Reading data from handle.
     * Testing...
     * 0.   Normal reading
     * 1.   Reading across an EEPROM page boundary.
     * 2.   Reading to the end of the file.
     */
    char tmp[256];
    REQUIRE(handle.read(ADDRESSES[0], tmp, 256) == 256);
    REQUIRE(memcmp(tmp, buffer, 256) == 0);

    memset(tmp, 0xFF, 256);
    REQUIRE(handle.read(ADDRESSES[1], tmp, 256) == 256);
    REQUIRE(memcmp(tmp, buffer, 256) == 0);

    memset(tmp, 0xFF, 256);
    REQUIRE(handle.read(ADDRESSES[2], tmp, 256) == 10);
    REQUIRE(memcmp(tmp, buffer, 10) == 0);

    // Testing handle exclusion.
    efs::handle bad_handle = efs::get_handle(ID, 0);
    CHECK_FALSE(bad_handle.is_valid());

    // Close handle for someone else's use.
    handle.close();
    REQUIRE_FALSE(handle.is_valid());

    handle = efs::get_handle(ID, portMAX_DELAY);
    REQUIRE(handle.is_valid());

    // Check FIRMWARE access rights.
    CHECK(handle.can_read());
    CHECK_FALSE(handle.can_write());
    CHECK_FALSE(handle.can_delete());

    // Verify write rejection.
    REQUIRE(handle.write(ADDRESSES[1] + 1, buffer, 256) == 0);

    // Verify reading still gives the correct data.
    memset(tmp, 0xFF, 256);
    REQUIRE(handle.read(ADDRESSES[0], tmp, 256) == 256);
    REQUIRE(memcmp(tmp, buffer, 256) == 0);

    // Verify deletion fails.
    REQUIRE_FALSE(handle.delete_file());

    handle.close();
    REQUIRE_FALSE(handle.is_valid());

    handle = efs::get_handle(ID, portMAX_DELAY, efs::EXTERNAL);
    REQUIRE(handle.is_valid());

    // Check EXTERNAL access rights.
    CHECK(handle.can_read());
    CHECK(handle.can_write());
    CHECK(handle.can_delete());

    // Verify deletion works.
    REQUIRE(handle.delete_file());
    REQUIRE_FALSE(efs::does_file_exist(ID));
}

TEST_CASE("EFS Metadata Cache")
{
    efs::metadata_cache cache;

    efs::file_metadata md;
    cycler(reinterpret_cast<char*>(&md), sizeof(md));
    md.attr |= efs::FILE_ATTRIBUTES_MASK_NOT_ALLOCATED;
    cache.cache(md);

    CHECK(cache.is_valid());
    md.attr &= ~efs::FILE_ATTRIBUTES_MASK_NOT_ALLOCATED;
    REQUIRE(memcmp(&md, &cache.get_metadata(), sizeof(md)) == 0);

    md.attr = static_cast<efs::file_attributes_t>(md.attr & ~efs::FILE_ATTRIBUTES_MASK_NOT_ALLOCATED);
    cache.cache(md);
    CHECK(cache.is_valid());

    cache.invalidate();
    CHECK_FALSE(cache.is_valid());
}

TEST_CASE("EFS Metadata Cache Pool")
{
    constexpr std::size_t LEN = 2;
    efs::file_identifier_t ids[LEN] = {0, 1};

    efs::metadata_cache_pool<LEN> pool;
    pool.clear();

    efs::file_metadata md;
    CHECK_FALSE(pool.get_metadata(ids[0], md));
    CHECK_FALSE(pool.get_metadata(ids[1], md));

    const efs::file_metadata FILE_0 = efs::file_metadata{ids[0], efs::FILE_ATTRIBUTES_EXTERNAL_FULL_ACCESS, 0, 10};
    pool.add(FILE_0);
    CHECK(pool.get_metadata(ids[0], md));
    CHECK(memcmp(&md, &FILE_0, sizeof(md)) == 0);
    CHECK_FALSE(pool.get_metadata(ids[1], md));

    const efs::file_metadata FILE_1 = efs::file_metadata{ids[1], efs::FILE_ATTRIBUTES_FIRMWARE_FULL_ACCESS, 80, 50};
    pool.add(FILE_1);
    CHECK(pool.get_metadata(ids[0], md));
    CHECK(memcmp(&md, &FILE_0, sizeof(md)) == 0);
    CHECK(pool.get_metadata(ids[1], md));
    CHECK(memcmp(&md, &FILE_1, sizeof(md)) == 0);

    const efs::file_metadata FILE_2 = efs::file_metadata{0xFF, 0, 20, 3};
    pool.add(FILE_2);
    CHECK_FALSE(pool.get_metadata(ids[0], md));
    CHECK(pool.get_metadata(ids[1], md));
    CHECK(memcmp(&md, &FILE_1, sizeof(md)) == 0);
    CHECK(pool.get_metadata(0xFF, md));
    CHECK(memcmp(&md, &FILE_2, sizeof(md)) == 0);

    pool.remove(ids[1]);
    CHECK_FALSE(pool.get_metadata(ids[1], md));
}

TEST_CASE("EFS File Cache")
{
    constexpr efs::file_identifier_t ID = 10;

    efs::file_cache lazy_cache(ID, true);

    CHECK(lazy_cache.get_file_id() == ID);
    CHECK_FALSE(lazy_cache.has_ownership());



    eeprom_init();
    efs::init();

    efs::file_cache cache(ID);
    CHECK(cache.get_file_id() == ID);
    CHECK_FALSE(cache.has_ownership());
    CHECK_FALSE(cache.get_handle(false).is_valid());

    REQUIRE(efs::create_file(ID, 1, efs::FILE_ATTRIBUTES_EXTERNAL_FULL_ACCESS, 0));

    {
        efs::handle handy = efs::get_handle(ID, 0);
        CHECK(handy.is_valid());

        CHECK_FALSE(cache.get_handle(false).is_valid());
    }
    {
        efs::cache_handle handy = cache.get_handle(false);
        CHECK(handy.is_valid());


        CHECK_FALSE(lazy_cache.get_handle(false).is_valid());
    }

    REQUIRE(cache.take_ownership());
    CHECK(cache.has_ownership());

    {
        efs::cache_handle handy = cache.get_handle(false);
        CHECK(handy.is_valid());

        CHECK(cache.has_ownership());
    }
    {
        efs::cache_handle handy = cache.get_handle(false);
        CHECK(handy.is_valid());
        handy.close();
        CHECK_FALSE(handy.is_valid());

        efs::cache_handle handy2 = lazy_cache.get_handle(false);
        CHECK_FALSE(handy.is_valid());
    }
    CHECK_FALSE(lazy_cache.get_handle(false).is_valid());

    cache.release_ownership();
    CHECK_FALSE(cache.has_ownership());
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/

//EOF
