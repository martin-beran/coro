#pragma once

/*! \file
 * \prief A coroutine that is started lazily
 */

#include "coro.hpp"
#include <coroutine>

namespace coro {

//! A coroutine type that is executed lazily when the return value is retrieved
/*! \tparam R the result type of the coroutine
 * \test in file test_lazy.cpp */
template <class R> class lazy {
    //! A helper class for void return value.
    struct empty {};
    //! The type for storing coroutine result
    using result_type = std::conditional_t<std::is_void_v<R>, empty, R>;
public:
    struct promise_type;
    //! The coroutine handle type
    using handle_type = log_handle<promise_type>;
    //! The coroutine promise type
    struct promise_type:
        log_promise<promise_type, lazy,
            std::suspend_always, std::suspend_always, R, not_transformer>
    {
        //! Stores the result value from \c co_return
        /*! \tparam T a template parameter used only to make this member
         * function a template, which allows to define it conditionally, only
         * if \a R is not \c void
         * \param[in] expr the value from \c co_return */
        template <std::same_as<R> T = R> requires (!std::is_void_v<T>)
        void return_value_impl(T&& expr) {
            result = std::forward<T>(expr);
        }
        //! The place for coroutine result in the coroutine type
        [[no_unique_address]] result_type result;
    };
    //! The constructor needed by log_promise::get_return_object()
    /*! \param[in] promise the promise object */
    explicit lazy(promise_type& promise):
        handle(handle_type::from_promise(promise))
    {
    }
    //! Destroys the coroutine
    ~lazy() {
        handle.destroy();
    }
    //! Gets the result of the coroutine.
    /*! It moves the value from an internal storage, therefore it may be called
     * at most once.
     * \copydetails result() const */
    template <std::same_as<R> T = R> requires (!std::is_void_v<T>)
    T result() && {
        run();
        return std::move(handle.promise().result);
    }
    //! Gets the result of the coroutine.
    /*! \tparam T a template parameter used only to make this member
     * function a template, which allows to define it conditionally, only if \a
     * R is not \c void
     * \return the stored result of the coroutine */
    template <std::same_as<R> T = R> requires (!std::is_void_v<T>)
    const T& result() & {
        run();
        return handle.promise().result;
    }
    //! Runs the coroutine
    void run() {
        if (!handle.done())
            handle.resume();
    }
private:
    //! The coroutine handle
    handle_type handle;
};

} // namespace coro
