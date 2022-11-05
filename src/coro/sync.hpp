#pragma once

/*! \file
 * \brief A coroutine that never suspends and executes synchronously
 */

#include "coro.hpp"

namespace coro {

//! A coroutine type that never suspends and executes synchronously.
/*! The function is called immediately when the coroutine object is created.
 * \tparam R the result type of the coroutine
 * \test in file test_sync.cpp */
template <class R> class sync {
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
        log_promise<promise_type, sync, std::suspend_never, std::suspend_never,
        R, not_transformer>
    {
        //! Stores the result value from \c co_return
        /*! \tparam T a template parameter used only to make this member
         * function a template, which allows to define it conditionally, only
         * if \a R is not \c void
         * \param[in] expr the value from \c co_return */
        template <std::same_as<R> T = R> requires (!std::is_void_v<T>)
        void return_value_impl(T&& expr) {
            if (_result)
                *_result = std::forward<T>(expr);
        }
        result_type* _result = nullptr;
    };
    //! The constructor needed by log_promise::get_return_object()
    /*! \param[in] promise the promise object */
    explicit sync(promise_type& promise) {
        promise._result = &_result;
    }
    //! Gets the result of the coroutine.
    /*! It moves the value from an internal storage, therefore it may be called
     * at most once.
     * \copydetails result() const & */
    template <std::same_as<R> T = R> requires (!std::is_void_v<T>)
    T result() && {
        return std::move(_result);
    }
    //! Gets the result of the coroutine.
    /*! \tparam T a template parameter used only to make this member
     * function a template, which allows to define it conditionally, only if \a
     * R is not \c void
     * \return the stored result of the coroutine */
    template <std::same_as<R> T = R> requires (!std::is_void_v<T>)
    const T& result() const & {
        return _result;
    }
private:
    //! The internal storage of the return value.
    [[no_unique_address]] result_type _result;
};

} // namespace coro
