#pragma once

/*! \file
 * \brief The main header file of the Coro library
 */

#include "log.hpp"

#include <coroutine>
#include <type_traits>

//! The top-level namespace of Coro
namespace coro {

//! This namespace contains various implementation details
namespace impl {

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

//! A type that does not satisfy coro::impl::transformer
/*! \tparam T a mapped type */
template <class T> struct not_transformer {};

} // namespace impl

//! A coroutine handle with logging at interesting places
/*! \tparam Promise the related promise type
 * \tparam Yield mapping of the parameter type to the return type of
 * yield_value() */
template <class Promise, template <class> class Yield = impl::not_transformer>
struct log_handle: std::coroutine_handle<Promise> {
    //! The related promise type
    using promise_type = Promise;
    //! Logs constuction of the object
    log_handle() {
        log() << this << "->log_handle()";
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
    //! Called by \c co_yield
    /*! It is not defined if \c Yield<Expr> does not satisfy impl::transformer,
     * which causes disabling \c co_yield.
     * \tparam Expr the type of expression passed to \c co_yield
     * \param[in] expr the expression passed to \c co_yield
     * \return transformed \a expr */
    template <class Expr> requires impl::transformer<Yield<Expr>>
    typename Yield<Expr>::type yield_value(Expr&& expr) {
        if constexpr (impl::printable<Expr>)
            log() << "log_promise(" << this << ")->yield_value(" << expr << ")";
        else
            log() << "log_promise(" << this << ")->yield_value()";
        return typename Yield<Expr>::type{std::forward<Expr>(expr)};
    }
};

//! A coroutine promise type with logging at interesting places
/*! \tparam Handle the related coroutine handle object
 * \tparam InitialSuspend the return type of initial_suspend()
 * \tparam FinalSuspend the return type of final_suspend()
 * \tparam Return mapping of the parameter type to the return type of
 * return_value()
 * \tparam Await mapping of the parameter type to the return type of
 * await_transform() */
template <class Handle, class InitialSuspend, class FinalSuspend,
    template <class> class Return, template <class> class Await>
struct log_promise {
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
    //! Creates the return value for the first suspend
    /*! \return a coroutine handle */
    Handle get_return_object() {
        Handle h{Handle::from_promise(*this)};
        if constexpr (impl::printable<Handle>)
            log() << "log_handle(" << this << ")->get_return_object()=" << h;
        else
            log() << "log_handle(" << this << ")->get_return_object()";
        return h;
    }
    //! Creates the initial suspend object
    /*! \return a default-constructed \a InitialSuspend */
    InitialSuspend initial_suspend() {
        InitialSuspend s{};
        if constexpr (impl::printable<InitialSuspend>)
            log() << "log_handle(" << this << ")->initial_suspend()=" << s;
        else
            log() << "log_handle(" << this << ")->initial_suspend()";
        return s;
    }
    //! Creates the final suspend object
    /*! \return a default-constructed \a FinalSuspend */
    FinalSuspend final_suspend() {
        FinalSuspend s{};
        if constexpr (impl::printable<FinalSuspend>)
            log() << "log_handle(" << this << ")->final_suspend()=" << s;
        else
            log() << "log_handle(" << this << ")->final_suspend()";
        return s;
    }
    //! Called by \c co_return or falling off of a void-returning coroutine
    void return_void() {
        log() << "log_handle(" << this << ")->return_void()";
    };
    //! Called by \c co_return with an expression
    /*! \tparam Expr the type of expression returned by \c co_return
     * \param[in] expr the value returned by \c co_return
     * \return a value of type \c Return<Expr>::type constructed from \a expr */
    template <class Expr> requires impl::transformer<Return<Expr>>
    typename Return<Expr>::type return_value(Expr&& expr) {
        if constexpr (impl::printable<Expr>)
            log() << "log_handle(" << this << ")->return_value(" << expr << ")";
        else
            log() << "log_handle(" << this << ")->return_value()";
        return typename Return<Expr>::type{std::forward<Expr>(expr)};
    }
    //! Called if a coroutine body is terminated by an exception
    void unhandled_exception() {
        try {
            std::rethrow_exception(std::current_exception());
        } catch (std::exception& e) {
            log() << "log_handle(" << this << ")->unhandled_exception(" <<
                e.what() << ")";
        } catch (...) {
            log() << "log_handle(" << this << ")->unhandled_exception()";
        }
    }
    //! Called by \c co_await
    /*! It is not defined if \c Await<Expr> does not satisfy impl::transformer,
     * which causes using \a expr as-is.
     * \tparam Expr the type of expression passed to \c co_await
     * \param[in] expr the expression passed to \c co_await
     * \return transformed \a expr */
    template <class Expr> requires impl::transformer<Await<Expr>>
    typename Await<Expr>::type await_transform(Expr&& expr) {
        if constexpr (impl::printable<Expr>)
            log() << "log_handle(" << this << ")->log_await_transform(" <<
                expr << ")";
        else
            log() << "log_handle(" << this << ")->log_await_transform()";
        return typename Await<Expr>::type{std::forward<Expr>(expr)};
    }
};

//! A base class of an awaitable class with logging at interesting places
/*! \tparam Awaitable an awaitable class derived from log_awaitable
 * \tparam Awaiter a mapping to an awaiter type */
template <class Awaitable, template <class> class Awaiter>
struct log_awaitable {
    static_assert(std::derived_from<Awaitable, log_awaitable>);
    //! Logs construction of the object
    log_awaitable() {
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
    /*! It is not defined if \c Awaiter<Awaitable> does not satisfy
     * impl::transformer, which causes using \a Awaitable as-is
     * \return \c this converted to \a Awaitable, which is its real dynamic
     * type, and then transformed to \a Awaiter
     * \note The dummy template parameter turns this function to a template and
     * so allows it to be conditionally declared. */
    template <class = void> requires impl::transformer<Awaiter<Awaitable>>
    typename Awaiter<Awaitable>::type operator co_await() {
        auto& self = static_cast<Awaitable&>(*this);
        if constexpr (impl::printable<Awaitable>)
            log() << "log_awaitable(" << this << ")->co_await(" << self << ")";
        else
            log() << "log_awaitable(" << this << ")->co_await()";
        return typename Awaiter<Awaitable>::type{self};
    }
};

//! Controls if <tt>operator co_await()</tt> is defined for type \a T.
/*! The operator is not defined by default, but can be enabled by defining
 * specializations derived from \c std::true_type. */
template <class T> struct define_co_await: std::false_type {};

//! A non-member alternative to log_awaitable::operator co_await()
/*! It is defined only if <tt>define_co_await<Awaitable>::value</tt> is \c
 * true.
 * \tparam Awaitable an awaitable class
 * \tparam Awaiter a mapping to an awaiter type
 * \param[in] awaitable an awaitable object
 * \return the awaiter object corresponding to \a awaitable */
template <class Awaitable, template <class> class Awaiter>
    requires define_co_await<Awaitable>::value
typename Awaiter<Awaitable>::type operator co_await(Awaitable&& awaitable)
{
    if constexpr (impl::printable<Awaitable>)
        log() << "operator co_await(" << &awaitable << "=" << awaitable << ")";
    else
        log() << "operator co_await(" << &awaitable << ")";
    return
        typename Awaiter<Awaitable>::type{std::forward<Awaitable>(awaitable)};
}

//! The base awaiter class with logging at interesting places
/*! It expects the real awaiter implementation in derived class \a Awaiter and
 * just adds logging to it.
 * \tparam Awaiter a derived class that does the real work
 * \tparam Log whether to log construction and destruction (set to \c false in
 * log_avaitable_awaiter) */
template <class Awaiter, bool Log = true> struct log_awaiter {
    static_assert(std::derived_from<Awaiter, log_awaiter>);
    //! Logs construction of the object
    log_awaiter() {
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
    auto await_resume_impl() {
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

} // namespace coro
