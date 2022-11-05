/*! \file
 * \brief Tests of coro::sync
 */

//! \cond
#include "coro/sync.hpp"

#define BOOST_TEST_MODULE sync
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <sstream>
//! \endcond

/*! \file
 * \test \c return_void -- a coro::sync coroutine returning \c void */
//! \cond
BOOST_AUTO_TEST_CASE(return_void)
{
    std::string val{};
    struct coroutine {
        coro::sync<void> operator()(std::string& val) {
            coro::log() << "coroutine running";
            val.append("Hello World!");
            co_return;
        }
    };
    coroutine()(val);
    BOOST_CHECK_EQUAL(val, "Hello World!");

}
//! \endcond

/*! \file
 * \test \c return_value -- a coro::sync coroutine returning a value */
//! \cond
BOOST_AUTO_TEST_CASE(return_value)
{
    struct coroutine {
        coro::sync<std::string> operator()() {
            coro::log() << "coroutine running";
            co_return std::string{"Hello World!"};
        }
    };
    auto val = coroutine()();
    BOOST_CHECK_EQUAL(val.result(), "Hello World!");
    BOOST_CHECK_EQUAL(std::move(val).result(), "Hello World!");

}
//! \endcond
