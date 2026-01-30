// lut-tree.hh

/**************************************************************************//**
 * \file lut-tree.hh
 * \author Sean Benish
 * \brief 
 *
 * 
 *****************************************************************************/
#pragma once

#include <vector>
#include <memory>
#include <cstdint>

#include "lut-builder/intermediate-formats.hh"

/*****************************************************************************
 * Defines
 *****************************************************************************/

/*****************************************************************************
 * Data Types
 *****************************************************************************/
class lut_tree_node;
class lut_tree_node
{
private:
    std::vector<lut_tree_node*>  _children;
    lut_tree_node * _p_parent;

    const std::vector<uint8_t> _identifier;
protected:
    inline const std::vector<lut_tree_node*>& get_children() const
    {
        return _children;
    }
public:

    lut_tree_node();
    lut_tree_node(const std::vector<uint8_t>& identifier);

    lut_tree_node * get_parent() const
    {
        return _p_parent;
    }

    inline auto begin_children()
    {
        return _children.begin();
    }
    inline const auto begin_children() const
    {
        return _children.cbegin();
    }
    inline auto end_children()
    {
        return _children.end();
    }
    inline const auto end_children() const
    {
        return _children.cend();
    }

    void add_child(lut_tree_node * const p_child);
    void remove_child(lut_tree_node * const p_child);
    std::size_t children_size() const
    {
        return _children.size();
    }

    inline const std::vector<uint8_t>& get_id() const
    {
        return _identifier;
    }

    bool operator<(const lut_tree_node& r_other) const;
};

class entry_node : public lut_tree_node
{
private:
    compiled_entry_slim _entry;
public:

    entry_node(const compiled_entry_slim& r_entry);

    inline const compiled_entry_slim& get_entry() const
    {
        return _entry;
    }
};

class lut_tree : public lut_tree_node
{
private:
    std::vector<std::unique_ptr<lut_tree_node>> _objs;

    void create_down(entry_node * const p_entry, lut_tree_node * const p_target,
        std::size_t depth = 0);

public:
    const std::size_t HEIGHT;

    lut_tree(const std::vector<std::size_t> key_sizes);

    void add_struct(const compiled_entry_slim& entity);

    // class iterator : public std::iterator<
    //     std::forward_iterator_tag,
    //     entry_node&,
    //     std::size_t,
    //     entry_node*,
    //     entry_node&
    // > {
    //     std::vector<
    //         std::pair<
    //             std::vector<lut_tree_node*>::iterator,
    //             std::vector<lut_tree_node*>::iterator
    //         >
    //     > _itr_stack;

    //     void increment_stack_iterator(const std::size_t index);

    // public:
    //     explicit iterator(const std::vector<
    //         std::pair<
    //             std::vector<lut_tree_node*>::iterator,
    //             std::vector<lut_tree_node*>::iterator
    //         >
    //     >& r_stack);
    //     iterator& operator++();
    //     iterator operator++(int);
    //     bool operator==(const iterator& r_other) const;
    //     bool operator!=(const iterator& r_other) const;
    //     reference operator*() const;
    // };

    // iterator begin();
    // iterator end();
};

/*****************************************************************************
 * Constants
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/

//EOF
