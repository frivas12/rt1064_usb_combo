/**
 * \file channels.hh
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 2024-05-22
 */
#pragma once

#include <concepts>
#include <numeric>
#include <ranges>
#include <tuple>

namespace utils {

/// \brief Double-ended inclusive range (aka [x, y])
template <std::integral ChannelType>
using channel_range = std::tuple<ChannelType, ChannelType>;

template <std::integral ChannelType>
constexpr bool is_channel_in_range(ChannelType channel,
                                   const channel_range<ChannelType> &range) {
    return channel >= std::get<0>(range) && channel <= std::get<1>(range);
}

template <std::integral ChannelType>
constexpr ChannelType
index_to_channel(std::size_t index, const channel_range<ChannelType> &range) {
    return static_cast<ChannelType>(index + std::get<0>(range));
}

template <std::integral ChannelType>
constexpr std::size_t
channel_to_index(ChannelType channel, const channel_range<ChannelType> &range) {
    return static_cast<std::size_t>(channel - std::get<0>(range));
}

template <std::integral ChannelType>
constexpr ChannelType first(const channel_range<ChannelType> &range) {
    return std::get<0>(range);
}

template <std::integral ChannelType>
constexpr ChannelType last(const channel_range<ChannelType> &range) {
    return std::get<1>(range);
}

template <std::integral ChannelType>
constexpr std::ranges::range auto
as_iterable(const channel_range<ChannelType> &range) {
    return std::views::iota(first(range),
                            static_cast<ChannelType>(last(range) + 1));
}

} // namespace utils

namespace std {

template <integral ChannelType>
constexpr size_t size(const utils::channel_range<ChannelType> &range) {
    return get<1>(range) - get<0>(range) + 1;
}

} // namespace std

// EOF
