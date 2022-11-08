#pragma once

/*! \file
 * \brief An asynchronous task coroutine
 */

#include "coro.hpp"
#include "sched_rr.hpp"

#include <coroutine>
#include <optional>

namespace coro {

//! An asynchronous task
/*! It starts lazily, can be awaited, and can await other coroutines The final
 * result is generated by \c co_return. It is possible to generate intermediate
 * results by \c co_yield.
 * \tparam R the result type of the coroutine
 * \tparam Sched the scheduler type used for suspending and resuming
 * \test in file test_task.cpp */
template <class R, scheduler Sched> class task:
        public log_awaitable<task<R, Sched>&, not_transformer>,
        public log_awaiter<task<R, Sched>>
{
    //! The type for storing coroutine result
    using result_type =
        std::optional<std::conditional_t<std::is_void_v<R>, impl::empty, R>>;
public:
    struct promise_type;
    //! The coroutine handle type
    using handle_type = log_handle<promise_type>;
    //! Resumes some other coroutine at the final suspension point
    struct task_final_suspend: std::suspend_always {
        //! If there is another coroutine, resume it
        /*! \param[in] hnd the handle of the current coroutine
         * \return another coroutine handle; \c std::noop_coroutine_handle if
         * the there is no other coroutine */
        std::coroutine_handle<void>
        await_suspend(std::coroutine_handle<promise_type> hnd) noexcept {
            auto& promise = hnd.promise();
            if (promise.sch_it != typename Sched::iterator{}) {
                auto [next, other] = promise.sched.resume(promise.sch_it);
                coro::log() << "await_suspend erase";
                promise.sched.erase(promise.sch_it);
                promise.sch_it = {};
                coro::log() << "sch_it=" << &promise.sch_it << " null=" << (promise.sch_it ==typename Sched::iterator{});
                if (other)
                    return next;
            }
            return std::noop_coroutine();
        }
    };
    //! Resumes some other coroutine if awaited
    struct yield: std::suspend_always {
        //! If there is another coroutine, resume it
        /*! \param[in] hnd the handle of the current coroutine
         * \return another coroutine handle; this coroutine handle if there is
         * no other coroutine */
        std::coroutine_handle<void>
        await_suspend(std::coroutine_handle<promise_type> hnd) noexcept {
            auto& promise = hnd.promise();
            return promise.sched.resume(promise.sch_it).first;
        }
    };
    //! The coroutine promise type
    struct promise_type:
        log_promise<promise_type, task, std::suspend_always,
            task_final_suspend, R, not_transformer, std::type_identity>
    {
        //! Disable a coroutine without a scheduler as the first parameter.
        promise_type() = delete;
        //! Creates the promise object and associates it with a scheduler.
        /*! \tparam Args the remaining coroutine parameters
         * \param[in] sched the scheduler used by this coroutine */
        template <class ...Args> explicit promise_type(Sched& sched, Args&&...):
            sched(sched), sch_it(sched.insert(handle_type::from_promise(*this)))
        {
            coro::log() << "sch_it=" << &sch_it << " val=" << &*sch_it;
        }
        //! Creates the promise object and associates it with a scheduler.
        /*! This constructor is used if the coroutine is a non-static member
         * function of class \a T
         * \tparam T the class containing this coroutine as a non-static member
         * function
         * \tparam Args types of the remaining coroutine parameters
         * \param[in] sched the scheduler used by this coroutine
         * \param[in] args the remaining coroutine parameters
         * \note Allowing copy or move would cause double erase from \ref sched
         * in the destructor. */
        template <class T, class ...Args>
        explicit promise_type(T&&, Sched& sched, Args&& ...args):
            promise_type(sched, std::forward<Args>(args)...)
        {}
        //! No copy
        promise_type(const promise_type&) = delete;
        //! No move
        promise_type(promise_type&&) noexcept = delete;
        //! Unregisters the coroutine from the scheduler
        ~promise_type() {
            coro::log() << "sch_it=" << &sch_it << " null=" << (sch_it ==typename Sched::iterator{});
            if (sch_it != typename Sched::iterator{}) 
                sched.erase(sch_it);
        }
        //! No copy
        /*! \return \c *this */
        promise_type& operator=(const promise_type&) = delete;
        //! No move
        /*! \return \c *this */
        promise_type& operator=(promise_type&&) noexcept = delete;
        //! Stores the result value from \c co_return
        /*! \tparam T a template parameter used only to make this member
         * function a template, which allows to define it conditionally, only
         * if \a R is not \c void
         * \param[in] expr the value from \c co_return */
        template <std::same_as<R> T = R> requires (!std::is_void_v<T>)
        void return_value_impl(T&& expr) {
                current = std::forward<T>(expr);
        }
        //! Stores the result value and suspends
        /*! \tparam T a template parameter used only to make this member
         * function a template, which allows to define it conditionally, only
         * if \a R is not \c void
         * \param[in] expr the value from \c co_yield
         * \return a request for suspension */
        template <std::same_as<R> T = R> requires (!std::is_void_v<T>)
        std::suspend_always yield_value(T&& expr) {
            current = promise_type::log_promise_type::yield_value(
                                                        std::forward<T>(expr));
            return {};
        }
        //! The current result value
        result_type current;
        //! The scheduler used by this coroutine
        Sched& sched;
        //! The registration of this coroutine in \ref sched
        typename Sched::iterator sch_it{};
    };
    //! The constructor needed by log_promise::get_return_object()
    /*! It stores the coroutine handle.
     * \param[in] promise the promise object */
    explicit task(promise_type& promise):
        handle(handle_type::from_promise(promise))
    {}
    //! No copy
    /*! Because all copies would destroy the same coroutine via copied \ref
     * handle. */
    task(const task&) = delete;
    //! Default move
    task(task&&) noexcept = default;
    //! Destroys the coroutine
    ~task() {
        handle.destroy();
    }
    //! No copy
    /*! Because all copies would destroy the same coroutine via copied \ref
     * handle.
     * \return \c *this */
    task& operator=(const task&) = delete;
    //! Default move
    /*! \return \c *this */
    task& operator=(task&&) noexcept = default;
    //! Resumes the coroutine and gets the current result value
    /*! The coroutine is not resumed if it is suspended at the final suspend
     * point.
     * \return the current result value, \c std::nullopt before the first
     * result value is created */
    const result_type& operator()() const {
        if (!handle.done())
            handle.resume();
        return handle.promise().current;
    }
    //! Tests if the coroutine has finished.
    /*! \return \c true if the coroutine is suspended at the final suspend
     * point, \c false otherwise */
    [[nodiscard]] bool done() const noexcept {
        return handle.done();
    }
    //! Suspends always
    /*! \return false */
    bool await_ready_impl() {
        return false;
    }
    //! Schedules itself
    /*! \param[in] hnd a handle to the currently running coroutine
     * \return a handle to a coroutine that will be resumed */
    std::coroutine_handle<void>
    await_suspend_impl([[maybe_unused]] std::coroutine_handle<void> hnd) {
        return handle;
    }
    //! Generates the result value of await_resume().
    /*! \tparam T a template parameter used only to make this member
     * function a template, which allows to define it conditionally, only
     * if \a R is \c void */
    template <std::same_as<R> T = R> requires (std::is_void_v<T>)
    void await_resume() {
    }
    //! Generates the result value of await_resume().
    /*! \tparam T a template parameter used only to make this member
     * function a template, which allows to define it conditionally, only
     * if \a R is \c void
     * \return the current result */
    template <std::same_as<R> T = R> requires (!std::is_void_v<T>)
    const result_type& await_resume() {
        return handle.promise().current;
    }
private:
    //! The coroutine handle
    handle_type handle;
};

} // namespace coro
