/*! \file
 * \brief Tests of coro::sync
 */

//! \cond
#include "coro/lazy.hpp"

#define BOOST_TEST_MODULE lazy
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <sstream>
//! \endcond

/*! \file
 * \test \c return_void -- a coro::lazy coroutine returning \c void */
//! \cond
BOOST_AUTO_TEST_CASE(return_void)
{
    std::string val{};
    struct coroutine {
        coro::lazy<void> operator()(std::string& val) {
            coro::log() << "coroutine running";
            val.append("Hello World!");
            co_return;
        }
    };
    coro::log() << "coroutine start";
    auto co = coroutine()(val);
    BOOST_CHECK_EQUAL(val, "");
    coro::log() << "coroutine resume";
    co.run();
    coro::log() << "check result";
    BOOST_CHECK_EQUAL(val, "Hello World!");
}
//! \endcond

/*! \file
 * \test \c return_value -- a coro::lazy coroutine returning a value */
//! \cond
BOOST_AUTO_TEST_CASE(return_value)
{
    std::string val{};
    struct coroutine {
        coro::lazy<size_t> operator()(std::string& val, std::string_view src) {
            coro::log() << "coroutine running";
            val.append(src);
            co_return val.size();
        }
    };
    std::string_view src{"Hello World!"};
    coro::log() << "coroutine start";
    auto co = coroutine()(val, src);
    BOOST_CHECK_EQUAL(val, "");
    coro::log() << "coroutine resume";
    auto val1 = co.result();
    BOOST_CHECK_EQUAL(val1, src.size());
    coro::log() << "check val1=" << val1;
    BOOST_CHECK_EQUAL(val, src);
    coro::log() << "coroutine done";
    auto val2 = std::move(co).result();
    BOOST_CHECK_EQUAL(val2, src.size());
    coro::log() << "check val2=" << val2;
    BOOST_CHECK_EQUAL(val, src);
}
//! \endcond
