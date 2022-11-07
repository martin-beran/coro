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
BOOST_AUTO_TEST_CASE(await)
{
    coro::sched_rr scheduler;
    struct coroutine {
        static coro::task<int, coro::sched_rr> task1(coro::sched_rr& sched,
                                                     int i1, int i2)
        {
            coro::log() << "task1 i1=" << i1 << " i2=" << i2;
            auto t2 = task2(sched, i2);
            std::optional<int> v = co_await t2;
            BOOST_CHECK(v.has_value());
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
