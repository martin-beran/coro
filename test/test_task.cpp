/*! \file
 * \brief Tests of coro::task
 */

//! \cond
#include "coro/task.hpp"
#include "coro/sched_rr.hpp"

#define BOOST_TEST_MODULE task
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <sstream>
//! \endcond

/*! \file
 * \test noop -- A coro::task that does nothing */
//! \cond
BOOST_AUTO_TEST_CASE(noop)
{
    std::string val{};
    coro::sched_rr scheduler;
    struct coroutine {
        coro::task<void, coro::sched_rr> operator()(coro::sched_rr&,
                                                    std::string& val)
        {
            coro::log() << "coroutine running";
            val = "in coroutine";
            co_return;
        }
    };
    coro::log() << "coroutine start";
    auto co = coroutine()(scheduler, val);
    BOOST_CHECK_EQUAL(val, "");
    co();
    BOOST_CHECK_EQUAL(val, "in coroutine");
    coro::log() << "coroutine done";
}
//! \endcond

/*! \file
 * \test result -- A coro::task that yields several times and then returns (it
 * is used as a generator) */
//! \cond
BOOST_AUTO_TEST_CASE(result)
{
    coro::sched_rr scheduler;
    struct coroutine {
        coro::task<std::string, coro::sched_rr> operator()(coro::sched_rr&) {
            co_yield std::string{"Hello"};
            co_yield std::string{" "};
            co_yield std::string{"World"};
            co_return "!";
        }
    };
    std::string result{};
    coro::log() << "coroutine create";
    auto co = coroutine()(scheduler);
    coro::log() << "coroutine start";
    while (!co.done()) {
        const std::optional<std::string>& v = co();
        BOOST_REQUIRE(v.has_value());
        coro::log() << "v=" << *v;
        result.append(*v);
    }
    BOOST_CHECK_EQUAL(result, "Hello World!");
}
//! \endcond

/*! \file
 * \test await -- A coro::task that awaits another coro::task */
//! \cond
BOOST_AUTO_TEST_CASE(await)
{
    coro::sched_rr scheduler;
    struct coroutine {
        static coro::task<int, coro::sched_rr> task1(coro::sched_rr& sched,
                                                     int i1, int i2)
        {
            coro::log() << "task1 i1=" << i1 << " i2=" << i2;
            auto t2 = task2(sched, i2);
            coro::log() << "task2 created";
            std::optional<int> v = co_await t2;
            BOOST_REQUIRE(v.has_value());
            int res = i1 + *v;
            coro::log() << "task1 v=" << *v << " return=" << res;
            co_return res;
        }
        static coro::task<int, coro::sched_rr> task2(coro::sched_rr&, int i) {
            coro::log() << "task2 i=" << i;
            co_return i;
        }
    };
    coro::log() << "task1 create";
    auto t1 = coroutine::task1(scheduler, 1, 2);
    coro::log() << "coroutine start t1=" << &t1;
    std::optional<int> v = t1();
    BOOST_REQUIRE(v.has_value());
    BOOST_CHECK_EQUAL(*v, 3);
}
//! \endcond

/*! \file
 * \test yield1 -- A coro::task that resumes itself */
//! \cond
BOOST_AUTO_TEST_CASE(yield)
{
    coro::sched_rr scheduler;
    struct coroutine {
        using task_type = coro::task<std::string, coro::sched_rr>;
        static task_type task(coro::sched_rr&) {
            std::string result{};
            co_await task_type::yield{};
            result.append("Hello");
            co_await task_type::yield{};
            result.append(" ");
            co_await task_type::yield{};
            result.append("World");
            co_await task_type::yield{};
            result.append("!");
            co_return result;
        }
    };
    std::string result{};
    coro::log() << "coroutine create";
    auto co = coroutine::task(scheduler);
    coro::log() << "coroutine start";
    while (!co.done()) {
        const std::optional<std::string>& v = co();
        BOOST_REQUIRE(v.has_value());
        coro::log() << "v=" << *v;
        result.append(*v);
    }
    BOOST_CHECK_EQUAL(result, "Hello World!");
}
//! \endcond

/*! \file
 * \test yield2 -- A coro::task that resumes another task */
