// lut-tree.cc

/**************************************************************************//**
 * \file lut-tree.cc
 * \author Sean Benish
 *****************************************************************************/
#include "lut-builder/lut-tree.hh"

#include <algorithm>
#include <stdexcept>
#include <string>

/*****************************************************************************
 * Constants
 *****************************************************************************/

/*****************************************************************************
 * Macros
 *****************************************************************************/

/*****************************************************************************
 * Data Types
 *****************************************************************************/
struct
{
    bool operator()(const lut_tree_node * a, const lut_tree_node * b)
    {
        return *a < *b;
    }
} ptr_cmp;

/*****************************************************************************
 * Private Function Prototypes
 *****************************************************************************/

/*****************************************************************************
 * Static Data
 *****************************************************************************/

/******************************************************************************
 * Interrupt Handlers
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/
entry_node::entry_node(const compiled_entry_slim& r_entry)
    : lut_tree_node(r_entry.keys[r_entry.keys.size() - 1]), _entry(r_entry)
{}

lut_tree_node::lut_tree_node()
    : _identifier(std::vector<uint8_t>())
{}
lut_tree_node::lut_tree_node(const std::vector<uint8_t>& r_identifier)
    : _identifier(r_identifier)
{}

void lut_tree_node::add_child(lut_tree_node * const p_child)
{
    _children.push_back(p_child);
    p_child->_p_parent = this;
    std::sort(_children.begin(), _children.end(), ptr_cmp);
}

void lut_tree_node::remove_child(lut_tree_node * const p_child)
{
    for (auto itr = _children.begin(); itr != _children.end(); ++itr)
    {
        if (*itr == p_child)
        {
            _children.erase(itr);
            return;
        }
    }
}

bool lut_tree_node::operator<(const lut_tree_node& r_other) const
{
    if (_identifier.size() < r_other._identifier.size())
    {
        return true;
    }
    if (_identifier.size() > r_other._identifier.size())
    {
        return false;
    }

    for (std::size_t i = _identifier.size() - 1; i >= 0; --i)
    {
        if (_identifier[i] != r_other._identifier[i])
        {
            return _identifier[i] < r_other._identifier[i];
        }
    }

    // Equal
    return false;
}


lut_tree::lut_tree(const std::vector<std::size_t> key_sizes)
    : HEIGHT(key_sizes.size())
{}

void lut_tree::add_struct(const compiled_entry_slim& entry)
{
    if (entry.keys.size() != HEIGHT)
    {
        throw std::invalid_argument(std::string("Entry had a indirection level of ") 
            + std::to_string(entry.keys.size() - 2) + ". LUT had indirection of " + std::to_string(HEIGHT - 2));
    }

    entry_node * const p_node = new entry_node(entry);

    create_down(p_node, this);

    _objs.emplace_back(p_node);
}


// lut_tree::iterator::iterator(const std::vector<
//     std::pair<
//         std::vector<lut_tree_node*>::iterator,
//         std::vector<lut_tree_node*>::iterator
//     >
// >& r_stack)
//     : _itr_stack(r_stack)
// {}

// lut_tree::iterator& lut_tree::iterator::operator++()
// {
//     increment_stack_iterator(_itr_stack.size());
// }
// lut_tree::iterator lut_tree::iterator::operator++(int)
// {
//     lut_tree::iterator tmp(*this);
//     ++(*this);
//     return tmp;
// }
// bool lut_tree::iterator::operator==(const lut_tree::iterator& r_other) const
// {
//     return _itr_stack == r_other._itr_stack;
// }
// bool lut_tree::iterator::operator!=(const lut_tree::iterator& r_other) const
// {
//     return _itr_stack != r_other._itr_stack;
// }
// lut_tree::iterator::reference lut_tree::iterator::operator*() const
// {
//     return *reinterpret_cast<entry_node*>(*_itr_stack.back().first);
// }




// lut_tree::iterator lut_tree::begin()
// {
//     std::vector<
//         std::pair<
//             std::vector<lut_tree_node*>::iterator,
//             std::vector<lut_tree_node*>::iterator
//         >
//     > itr_stack(HEIGHT);
//     lut_tree_node * p_node = this;
//     std::pair<
//         std::vector<lut_tree_node*>::iterator,
//         std::vector<lut_tree_node*>::iterator
//     > pair = std::pair(p_node->begin_children(), p_node->end_children());
//     while (pair.first != pair.second)
//     {
//         itr_stack.push_back(pair);
//         p_node = *pair.first;
//         pair = std::pair(p_node->begin_children(), p_node->end_children());
//     }

//     return iterator(itr_stack);
// }

// lut_tree::iterator lut_tree::end()
// {
//     std::vector<
//         std::pair<
//             std::vector<lut_tree_node*>::iterator,
//             std::vector<lut_tree_node*>::iterator
//         >
//     > itr_stack(HEIGHT);
//     lut_tree_node * p_node = this;
//     std::pair<
//         std::vector<lut_tree_node*>::iterator,
//         std::vector<lut_tree_node*>::iterator
//     > pair = std::pair(p_node->begin_children(), p_node->end_children());
//     while (pair.first != pair.second)
//     {
//         pair.first = pair.second - 1;
//         itr_stack.push_back(pair);
//         p_node = *pair.first;
//         pair = std::pair(p_node->begin_children(), p_node->end_children());
//     }

//     return iterator(itr_stack);
// }

/*****************************************************************************
 * Private Functions
 *****************************************************************************/
void lut_tree::create_down(entry_node * const p_entry, lut_tree_node * const p_target,
    std::size_t depth)
{
    if (depth + 1 == HEIGHT)
    {
        p_target->add_child(p_entry);
        return;
    }

    const std::vector<uint8_t>& r_ident = p_entry->get_entry().keys[depth];
    for (auto itr = p_target->begin_children(); itr != p_target->end_children(); ++itr)
    {
        if (r_ident == (*itr)->get_id())
        {
            // Found a child node with the right identifier.
            create_down(p_entry, *itr, depth + 1);
            return;
        }
    }

    // Did not find a child node with the right identifier?
    // Add the node and continue down.
    lut_tree_node * const p_node = new lut_tree_node(r_ident);
    _objs.emplace_back(p_node);
    p_target->add_child(p_node);
    create_down(p_entry, p_node, depth + 1);
}

// void lut_tree::iterator::increment_stack_iterator(const std::size_t index)
// {
//     if (index == 0)
//     {
//         return;
//     }

//     auto& pair = _itr_stack[index - 1];
//     ++pair.first;
//     if (pair.first == pair.second)
//     {
//         increment_stack_iterator(index - 1);
//         if (index >= 2)
//         {
//             lut_tree_node * const p_node = *_itr_stack[index - 2].first;
//             pair.first = p_node->begin_children();
//             pair.second = p_node->end_children();
//         }
//     }
// }

// EOF
