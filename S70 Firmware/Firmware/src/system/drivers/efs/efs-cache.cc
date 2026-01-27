// efs-cache.cc

/**************************************************************************//**
 * \file efs-cache.cc
 * \author Sean Benish
 *****************************************************************************/
#include "efs-cache.hh"
#include "efs.hh"

#include <utility>

using namespace efs;

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

/*****************************************************************************
 * Static Data
 *****************************************************************************/

/******************************************************************************
 * Interrupt Handlers
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/
file_cache::file_cache(const file_identifier_t id_to_cache, const bool lazy)
{
    if (lazy) {
        file_metadata md;
        md.id = id_to_cache;
        _cache.invalid_cache(md);
    } else {
        file_cache::init(id_to_cache);
    }
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/

cache_handle::cache_handle(const user_e user)
    : handle(user), _p_parent(nullptr)
{}

cache_handle::cache_handle(const user_e user, file_cache * const p_parent)
    : handle(user, p_parent->_cache.get_metadata()), _p_parent(p_parent)
{}

cache_handle::cache_handle(cache_handle&& r_other)
    : handle(static_cast<handle&&>(r_other)), _p_parent(r_other._p_parent)
{}

cache_handle::~cache_handle()
{
    close();
}

init_file_cache::init_file_cache(const file_identifier_t id_to_cache)
    : file_cache(id_to_cache, true)
{}

// EOF
