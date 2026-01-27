#pragma once

#include <cstddef>

#include "FreeRTOS.h"
#include "portmacro.h"
#include "semphr.h"

namespace sync {

class rw_lock {
   public:
    class reader_reference {
       public:
        reader_reference(rw_lock& parent);
        reader_reference(const reader_reference&) = default;

        inline void lock() { _parent.lock_reader(); }

        inline bool try_lock(TickType_t timeout) {
            return _parent.try_lock_reader(timeout);
        }

        inline void unlock() { _parent.unlock_reader(); }

       private:
        rw_lock& _parent;
    };

    class writer_reference {
       public:
        writer_reference(rw_lock& parent);
        writer_reference(const writer_reference&) = default;

        inline void lock() { _parent.lock_writer(); }

        inline bool try_lock(TickType_t timeout) {
            return _parent.try_lock_writer(timeout);
        }

        inline void unlock() { _parent.unlock_writer(); }

       private:
        rw_lock& _parent;
    };

    rw_lock();
    rw_lock(const rw_lock&) = delete;
    rw_lock(rw_lock&&)      = delete;
    ~rw_lock();

    inline reader_reference reader_lock() { return reader_reference(*this); }

    inline writer_reference writer_lock() { return writer_reference(*this); }

    bool try_lock_reader(TickType_t timeout);
    bool try_lock_writer(TickType_t timeout);

    void lock_reader();
    void lock_writer();

    void unlock_reader();
    void unlock_writer();

   protected:

   private:
    SemaphoreHandle_t _write_lock;
    SemaphoreHandle_t _read_lock;
    std::size_t _readers;
};

}  // namespace sync
