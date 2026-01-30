#pragma once

#include <array>
#include <cstddef>

#include "inline-unordered-vector.hh"

/**
 * An unordered set that has a maximum capacity.
 * add() will fail if the set already contains the value.
 */
template <typename T, std::size_t Extent>
class inline_unordered_set {
   public:
    using value_type = T;

    auto begin() { return _vector.begin(); }
    auto end() { return _vector.end(); }

    auto cbegin() const { return _vector.cbegin(); }
    auto cend() const { return _vector.cend(); }

    constexpr std::size_t capacity() const { return _vector.capacity(); }
    std::size_t size() const { return _vector.size(); }

    bool add(const T& item) {
        for (T& stored : _vector) {
            if (stored == item) {
                return false;
            }
        }

        return _vector.add(item);
    }

    bool remove(const T& item) { return _vector.remove(item); }

   private:
    inline_unordered_vector<T, Extent> _vector;
};