//! \cond
BOOST_AUTO_TEST_CASE(yield2)
{
    coro::sched_rr scheduler;
    struct coroutine {
        using task_type = coro::task<void, coro::sched_rr>;
        static task_type task1(coro::sched_rr&, std::string& result) {
            co_await task_type::yield{};
            result.append(" ");
            co_await task_type::yield{};
            result.append("!");
        }
        static task_type task2(coro::sched_rr&, std::string& result) {
            result.append("Hello");
            co_await task_type::yield{};
            result.append("World");
            co_await task_type::yield{};
        }
    };
    std::string result{};
    coro::log() << "coroutine1 create";
    auto co1 = coroutine::task1(scheduler, result);
    coro::log() << "coroutine2 create";
    auto co2 = coroutine::task2(scheduler, result);
    coro::log() << "coroutine start";
    co1();
    coro::log() << "result=" << result;
    BOOST_CHECK_EQUAL(result, "Hello World!");
}
//! \endcond

/*! \file
 * \test yield4 -- Several mutually resuming coro::task coroutines */
//! \cond
BOOST_AUTO_TEST_CASE(yield4)
{
    coro::sched_rr scheduler;
    struct coroutine {
        using task_type = coro::task<void, coro::sched_rr>;
        static task_type task1(coro::sched_rr&, std::string& result) {
            result.append("Hello");
            co_await task_type::yield{};
            result.append(" ");
            co_await task_type::yield{};
        }
        static task_type task2(coro::sched_rr&, std::string& result) {
            result.append(" ");
            co_await task_type::yield{};
            result.append("Hello");
            co_await task_type::yield{};
        }
        static task_type task3(coro::sched_rr&, std::string& result) {
            result.append("World");
            co_await task_type::yield{};
            result.append(" ");
            co_await task_type::yield{};
        }
        static task_type task4(coro::sched_rr&, std::string& result) {
            result.append("!");
            co_await task_type::yield{};
            result.append("again...");
            co_await task_type::yield{};
        }
    };
    std::string result{};
    coro::log() << "coroutine1 create";
    auto co1 = coroutine::task1(scheduler, result);
    coro::log() << "coroutine2 create";
    auto co2 = coroutine::task2(scheduler, result);
    coro::log() << "coroutine3 create";
    auto co3 = coroutine::task3(scheduler, result);
    coro::log() << "coroutine4 create";
    auto co4 = coroutine::task4(scheduler, result);
    coro::log() << "coroutine start";
    co1();
    coro::log() << "result=" << result;
    BOOST_CHECK_EQUAL(result, "Hello World! Hello again...");
}
//! \endcond

/*! \file
 * \test yield_some -- Several mutually resuming coro::task coroutines, not all
 * terminating in the same cycle */
//! \cond
BOOST_AUTO_TEST_CASE(yield_some)
{
    coro::sched_rr scheduler;
    struct coroutine {
        using task_type = coro::task<void, coro::sched_rr>;
        static task_type task1(coro::sched_rr&, std::string& result) {
            result.append("Hello");
            co_await task_type::yield{};
            result.append(" ");
            co_await task_type::yield{};
        }
        static task_type task2(coro::sched_rr&, std::string& result) {
            result.append(" ");
            co_await task_type::yield{};
        }
        static task_type task3(coro::sched_rr&, std::string& result) {
            result.append("World");
            co_await task_type::yield{};
            result.append("Hello again...");
            co_await task_type::yield{};
        }
        static task_type task4(coro::sched_rr&, std::string& result) {
            result.append("!");
            co_return;
        }
    };
    std::string result{};
    coro::log() << "coroutine1 create";
    auto co1 = coroutine::task1(scheduler, result);
    coro::log() << "coroutine2 create";
    auto co2 = coroutine::task2(scheduler, result);
    coro::log() << "coroutine3 create";
    auto co3 = coroutine::task3(scheduler, result);
    coro::log() << "coroutine4 create";
    auto co4 = coroutine::task4(scheduler, result);
    coro::log() << "coroutine start";
    co1();
    coro::log() << "result=" << result;
    BOOST_CHECK_EQUAL(result, "Hello World! Hello again...");
}
//! \endcond
