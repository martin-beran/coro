/*! \file
 * \brief Tests of coro::generator
 */

//! \cond
#include "coro/generator.hpp"
#include <limits>

#define BOOST_TEST_MODULE generator
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <sstream>
//! \endcond

/*! \file
 * \test empty -- a coro::generator coroutine returning an empty sequence */
//! \cond
BOOST_AUTO_TEST_CASE(empty)
{
    std::string val{};
    struct coroutine {
        coro::generator<std::string> operator()() {
            coro::log() << " coroutine running";
            co_return;
        }
    };
    coro::log() << "coroutine start";
    for (auto&& v: coroutine()()) {
        coro::log() << "generated=" << val;
        val.append(v);
    }
    coro::log() << "loop done val=" << val;
    BOOST_CHECK_EQUAL(val, "");
}
//! \endcond

/*! \file
 * \test nonempty -- a coro::generator coroutine returning an nonempty sequence
 */
//! \cond
BOOST_AUTO_TEST_CASE(nonempty)
{
    std::string val{};
    struct coroutine {
        coro::generator<std::string> operator()() {
            coro::log() << " coroutine running";
            co_yield "Hello";
            co_yield " ";
            co_yield "World";
            co_yield "!";
            co_return;
        }
    };
    coro::log() << "coroutine start";
    for (auto&& v: coroutine()()) {
        coro::log() << "generated=" << v;
        val.append(v);
    }
    coro::log() << "loop done val=" << val;
    BOOST_CHECK_EQUAL(val, "Hello World!");
}
//! \endcond

/*! \file
 * \test int_seq -- a coro::generator coroutine returning an integer sequence
 */
//! \cond
BOOST_AUTO_TEST_CASE(int_seq)
{
    struct coroutine {
        coro::generator<int> operator()(int n) {
            for (int i = 0; i < n; ++i)
                co_yield int(i);
        }
    };
    int n = 10;
    coro::log() << "coroutine start";
    auto co = coroutine()(n);
    for (int i = 0; i < n; ++i) {
        coro::log() << "i=" << i;
        BOOST_CHECK_EQUAL(co().value(), i);
    }
    coro::log() << "last call";
    BOOST_CHECK(!co().has_value());
    coro::log() << "coroutine done";
}
//! \\endcond

/*! \file
 * \test infinite_seq -- a coro::generator coroutine returning an infinite
 * sequence
 */
//! \cond
BOOST_AUTO_TEST_CASE(infinite_seq)
{
    struct coroutine {
        static coro::generator<int> inf_seq() {
            for (int i = 0; true; ++i) {
                co_yield int(i);
                if (i == std::numeric_limits<decltype(i)>::max())
                    throw std::overflow_error("max");
            } 
        }
    };
    int max = 0;
    for (auto co = coroutine::inf_seq();;) {
        auto v = co();
        BOOST_REQUIRE(v.has_value());
        if (v > max && *v % 3 == 0)
            max = *v;
        if (v >= 20)
            break;
    }
    BOOST_CHECK_EQUAL(max, 18);
}
//! \\endcond
