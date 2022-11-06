/*! \file
 * \brief Tests of coro::task
 */

//! \cond
#include "coro/task.hpp"

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
}
//! \endcond
