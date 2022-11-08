#pragma once

/*! \file
 * \brief The main header file of the Coro library
 */

#include "log.hpp"

#include <coroutine>
#include <type_traits>

//! The top-level namespace of Coro
namespace coro {

//! A template that does not satisfy coro::impl::transformer for any type
/*! \tparam T a mapped type */
template <class T> struct not_transformer {
    //! Used to define return_void() instead of return_value() in log_promise
    static constexpr bool return_void = true;
};

//! This namespace contains various implementation details
namespace impl {

//! A helper empty class
struct empty {};

//! A type that can be written to an output stream
/*! \tparam T a type */
template <class T> concept printable = requires (std::ostream os, T v) {
    os << v;
};

//! A type that transforms a type to some other type
/*! It maps type \a T to type \c T::type. An example of a type that satisfies
 * this concept is \c std::type_identity.
 * \tparam T a mapped type */
template <class T> concept transformer = requires {
    typename T::type;
};

//! A helper concept
/*! It is used to choose between declaring \c return_void() or \c
 * return_value() in a promise class.
 * \tparam T a class */
template <class T> concept return_void = T::return_void;

//! The base class that declares \c return_void() for log_promise
struct declare_return_void {
    //! Called by \c co_return or falling off of a void-returning coroutine
    /*! \note The dummy template parameter turns this function to a template
     * and so allows it to be conditionally declared. */
    void return_void() {
        log() << "log_handle(" << this << ")->return_void()";
    }
};

//! The base class that declares \c return_value() for log_promise
/*! \tparam Promise the derived promise class
 * \tparam Return the coroutine return type */
template <class Promise, class Return> struct declare_return_value {
    //! Called by \c co_return with an expression
    /*! Calls return_value_impl(), which can be overriden in a derived \a
     * Promise class in order to use the returned value.
     * \param[in] expr the value returned by \c co_return */
    void return_value(Return&& expr) {
        if constexpr (impl::printable<Return>)
            log() << "log_handle(" << this << ")->return_value(" << expr << ")";
        else
            log() << "log_handle(" << this << ")->return_value()";
        static_cast<Promise*>(this)->
            template return_value_impl(std::move(expr));
    }
protected:
    //! Uses a value passed to \c co_return.
    /*! It does nothing by default, but it can be overriden in a derived \a
     * Promise class
     * \param[in] expr the value from \c co_return, passed by return_value() */
    template <class>
    void return_value_impl([[maybe_unused]] Return&& expr) {
    }
};

//! The base class of log_promise
/*! \tparam Promise the derived promise class
 * \tparam Return template argument \a Return of log_promise
 * \note Direct declaration in log_promise of \c return_void() and \c
 * return_value(), guarded by appropriate constraints that select one of these
 * functions, does not work, because the compiler (tested in GCC 11.3.0) treats
 * both functions as declared and fails. */
template <class Promise, class Return> struct return_void_or_value:
    std::conditional_t<std::is_void_v<Return>,
    declare_return_void, declare_return_value<Promise, Return>>
{
};

//! The base class that declares \c await_transform() for log_promise
/*! \tparam Await mapping of the parameter type to the return type of
 * await_transform() */
template <template <class> class Await> struct await_transform_impl {
    //! Called by \c co_await
    /*! \tparam Expr the type of expression passed to \c co_await
     * \param[in] expr the expression passed to \c co_await
     * \return transformed \a expr */
    template <class Expr> requires impl::transformer<Await<Expr>>
    typename Await<Expr>::type await_transform(Expr&& expr) {
        if constexpr (impl::printable<Expr>)
            log() << "log_promise(" << this << ")->log_await_transform(" <<
                expr << ")";
        else
            log() << "log_promise(" << this << ")->log_await_transform()";
        return typename Await<Expr>::type(std::forward<Expr>(expr));
    }
};

//! The specialization that does not declare await_transform()
template <> struct await_transform_impl<not_transformer> {};

//! The base class that declares \c yield_value() for log_promise
/*! \tparam Yield mapping of the parameter type to the return type of
 * yield_value() */
template <template <class> class Yield> struct yield_value_impl {
    //! Called by \c co_yield
    /*! \tparam Expr the type of expression passed to \c co_yield
     * \param[in] expr the expression passed to \c co_yield
     * \return transformed \a expr */
    template <class Expr> requires impl::transformer<Yield<Expr>>
    typename Yield<Expr>::type yield_value(Expr&& expr) {
        if constexpr (impl::printable<Expr>)
            log() << "log_promise(" << this << ")->yield_value(" << expr << ")";
        else
            log() << "log_promise(" << this << ")->yield_value()";
        return typename Yield<Expr>::type(std::forward<Expr>(expr));
    }
};

//! The specialization that does not declare yield_value()
template <> struct yield_value_impl<not_transformer> {};

} // namespace impl

//! A coroutine handle with logging at interesting places
/*! \tparam Promise the related promise type */
template <class Promise>
struct log_handle: std::coroutine_handle<Promise> {
    //! The base type
    using base_type = std::coroutine_handle<Promise>;
    //! The related promise type
    using promise_type = Promise;
    //! Logs constuction of the object
    log_handle() {
        log() << this << "->log_handle()";
    }
    //! Logs construction of the object
    /*! \param[in] p a promise object */
    explicit log_handle(const base_type& p): base_type(p) {
        log() << this << "->log_handle(Promise=" << &p.promise() << ")";
    }
    //! Default copy
    log_handle(const log_handle&) = default;
    //! Default move
    log_handle(log_handle&&) noexcept = default;
    //! Logs destruction of the object
    ~log_handle() {
        log() << this << "->~log_handle()";
    }
    //! Default copy
    /*! \return \c *this */
    log_handle& operator=(const log_handle&) = default;
    //! Default move
    /*! \return \c *this */
    log_handle& operator=(log_handle&&) noexcept = default;
};

//! A coroutine promise type with logging at interesting places
/*! \tparam Promise the derived promise class
 * \tparam RetObj the return type of get_return_object(), its constructor is
 * called with argument \c *this
 * \tparam InitialSuspend the return type of initial_suspend()
 * \tparam FinalSuspend the return type of final_suspend()
 * \tparam Return the type usable in \c co_return; if it is \c void,
 * return_value() is not defined and return_void() is defined instead
 * \tparam Await mapping of the parameter type to the return type of
 * await_transform()
 * \tparam Yield mapping of the parameter type to the return type of
 * yield_value() */
template <class Promise, class RetObj, class InitialSuspend, class FinalSuspend,
    class Return, template <class> class Await = not_transformer,
    template <class> class Yield = not_transformer>
struct log_promise:
    impl::return_void_or_value<Promise, Return>,
    impl::await_transform_impl<Await>,
    impl::yield_value_impl<Yield>
{
    //! A shortcut without template arguments for the current type
    using log_promise_type = log_promise;
    //! Logs construction of the object
    log_promise() {
        log() << this << "->log_promise()";
    }
    //! Default copy
    log_promise(const log_promise&) = default;
    //! Default move
    log_promise(log_promise&&) noexcept = default;
    //! Logs destruction of the object
    ~log_promise() {
        log() << this << "->~log_promise()";
    }
    //! Default copy
    /*! \return \c *this */
    log_promise& operator=(const log_promise&) = default;
    //! Default move
    /*! \return \c *this */
    log_promise& operator=(log_promise&&) noexcept = default;
    //! Used to allocate coroutine state
    /*! \param[in] count number of bytes to allocate
     * \return pointer to allocated memory */
    static void* operator new(size_t count) {
        void* p = ::operator new(count);
        log() << "log_promise::operator new(" << count << ")=" << p;
        return p;
    }
    //! Used to deallocate coroutine state
    /*! \param[in] p pointer to memory to be deallocated */
    static void operator delete(void* p) noexcept {
        try {
            log() << "log_promise::operator delete(" << p << ")";
        } catch (...) {
        }
        ::operator delete(p);
    }
    //! Creates the return value for the first suspend
    /*! \return a coroutine handle */
    RetObj get_return_object() {
        RetObj r{*static_cast<Promise*>(this)};
        if constexpr (impl::printable<RetObj>)
            log() << "log_promise(" << this << ")->get_return_object()=" <<
                &r << "/" << r;
        else
            log() << "log_promise(" << this << ")->get_return_object()=" << &r;
        return r;
    }
    //! Creates the initial suspend object
    /*! \return a default-constructed \a InitialSuspend */
    InitialSuspend initial_suspend() {
        InitialSuspend s{};
        if constexpr (impl::printable<InitialSuspend>)
            log() << "log_promise(" << this << ")->initial_suspend()=" << s;
        else
            log() << "log_promise(" << this << ")->initial_suspend()";
        return s;
    }
    //! Creates the final suspend object
    /*! \return a default-constructed \a FinalSuspend
     * \note C++20 requires \c noexcept. */
    FinalSuspend final_suspend() noexcept {
        FinalSuspend s{};
        try {
            if constexpr (impl::printable<FinalSuspend>)
                log() << "log_promise(" << this << ")->final_suspend()=" << s;
            else
                log() << "log_promise(" << this << ")->final_suspend()";
        } catch (...) {
        }
        return s;
    }
    //! Called if a coroutine body is terminated by an exception
    void unhandled_exception() {
        try {
            std::rethrow_exception(std::current_exception());
        } catch (std::exception& e) {
            log() << "log_promise(" << this << ")->unhandled_exception(" <<
                e.what() << ")";
        } catch (...) {
            log() << "log_promise(" << this << ")->unhandled_exception()";
        }
    }
};

//! A base class of an awaitable class with logging at interesting places
/*! \tparam Awaitable an awaitable class derived from log_awaitable
 * \tparam Awaiter a mapping to an awaiter type */
template <class Awaitable, template <class> class Awaiter>
struct log_awaitable {
    //! Logs construction of the object
    log_awaitable() {
        static_assert(std::derived_from<std::remove_reference_t<Awaitable>,
                      log_awaitable>);
        log() << this << "->log_awaitable()";
    }
    //! Default copy
    log_awaitable(const log_awaitable&) = default;
    //! Default move
    log_awaitable(log_awaitable&&) noexcept = default;
    //! Logs destruction of the object
    ~log_awaitable() {
        log() << this << "->~log_awaitable()";
    }
    //! Default copy
    /*! \return \c *this */
    log_awaitable& operator=(const log_awaitable&) = default;
    //! Default move
    /*! \return \c *this */
    log_awaitable& operator=(log_awaitable&&) noexcept = default;
    //! Converts the awaitable to an awaiter
    /*! If not defined, \a Awaitable is used as-is.
     * \tparam T a template parameter used only to make this member function a
     * template, which allows to define it conditionally, only if \c
     * Awaiter<Awaitable> satisfies impl::transformer
     * \return \c this converted to \a Awaitable, which is its real dynamic
     * type, and then transformed to \a Awaiter
     * \note Operator \c co_await can be also a non-member function. */
    template <std::same_as<Awaitable> A = Awaitable>
        requires impl::transformer<Awaiter<A>>
    typename Awaiter<A>::type operator co_await() {
        auto& self = static_cast<Awaitable&>(*this);
        if constexpr (impl::printable<Awaitable>)
            log() << "log_awaitable(" << this << ")->co_await(" << self << ")";
        else
            log() << "log_awaitable(" << this << ")->co_await()";
        return typename Awaiter<Awaitable>::type(self);
    }
};

//! The base awaiter class with logging at interesting places
/*! It expects the real awaiter implementation in derived class \a Awaiter and
 * just adds logging to it.
 * \tparam Awaiter a derived class that does the real work
 * \tparam Log whether to log construction and destruction (set to \c false in
 * log_avaitable_awaiter) */
template <class Awaiter, bool Log = true> struct log_awaiter {
    //! Logs construction of the object
    log_awaiter() {
        static_assert(std::derived_from<Awaiter, log_awaiter>);
        log() << this << "->log_awaiter()";
    }
    //! Default copy
    log_awaiter(const log_awaiter&) = default;
    //! Default move
    log_awaiter(log_awaiter&&) noexcept = default;
    //! Logs destruction of the object
    ~log_awaiter() {
        log() << this << "->~log_awaiter()";
    }
    //! Default copy
    /*! \return \c *this */
    log_awaiter& operator=(const log_awaiter&) = default;
    //! Default move
    /*! \return \c *this */
    log_awaiter& operator=(log_awaiter&&) noexcept = default;
    //! Checks if the result is immediately available without suspending
    /*! \return the result of \c Awaiter::await_ready_impl() */
    bool await_ready() {
        bool result = static_cast<Awaiter*>(this)->await_ready_impl();
        log() << "log_awaiter(" << this << ")->await_ready()=" << result;
        return result;
    }
    //! Controls suspending or immediate resuming the current coroutine.
    /*! \tparam Handle a coroutine handle type
     * \param[in] handle a coroutine handle
     * \return the result of \c Awaiter::await_ready_impl() */
    template <class Handle> auto await_suspend(Handle&& handle) {
        using result_type =
            decltype(static_cast<Awaiter*>(this)->
                     await_suspend_impl(std::forward<Handle>(handle)));
        if constexpr (std::is_void_v<result_type>) {
            static_cast<Awaiter*>(this)->
                await_suspend_impl(std::forward<Handle>(handle));
            log l{};
            std::move(l) << "log_awaiter(" << this << ")->await_suspend(";
            if constexpr (impl::printable<Handle>)
                std::move(l) << handle;
            std::move(l) << ")";
        } else {
            auto result = static_cast<Awaiter*>(this)->
                await_suspend_impl(std::forward<Handle>(handle));
            log l{};
            std::move(l) << "log_awaiter(" << this << ")->await_suspend(";
            if constexpr (impl::printable<Handle>)
                std::move(l) << handle;
            std::move(l) << ")=";
            if constexpr (impl::printable<result_type>)
                std::move(l) << result;
            return result;
        }
    }
    //! Gets the result of \c co_await.
    /*! \return the result of \c Awaiter::await_resume_impl() */
    auto await_resume() {
        using result_type =
            decltype(static_cast<Awaiter*>(this)->await_resume_impl());
        if constexpr (std::is_void_v<result_type>) {
            static_cast<Awaiter*>(this)->await_resume_impl();
            log() << "log_awaiter(" << this << ")->await_resume()";
        } else {
            auto result = static_cast<Awaiter*>(this)->await_resume_impl();
            log l{};
            std::move(l) << "log_awaiter(" << this << ")->await_resume()=";
            if constexpr (impl::printable<result_type>)
                std::move(l) << result;
            return result;
        }
    }
};

//! A combination of log_awaitable and log_awaiter, to be used as a base class
/*! It returns itself as the awaiter.
 * \tparam Awaiter a derived class that does the real awaiter work */
template <class Awaiter> struct log_avaitable_awaiter:
    log_awaitable<log_avaitable_awaiter<Awaiter>, std::add_lvalue_reference>,
    log_awaiter<Awaiter, false>
{
};

//!  A concept that defines requirements for a coroutine scheduler
/*! It has operations for registering, unregistering, and resuming coroutines.
 * \tparam T a scheduler class */
template <class T> concept scheduler =
    requires (T sched, std::coroutine_handle<void> co, typename T::iterator it)
    {
        typename T::iterator;
        { sched.insert(co) } -> std::same_as<typename T::iterator>;
        sched.erase(it);
        { sched.resume(it) } ->
            std::same_as<std::pair<std::coroutine_handle<void>, bool>>;
    };

} // namespace coro
