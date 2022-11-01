#pragma once

/*! \file
 * \brief The main header file of the Coro library
 */

#include "log.hpp"

#include <coroutine>

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

} // namespace impl

//! A coroutine handle with logging at interesting places
/*! \tparam Promise the related promise type */
template <class Promise> struct log_handle: std::coroutine_handle<Promise> {
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
    virtual ~log_handle() {
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
/*! \tparam Handle the related coroutine handle object
 * \tparam InitialSuspend the return type of initial_suspend()
 * \tparam FinalSuspend the return type of final_suspend()
 * \tparam Return mapping of the parameter type to the return type of
 * return_value()
 * \tparam Await mapping of the parameter type fo the return type of
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
    virtual ~log_promise() {
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
    virtual Handle get_return_object() {
        Handle h{Handle::from_promise(*this)};
        if constexpr (impl::printable<Handle>)
            log() << "log_handle(" << this << ")->get_return_object()=" << h;
        else
            log() << "log_handle(" << this << ")->get_return_object()";
        return h;
    }
    //! Creates the initial suspend object
    /*! \return a default-constructed \a InitialSuspend */
    virtual InitialSuspend initial_suspend() {
        InitialSuspend s{};
        if constexpr (impl::printable<InitialSuspend>)
            log() << "log_handle(" << this << ")->initial_suspend()=" << s;
        else
            log() << "log_handle(" << this << ")->initial_suspend()";
        return s;
    }
    //! Creates the final suspend object
    /*! \return a default-constructed \a FinalSuspend */
    virtual FinalSuspend final_suspend() {
        FinalSuspend s{};
        if constexpr (impl::printable<FinalSuspend>)
            log() << "log_handle(" << this << ")->final_suspend()=" << s;
        else
            log() << "log_handle(" << this << ")->final_suspend()";
        return s;
    }
    //! Called by \c co_return or falling off of a void-returning coroutine
    virtual void return_void() {
        log() << "log_handle(" << this << ")->return_void()";
    };
    //! Called by \c co_return with an expression
    /*! \tparam Expr the type of expression returned by \c co_return
     * \param[in] expr the value returned by \c co_return
     * \return a value of type \c Return<Expr>::type constructed from \a expr */
    template <class Expr> requires impl::transformer<Return<Expr>>
    typename Return<Expr>::type return_value(Expr&& expr) {
        if constexpr (impl::printable<Expr>)
            log_return_value([&expr](log& l) { std::move(l) << expr; });
        else
            log_return_value([](log&) {});
        return typename Return<Expr>::type{std::forward<Expr>(expr)};
    }
    //! Logs a call of return_value().
    /*! \param[in] f a function that logs the value passed to return_value() */
    virtual void log_return_value(const std::function<void(log&)>& f) {
        log l{};
        std::move(l) << "log_handle(" << this << ")->return_value(";
        f(l);
        std::move(l) << ")";
    }
    //! Called if a coroutine body is terminated by an exception
    virtual void unhandled_exception() {
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
     * \return \a expr */
    template <class Expr> requires impl::transformer<Await<Expr>>
    typename Await<Expr>::type await_transform(Expr&& expr) {
        if constexpr (impl::printable<Expr>)
            log_await_transform([&expr](log& l) { std::move(l) << expr; });
        else
            log_await_transform([](log&) {});
        return typename Await<Expr>::type{std::forward<Expr>(expr)};
    }
    //! Logs a call of await_transform().
    /*! \param[in] f a function that logs the value passed to await_transform()
     */
    virtual void log_await_transform(const std::function<void(log&)>& f) {
        log l{};
        std::move(l) << "log_handle(" << this << ")->log_await_transform(";
        f(l);
        std::move(l) << ")";
    }
};

} // namespace coro
