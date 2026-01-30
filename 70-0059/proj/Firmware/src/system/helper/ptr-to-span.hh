#pragma once

#include <cstddef>
#include <ranges>
#include <span>

template<typename T>
std::span<T, std::dynamic_extent> ptr2span(T* const ptr, std::size_t length) {
    return std::span(std::ranges::subrange(ptr, ptr + length));
}

template<std::size_t Extent, typename T>
    requires (Extent != std::dynamic_extent)
std::span<T, Extent> ptr2span(T* const ptr) {
    return ptr2span(ptr, Extent).template subspan<0, Extent>();
}
