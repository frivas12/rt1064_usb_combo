#pragma once

#include <cstdint>


enum class hid_tag_e : uint8_t
{
    // Main Items
    INPUT               = 0b10000000,
    OUTPUT              = 0b10010000,
    FEATURE             = 0b10110000,
    COLLECTION          = 0b10100000,
    END_COLLECTION      = 0b11000000,

    // Global Items
    USAGE_PAGE          = 0b00000100,
    LOGICAL_MINIMUM     = 0b00010100,
    LOGICAL_MAXIMUM     = 0b00100100,
    PHYSICAL_MINIMUM    = 0b00110100,
    PHYSICAL_MAXIMUM    = 0b01000100,
    UNIT_EXPONENT       = 0b01010100,
    UNIT                = 0b01100100,
    REPORT_SIZE         = 0b01110100,
    REPORT_ID           = 0b10000100,
    REPORT_COUNT        = 0b10010100,
    PUSH                = 0b10100100,
    POP                 = 0b10110100,
    // RESERVED

    // Local Items
    USAGE               = 0b00001000,
    USAGE_MINIMUM       = 0b00011000,
    USAGE_MAXIMUM       = 0b00101000,
    DESIGNATOR_INDEX    = 0b00111000,
    DESIGNATOR_MINIMUM  = 0b01001000,
    DESIGNATOR_MAXIMUM  = 0b01011000,
    STRING_INDEX        = 0b01111000,
    STRING_MINIMUM      = 0b10001000,
    STRING_MAXIMUM      = 0b10011000,
    DELIMITER           = 0b10101000,
    // Reserved
};

enum class hid_type_e : uint8_t
{
    MAIN    = 0b00000000,
    GLOBAL  = 0b00000100,
    LOCAL   = 0b00001000
    // Reserved is not defined.
};

/**
 * Class for decoding an HID report stream.
 */
class hid_item_parser
{
private:

    const uint8_t *         _p_head;
    const uint8_t * const   _p_tail;

public:

    /**
     * Constructs an HID parser from a binary stream and constrains it to a memory location.
     * \param p_stream The binary stream to read in.
     * \param p_stream_end The byte after the binary stream.
     */
    hid_item_parser(const void * const p_stream, const void * const p_stream_end);
    
    /**
     * Constructs an HID parser from a binary stream and constrains it to a number of bytes.
     * \param p_stream The binary stream to read in.
     * \param stream_size The size of the stream in bytes.
     */
    hid_item_parser(const void * const p_stream, const std::size_t stream_size);

    /**
     * Constructs an HID parser from another parser at the base parser's current location.
     * \param base The HID parser to copy.
     */
    hid_item_parser(const hid_item_parser& base);



    /**
     * Advances the parser to the next item.
     * \return true A new item is available.
     * \return false The stream is at the end.
     */
    bool next_item();

    /**
     * Indicates if the parser is at the end of the stream.
     * \return true The parser is at the end of the stream.
     * \return false The parser is not at the end of the stream.
     */
    bool is_eof() const;

    /**
     * Gets the size of the current item.
     * \warning The stream may not be at the end.
     * \return std::size_t The size of the current item in bytes.
     */
    std::size_t get_item_size() const;

    /**
     * Gets the tag of the current item.
     * \warning The stream may not be at the end.
     * \return hid_tag_e The tag (bTag | bType) of the current item.
     */
    hid_tag_e get_item_tag() const;

    /**
     * Gets the type of the current item.
     * \warning The stream may not be at the end.
     * \return hid_type_e The hid type (bType) of the curren titem.
     */
    hid_type_e get_item_type() const;


    /**
     * Gets the raw value of the item header.
     * \warning The stream may not be at the end.
     * \return uint8_t The raw binary header of the HID item.
     */
    uint8_t get_header() const;

    /**
     * Gets the type of the current item.
     * \warning The stream may not be at the end.
     * \return uint8_t The data of the item stored in a 32-bit value.
     */
    uint32_t get_data() const;

    /**
     * Gets the type of the current item.
     * \warning The stream may not be at the end.
     * \warning The current item must have a size equal to or greater than 1.
     * \return uint8_t The data of the item.
     */
    uint8_t get_data_as_uint8() const;

    /**
     * Gets the type of the current item.
     * \warning The stream may not be at the end.
     * \warning The current item must have a size equal to or greater than 2.
     * \return uint16_t The data of the item.
     */
    uint16_t get_data_as_uint16() const;

    /**
     * Gets the type of the current item.
     * \warning The stream may not be at the end.
     * \warning The current item must have a size equal to or greater than 4.
     * \return uint32_t The data of the item.
     */
    uint32_t get_data_as_uint32() const;
};

// EOF
