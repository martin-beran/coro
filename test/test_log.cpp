/*! \file
 * \brief Tests of coro::log
 */

//! \cond
#include "coro/log.hpp"

#define BOOST_TEST_MODULE log
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <sstream>
//! \endcond

/*! \file
 * \test \c message -- a simple test that generates a log message to a string
 * stream */
//! \cond
BOOST_AUTO_TEST_CASE(message)
{
    std::ostringstream log_out;
    coro::log::set_stream(&log_out);
    coro::log() << "Hello World!";
    BOOST_TEST_INFO("log_out=" << log_out.view());
    BOOST_TEST(log_out.view().ends_with("Hello World!\n"));
}
//! \endcond
