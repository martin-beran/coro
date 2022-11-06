#pragma once

/*! \file
 * \brief A generator coroutine
 */

#include "coro.hpp"

#include <optional>

namespace coro {

//! A coroutine type that generates a sequence of values
/*! Individual values are returned by \c co_yield, end of the sequence is
 * marked by returning from the coroutine (without a value in \c co_return).
 * \tparam R the result type of the coroutine
 * \test in file test_generator.cpp */
template <class R> class generator {
public:
    struct promise_type;
    //! The coroutine handle type
    using handle_type = log_handle<promise_type>;
    //! The coroutine promise type
    struct promise_type:
        log_promise<promise_type, generator, std::suspend_always,
            std::suspend_always, void, not_transformer, std::type_identity>
    {
        //! Resets \ref current.
        void return_void() {
            promise_type::log_promise_type::return_void();
            current.reset();
        }
        //! Stores the generated value and suspends
        /*! \param[in] expr the generated value from \c co_yield
         * \return a request for suspension */
        std::suspend_always yield_value(R&& expr) {
            current =
                promise_type::log_promise_type::yield_value(std::move(expr));
            return {};
        }
        //! The current generated value
        std::optional<R> current;
    };
    class iterator {
    public:
        //! Creates the end iterator.
        iterator() = default;
        //! Creates the iterator.
        /*! \param[in] gen the associated generator */
        explicit iterator(generator& gen): gen(&gen) {}
        //! Equality test
        /*! \return \c true if using the same generator, \c false otherwise */
        bool operator==(const iterator&) const noexcept = default;
        //! Dereference operator
        /*! It must not be called for the end iterator.
         * \return the current value of the generator */
        const R& operator*() const noexcept {
            return *gen->handle.promise().current;
        }
        //! Prefix increment operator
        /*! It generates the next value.
         * \return \c *this */
        iterator& operator++() {
            if (!(*gen)())
                gen = nullptr;
            return *this;
        }
    private:
        //! The associated generator, \c nullptr for the end iterator
        generator* gen = nullptr;
    };
    //! The constructor needed by log_promise::get_return_object()
    /*! It stores the coroutine handle.
     * \param[in] promise the promise object */
    explicit generator(promise_type& promise):
        handle(handle_type::from_promise(promise))
    {}
    //! Default copy
    generator(const generator&) = default;
    //! Default move
    generator(generator&&) noexcept = default;
    //! Destroys the coroutine
    ~generator() {
        handle.destroy();
    }
    //! Default copy
    generator& operator=(const generator&) = default;
    //! Default move
    generator& operator=(generator&&) noexcept = default;
    //! Gets the next generated value.
    /*! \return the next value or \c std::nullopt at the end of the sequence */
    const std::optional<R>& operator()() {
        if (!handle.done())
            handle.resume();
        return handle.promise().current;
    }
    //! Gets the next generated value.
    /*! \return pointer to the next value; \c nullptr at the end of the
     * sequence */
    iterator begin() {
        operator()();
        if (handle.promise().current)
            return iterator{*this};
        else
            return {};
    }
    //! Gets the end 
    //
    static constexpr iterator end() {
        return {};
    }
private:
    //! The coroutine handle
    handle_type handle;
};

} // namespace coro
