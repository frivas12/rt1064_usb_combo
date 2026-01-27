#pragma once

#include <array>
#include <cstddef>

/**
* An unordered list that has a maximum capacity.
*/
template <typename T, std::size_t Extent>
class inline_unordered_vector {
   public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = T*;
    using const_pointer = T*;
    using iterator = std::array<T, Extent>::iterator;
    using const_iterator = std::array<T, Extent>::const_iterator;
    using reverse_iterator = std::array<T, Extent>::reverse_iterator;
    using const_reverse_iterator = std::array<T, Extent>::const_reverse_iterator;


    auto begin() { return _items.begin(); }
    auto end() { return _items.begin() + _tail; }

    auto begin() const { return _items.begin(); }
    auto end() const { return _items.begin() + _tail; }

    auto cbegin() const { return _items.cbegin(); }
    auto cend() const { return _items.cbegin() + _tail; }

    constexpr std::size_t capacity() const { return Extent; }
    std::size_t size() const { return _tail; }

    bool insert(const T& item) {
        if (_tail == Extent) {
            return false;
        }

        _items[_tail++] = item;
        return true;
    }

    bool erase(const T& item) {
        if (_tail == 0) {
            return false;
        }

        --_tail;
        if (_items[_tail] == item) {
            return true;
        }

        // The item at the new tail is still valid,
        // so find the removed item and swap it.
        for (std::size_t i = 0; i < _tail; ++i) {
            if (_items[i] == item) {
                _items[i] = _items[_tail];
                return true;
            }
        }

        // Did not find the item, so increment
        // the tail to mark the current item as valid.
        ++_tail;
        return false;
    }

   private:
    std::array<T, Extent> _items;
    std::size_t _tail;
};
