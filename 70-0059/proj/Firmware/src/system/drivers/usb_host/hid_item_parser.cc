#include "hid_item_parser.hh"

hid_item_parser::hid_item_parser(const void * const p_stream, const void * const p_stream_end)
    : _p_head(reinterpret_cast<const uint8_t*>(p_stream)),
      _p_tail(reinterpret_cast<const uint8_t*>(p_stream_end))
{}

hid_item_parser::hid_item_parser(const void * const p_stream, const std::size_t stream_size)
    : _p_head(reinterpret_cast<const uint8_t*>(p_stream)),
      _p_tail(reinterpret_cast<const uint8_t*>(p_stream) + stream_size)
{}

hid_item_parser::hid_item_parser(const hid_item_parser& r_base)
    : _p_head(r_base._p_head), _p_tail(r_base._p_tail)
{}



bool hid_item_parser::next_item()
{
    if (is_eof())
    {
        return false;
    }

    // Data size + header byte
    _p_head += get_item_size() + 1;

    return true;
}

bool hid_item_parser::is_eof() const
{
    return _p_head >= _p_tail;
}

std::size_t hid_item_parser::get_item_size() const
{
    constexpr uint8_t SIZE_MASK = 0x03;

    const uint8_t VALUE = _p_head[0] & SIZE_MASK;
    return (VALUE == 0) ? 0 : (1 << (VALUE - 1));
}

hid_tag_e hid_item_parser::get_item_tag() const
{
    constexpr uint8_t MASK = 0xFC;

    return static_cast<hid_tag_e>(_p_head[0] & MASK);
}

hid_type_e hid_item_parser::get_item_type() const
{
    constexpr uint8_t MASK = 0x0C;

    return static_cast<hid_type_e>(_p_head[0] & MASK);
}

uint8_t hid_item_parser::get_header() const
{
    return _p_head[0];
}

uint32_t hid_item_parser::get_data() const
{
    uint32_t rt = 0;
    const std::size_t SIZE = get_item_size();

    for (std::size_t i = SIZE; i > 0; --i)
    {
        rt = (rt << 8) | _p_head[i];
    }

    return rt;
}

uint8_t hid_item_parser::get_data_as_uint8() const
{
    return _p_head[1];
}

uint16_t hid_item_parser::get_data_as_uint16() const
{
    return _p_head[1] | (_p_head[2] << 8);
}

uint32_t hid_item_parser::get_data_as_uint32() const
{
    return _p_head[1] | (_p_head[2] << 8) | (_p_head[3] << 16) | (_p_head[4] << 24);
}

// EOF
