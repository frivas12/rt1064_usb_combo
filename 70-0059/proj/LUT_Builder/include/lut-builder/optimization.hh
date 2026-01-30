// optimization.hh

/**************************************************************************//**
 * \file optimization.hh
 * \author Sean Benish
 * \brief 
 *
 * 
 *****************************************************************************/
#pragma once

#include <cstdint>
#include <limits>
#include <utility>

namespace optimization
{

/*****************************************************************************
 * Defines
 *****************************************************************************/

/*****************************************************************************
 * Data Types
 *****************************************************************************/

class opt_base
{
public:
    virtual std::size_t min_size() const = 0;
    virtual std::size_t max_size() const = 0;
    virtual uint16_t get_page_count(const uint16_t lut_page_size) const = 0;
};

class lut_header : public opt_base
{
private:
    const std::size_t _key_size;
    const std::size_t _key_count;
public:
    lut_header(const std::size_t key_size, const std::size_t key_count);

    std::size_t min_size() const override;
    std::size_t max_size() const override;
    uint16_t get_page_count(const uint16_t lut_page_size) const override;

};

class lut_entries : public opt_base
{
private:
    const std::size_t _entry_size;
    const std::size_t _count;
public:
    lut_entries(const std::size_t entry_size, const std::size_t entry_count);

    std::size_t min_size() const override;
    std::size_t max_size() const override;
    uint16_t get_page_count(const uint16_t lut_page_size) const override;

};

class global_lut_header : public opt_base
{
private:
    const std::size_t _keys;
public:
    global_lut_header(const std::size_t key_count);

    std::size_t min_size() const override;
    std::size_t max_size() const override;
    uint16_t get_page_count(const uint16_t lut_page_size) const override;
};

/*****************************************************************************
 * Constants
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/

/**
 * Returns the minimum and maximum (performant) LUT page size for a list of
 * LUT items.
 * \param[in]       begin Iterator to the start of the LUT items.
 * \param[in]       end Iterator to the end of the LUT items.
 * \return A pair of [min, max] bounds.
 */
template<typename ITR>
std::pair<std::size_t, std::size_t> get_bounds(const ITR begin, const ITR end)
{
    std::size_t min = std::numeric_limits<std::size_t>::min();
    std::size_t max = std::numeric_limits<std::size_t>::min();

    for (ITR itr = begin; itr != end; ++itr)
    {
        min = ((*itr)->min_size() > min) ? (*itr)->min_size() : min;
        max = ((*itr)->max_size() > max) ? (*itr)->max_size() : max;
    }

    return std::pair(min,max);
}

/**
 * Optimizes the LUT to use as little space as possible.
 * \param[in]       begin Iterator to the start of the optimization objects.
 * \param[in]       end Iterator to the end of the optimization objects.
 * \return The lut page size that minimizes the amount of space taken up.
 */
template<typename ITR>
uint16_t optimize(const ITR begin, const ITR end)
{
    // Find bounds.
    auto [min, max] = get_bounds(begin, end);


    // Iterate over bounds.
    std::size_t min_page_count = std::numeric_limits<std::size_t>::max();
    std::size_t min_page_count_page_size = 0;
    for (std::size_t page_size = min; min <= max; ++min)
    {
        std::size_t acc = 0;
        for (ITR itr = begin; itr != end; ++itr)
        {
            acc += (*itr)->get_page_count(page_size);
        }

        if (min_page_count > acc)
        {
            min_page_count = acc;
            min_page_count_page_size = page_size;
        }
    }

    return min_page_count_page_size;
}

}

//EOF
