#pragma once

/*! \file
 * \brief A round-robin scheduler for coroutines
 */

#include "coro.hpp"

namespace coro {

//! A round-robin scheduler
/*! It keeps a list of registered coroutines. If a coroutine requests
 * scheduling of another coroutine by calling resume(), the next coroutine
 * (wrapping around from the last to the first) is resumed. */
class sched_rr {
    //! The data structure used to store registered coroutines
    using storage_type = std::list<std::coroutine_handle<void>>;
public:
    //! Iterator type
    using iterator = typename storage_type::iterator;
    //! Registers a coroutine
    /*! It should not be called for the same coroutine again without erasing it
     * by erase() first. 
     * \param[in] hnd a coroutine handle
     * \return iterator to the registration record */
    iterator insert(std::coroutine_handle<void> hnd) {
        auto it = storage.insert(storage.end(), hnd);
        coro::log() << "sched_rr::insert()=" << &*it;
        return it;
    }
    //! Unregisters a coroutine
    /*! The coroutine \a it must be currently registered in this scheduler.
     * \param[in] it an iterator returned by insert() */
    void erase(iterator it) {
        coro::log() << "sched_rr::erase(" << &*it << ")";
        storage.erase(it);
    }
    //! Resumes a coroutine.
    /*! It resumes the next coroutine in the list after \a it (wrapping from
     * the last to the first. It resumes itself if \a it is the only registered
     * coroutine.
     * \param[in] it an iterator returned by insert()
     * \return a pair containg a handle of the coroutine that will be resumed
     * and an indicator if the returned handle is different from the handle
     * referenced by \a it */
    std::pair<std::coroutine_handle<void>, bool> resume(iterator it) {
        if (++it == storage.end())
            it = storage.begin();
        coro::log() << "sched_rr::resume()=" << &*it;
        return {*it, storage.size() > 1};
    }
private:
    //! The registered coroutines
    storage_type storage;
};

static_assert(scheduler<sched_rr>);

} // namespace coro
