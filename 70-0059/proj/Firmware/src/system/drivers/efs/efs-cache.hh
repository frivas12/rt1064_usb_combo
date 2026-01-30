// efs-cache.hh

/**************************************************************************//**
 * \file efs-cache.hh
 * \author Sean Benish
 * \brief 
 *
 * 
 *****************************************************************************/
#pragma once

#include "efs-handle.hh"

namespace efs
{
    void init();
    bool erase();
    bool create_file( const file_identifier_t, const uint16_t,
        const file_attributes_t, const TickType_t);
}

namespace efs
{

/*****************************************************************************
 * Defines
 *****************************************************************************/

/*****************************************************************************
 * Data Types
 *****************************************************************************/
class cache_handle;

class metadata_cache
{
private: 
    file_metadata _metadata;
public:

    /**
     * Caches metadata and asserts the metadata as valid.
     * \param[in]       metadata The valid metadata to cache.
     */
    inline void cache(const file_metadata& metadata)
    {
        _metadata = metadata;
        _metadata.attr = static_cast<file_attributes_t>(_metadata.attr & ~FILE_ATTRIBUTES_MASK_NOT_ALLOCATED);
    }
    
    /**
     * Caches metadata that is not valid.
     * Can be used to store the file id when the metadata is not available.
     * \param[in]       metadata The invalid metadata to cache.
     */
    inline void invalid_cache(const file_metadata& metadata)
    {
        _metadata = metadata;
        _metadata.attr = static_cast<file_attributes_t>(_metadata.attr | FILE_ATTRIBUTES_MASK_NOT_ALLOCATED);
    }

    /**
     * Changes the metadata to invalid.
     */
    inline void invalidate()
    {
        _metadata.attr |= FILE_ATTRIBUTES_MASK_NOT_ALLOCATED;
    }

    /**
     * \return true The cached metadata is valid.
     * \return false The cached metadata is invalid.
     */
    inline bool is_valid() const
    {
        return (_metadata.attr & FILE_ATTRIBUTES_MASK_NOT_ALLOCATED) == 0;
    }

    /**
     * \return A reference to the immutable cached metadata.
     */
    inline const file_metadata& get_metadata() const
    {
        return _metadata;
    }
};

template<std::size_t LEN>
class metadata_cache_pool
{
private:
    std::size_t _oldest;
    metadata_cache _caches[LEN];
public:
    metadata_cache_pool()
        : _oldest(0)
    {
        for (std::size_t i = 0; i < LEN; ++i)
        {
            _caches[i].invalidate();
        }
    }

    void add(const file_metadata& metadata)
    {
        _caches[_oldest].cache(metadata);
        if (++_oldest == LEN)
        {
            _oldest = 0;
        }
    }

    void remove(const file_identifier_t id)
    {
        for (std::size_t i = 0; i < LEN; ++i)
        {
            const file_metadata& r_md = _caches[i].get_metadata();
            if (r_md.id == id)
            {
                _caches[i].invalidate();
                return;
            }
        }
    }

    void clear()
    {
        for (std::size_t i = 0; i < LEN; ++i)
        {
            _caches[i].invalidate();
        }
    }

    bool get_metadata(const file_identifier_t id, file_metadata& r_metadata)
    {
        for (std::size_t i = 0; i < LEN; ++i)
        {
            const file_metadata& r_md = _caches[i].get_metadata();
            if (r_md.id == id && _caches[i].is_valid())
            {
                r_metadata = r_md;
                return true;
            }
        }

        return false;
    }
};


class file_cache
{
protected:
    metadata_cache _cache;

    file_cache * _p_next;
    /// @brief Set to true when the cache has ownership of the file.
    bool _owns_file = false;
    
    /// @brief When true, the cache's metadata is synced with the EFS.  This can be false when constructed from ISR.
    bool _cache_updated;

    // Must be protected with the EFS mutex.
    volatile bool _has_handle = false;

    void init(const file_identifier_t id_to_cache);
    void init_from_isr(const file_identifier_t id_to_cache);

public:

    /**
     * Creates a cache for a file making handle creation fast.
     * If the file becomes owned by the cache, then the file will not be accessible until the cache releases control.
     * Calling get_handle() while the cache owns the file will transfer ownership to the handle until the handle closes.
     * \note Static file caches must be lazy, but will be initialized upon efs::init() being called.
     * \warning The constructor may block the thread until it gets the metadata to cache.
     * \param[in]       id_to_cache The identifier of the file to cache.
     * \param[in]       lazy Default: false.  When true, the intializiation will be to get handle() or initialization.
     */
    file_cache(const file_identifier_t id_to_cache, const bool lazy = false);

    file_cache(const file_cache&) = delete;
    file_cache& operator= (const file_cache&) = delete;
    ~file_cache();

    inline file_identifier_t get_file_id () const
    {
        return _cache.get_metadata().id;
    }

    /**
     * Gives a handle to the file.
     * If the cache owned the file, then the file will be returned to the cache on completion.
     * \warning The end user should verify that the handle can perform the actions desired.
     * \param user
     * \param wait_until_valid Defaults to true.  When true, the cache will block the thread until the handle is returned.
     * \return cache_handle 
     */
    cache_handle get_handle(
        const bool wait_until_valid = true,
        const user_e user = FIRMWARE
    );


    /**
     * Takes ownership of the cached file from the EFS if possible.
     * \note This does *not* give the executing code ownership of the file, only the cache.
     * \return true The cache owns the file.
     * \return false The cache does not own the file.
     */
    bool take_ownership();

    /**
     * Releases ownership of the cached file if the cache owned it.
     */
    void release_ownership();

    /**
     * \return true The file cache owns the file identified.
     * \return false The file cache does not own the file identified.
     */
    inline bool has_ownership() const
    {
        return _owns_file;
    }


    friend class cache_handle;
    friend class handle;
    friend void efs::init();
    friend bool efs::erase();
    friend bool create_file( const file_identifier_t, const uint16_t,
        const file_attributes_t, const TickType_t);
};

/**
 * Class for static file caches.
 * This allows for ownership to be taken at initialization.
 * \note Static file caches are ALWAYS lazy.
 */
class init_file_cache : public file_cache
{
public:

    init_file_cache(const file_identifier_t id_to_cache);

    init_file_cache(const init_file_cache&) = delete;
    init_file_cache& operator= (const init_file_cache&) = delete;
    ~init_file_cache() = default;

    /**
     * Takes ownership of the cached file from the EFS if possible.
     * \warning This may only be used during initialization.
     * \note This does *not* give the executing code ownership of the file, only the cache.
     * \return true The cache owns the file.
     * \return false The cache does not own the file.
     */
    bool take_ownership_init();

    /**
     * Releases ownership of the cached file if the cache owned it.
     * \warning This may only be used during initialization.
     */
    void release_ownership_init();

};

class cache_handle : public handle
{
private:
    file_cache * const _p_parent;

protected:

    void release_ownership () override;

    cache_handle(const user_e source);
    cache_handle(const user_e source, file_cache * const p_parent);

    friend class file_cache;

public:

    cache_handle(const cache_handle&) = delete;
    cache_handle& operator=(const cache_handle&) = delete;
    cache_handle(cache_handle&& r_other);

    ~cache_handle();
};

/*****************************************************************************
 * Constants
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/

}

//EOF
