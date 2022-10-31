#pragma once

/*! \file
 * \brief An include file existing only for declarations useful for building
 * documetation by Doxygen
 */

#ifdef DOXYGEN
//! Distinguish between a compiler and Doxygen.
/*! This macro is defined during Doxygen building documentation and undefined
 * during compilation. It can be used to selectively hide code from Doxygen or
 * from the compiler. */
#define DOXYGEN
#endif
