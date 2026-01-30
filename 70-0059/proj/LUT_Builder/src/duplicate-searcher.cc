// duplicate-searcher.cc

/**************************************************************************//**
 * \file duplicate-searcher.cc
 * \author Sean Benish
 *****************************************************************************/
#include "lut-builder/duplicate-searcher.hh"

/*****************************************************************************
 * Constants
 *****************************************************************************/

/*****************************************************************************
 * Macros
 *****************************************************************************/

/*****************************************************************************
 * Data Types
 *****************************************************************************/

/*****************************************************************************
 * Private Function Prototypes
 *****************************************************************************/
std::size_t hash_function(const std::vector<uint8_t>& vector) noexcept;

hash_set find_duplicate_ids(const lut_tree_node &node, const std::vector<uint8_t>& prefix, hash_set& seen) {
    hash_set rt = hash_set(0, &hash_function);

    const auto END = node.end_children();

    // Mark all children as seen.
    for(auto begin = node.begin_children(); begin != END; ++begin) {
        std::vector<uint8_t> id = prefix;
        for (uint8_t elem : (*begin)->get_id())
        { id.push_back(elem); }

        auto pair = seen.insert(id);
        if (!pair.second) {
            rt.insert(id);
        }

        // Recurse into children
        rt.merge(find_duplicate_ids(**begin, id, seen));
    }

    return rt;
}

/*****************************************************************************
 * Static Data
 *****************************************************************************/

/******************************************************************************
 * Interrupt Handlers
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/
hash_set find_duplicate_ids(const lut_tree_node &node) {
    hash_set seen = hash_set(0, &hash_function);

    return find_duplicate_ids(node, {}, seen);
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/
std::size_t hash_function(const std::vector<uint8_t>& vector) noexcept {
    std::size_t rt = 0;
    for (auto i : vector)
    { rt ^= i; }

    return rt;
}

// EOF
