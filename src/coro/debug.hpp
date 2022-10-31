#pragma once

/*! \file
 * \brief Tools for generating temporary debugging messages
 */

#include <array>
#include <cassert>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <ostream>
#if __has_include(<source_location>)
#include <source_location>
#endif
#include <string_view>
#include <thread>

#ifndef __cpp_lib_source_location
#include <boost/assert/source_location.hpp>
#endif
#include <boost/process/environment.hpp>

#ifdef DOXYGEN
//! It allows to use class coro::DEBUG.
/*! If this macro is not defined, any attempt to create a coro::DEBUG
 * instance is a compile time error. It can be used to make sure that all
 * temporary debugging messages have been removed in a non-debug build. It is
 * controlled by CMake. */
#define TEMPORARY_DEBUG
#endif

#ifndef TEMPORARY_DEBUG_ENV
//! The name of the environment variable that selects a debugging output.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TEMPORARY_DEBUG_ENV "TEMPORARY_DEBUG"
#endif

#ifndef TEMPORARY_DEBUG_FORMAT_ENV
//! The name of the environment variable specifying a debugging format.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TEMPORARY_DEBUG_FORMAT_ENV "TEMPORARY_DEBUG_FORMAT"
#endif

namespace coro {

//! An instance of this class is used to write a single debugging message.
/*! This class is intended to be used similarly to an output stream, using \c
 * operator<<() to compose a message. Values of any type, which has a stream
 * output \c operator<<(), can be written. Each message is terminated by a
 * newline and the output stream is flushed. Example:
 * \code
 * DEBUG() << "value=" << val;
 * \endcode
 * An instance of \ref DEBUG may be created only if macro TEMPORARY_DEBUG is
 * defined.
 *
 * Destination of debugging messages is controlled by the environment variable
 * which name is the value of macro TEMPORARY_DEBUG_ENV.
 * Possible values:
 * \arg The empty value means that debugging message will not be written.
 * \arg cout -- Write to \c std::cout
 * \arg cerr -- Write to \c std::cerr
 * \arg Any other value is taken as a file name, which will be opened for
 * appending.
 *
 * If the environment variable does not exist, the default value is \c cerr.
 * \threadsafe{safe\, each message is written atomically\, parts of messages
 * from different threads are not mixed in the output, unsafe}
 *
 * Formatting of messages is controlled by the environment variable which name
 * is the value of macro TEMPORARY_DEBUG_FORMAT_ENV. It contains zero or
 * more formatting flags:
 * \arg p -- Includes PID in each message.
 * \arg t -- Includes thread ID in each message.
 *
 * No flags are enabled by default. If the flags are followed by a space or
 * colon and some (possibly emtpy) text, this text is used as the message
 * prefix, instead of the default \c DBG.
 *
 * \note If several instances of \ref DEBUG exist in a thread at the same time,
 * only the one created first produces output. This rule prevents deadlocks or
 * garbled output if a \ref DEBUG is invoked recursively when generating a
 * debugging message, e.g., by a debugging message in an \c operator<<().
 *
 * \note Debugging messages generated after destroying file_os will not be
 * written. Such message can be written, e.g., by destructors of objects with
 * static storage duration destroyed after file_os.
 *
 * \note The name of this class is in upper case and does not follow the code
 * style rules in order to be easily found in source code. */
class DEBUG {
public:
    //! The name of the environment variable that selects a debugging output.
    static constexpr std::string_view env_var = TEMPORARY_DEBUG_ENV;
    //! The name of the environment variable that defines a message format.
    static constexpr std::string_view env_var_format =
        TEMPORARY_DEBUG_FORMAT_ENV;
#ifndef TEMPORARY_DEBUG
private: // Making constructors private makes this class unusable
#endif
    //! Default constructor.
    /*! \param[in] loc the location in the source code where this message is
     * generated */
#ifdef __cpp_lib_source_location
    // NOLINTNEXTLINE(modernize-use-equals-delete)
    explicit DEBUG(const std::source_location& loc =
                   std::source_location::current());
#else
    explicit DEBUG(const boost::source_location& loc = {});
#endif
public: // NOLINT(readability-redundant-access-specifiers)
    //! No copying
    DEBUG(const DEBUG&) = delete;
    //! No moving
    DEBUG(DEBUG&&) = delete;
    //! The destructor finishes writing the message.
    ~DEBUG();
    //! No copying
    DEBUG& operator=(const DEBUG&) = delete;
    //! No moving
    DEBUG& operator=(DEBUG&&) = delete;
private:
    //! Global initialization of debugging output
    /*! It opens the output stream and initializes a mutex for synchronization
     * of DEBUG() instances in different threads.
     * \return a lock to be stored in \ref lck */
    static std::unique_lock<std::mutex> init();
    //! An internal function used by init()
    static void init_once();
    std::unique_lock<std::mutex> lck; //!< The lock held by this object
    //! The output stream; \c nullptr if debugging output is disabled
    std::ostream* os = nullptr;
    //! Used to detect if a \ref DEBUG instance exists in the current thread.
    static thread_local bool active;
    static std::mutex mtx; //!< The mutex for synchronizing different instances
    //! The output stream; \c nullptr if debugging output is disabled.
    static std::ostream* common_os;
    //! The output file stream; \c nullptr if not using a file
    static std::unique_ptr<std::ofstream> file_os;
    static std::string prefix; //!< A message prefix
    static bool fmt_pid; //!< Add PID to messages
    static bool fmt_tid; //!< Add thread ID to messages
    //! Appends a value to the debugging message currently being built.
    /*! \tparam T the type of a written value
     * \param[in] dbg a debugging object
     * \param[in] v a value to be written
     * \return \a dbg */
    template <class T> friend DEBUG&& operator<<(DEBUG&& dbg, T&& v) {
        if (dbg.os)
            *dbg.os << std::forward<T>(v);
        return std::move(dbg);
    }
};

/*** DEBUG *******************************************************************/

inline thread_local bool DEBUG::active = false;

inline std::ostream* DEBUG::common_os = nullptr;

inline std::unique_ptr<std::ofstream> DEBUG::file_os = nullptr;

inline bool DEBUG::fmt_pid = false;

inline bool DEBUG::fmt_tid = false;

inline std::mutex DEBUG::mtx;

inline std::string DEBUG::prefix{};

#ifdef __cpp_lib_source_location
inline DEBUG::DEBUG(const std::source_location& loc):
#else
inline DEBUG::DEBUG(const boost::source_location& loc):
#endif
    lck(init())
{
    if (active) {
        assert(!lck);
        return;
    }
    assert(lck);
    active = true;
    os = common_os;
    if (os) {
        namespace sc = std::chrono;
        auto now =
            sc::time_point_cast<sc::microseconds>(sc::system_clock::now()).
            time_since_epoch().count();
        time_t sec = now / 1'000'000;
        int usec = int(now % 1'000'000);
        tm hms{};
        localtime_r(&sec, &hms);
        // hh:mm:ss.uuuuuu\0
        std::array<char, 16> t{};
        assert(snprintf(t.data(), t.size(), "%02d:%02d:%02d.%06d",
                        int(hms.tm_hour), int(hms.tm_min), int(hms.tm_sec),
                        usec) == t.size() - 1);
        std::string_view file = loc.file_name();
        if (auto i = file.rfind('/'); i != std::string_view::npos)
            file = file.substr(i + 1);
        *os << prefix;
        if (fmt_pid)
            *os << ' ' << boost::this_process::get_id();
        if (fmt_tid)
            * os << ' ' << std::this_thread::get_id();
        *os << ' ' << t.data() << ' ' << file << ':' << loc.line() << ' ';
    }
}

inline DEBUG::~DEBUG()
{
    if (lck) {
        std::move(*this) << std::endl<char, std::ostream::traits_type>;
        active = false;
    }
}

inline std::unique_lock<std::mutex> DEBUG::init()
{
    if (active)
        return {};
    static std::once_flag once;
    std::call_once(once, init_once);
    return std::unique_lock{mtx};
}

inline void DEBUG::init_once()
{
    using namespace std::string_view_literals;
    // NOLINTNEXTLINE(concurrency-mt-unsafe): MT access not expected
    char* p = std::getenv(env_var.data());
    std::string_view env = p ? p : "cerr"sv;
    if (env.empty())
        ; // not writing debugging messages
    else if (env == "cout"sv)
        common_os = &std::cout;
    else if (env == "cerr"sv)
        common_os = &std::cerr;
    else {
        file_os = std::make_unique<std::ofstream>(env.data(),
                                    std::ios_base::out | std::ios_base::app);
        common_os = file_os.get();
    }
    // NOLINTNEXTLINE(concurrency-mt-unsafe): MT access not expected
    p = std::getenv(env_var_format.data());
    prefix = "DBG";
    std::string_view fmt = p ? p : std::string_view{prefix};
    for (size_t i = 0; i < fmt.size(); ++i)
        switch (fmt[i]) {
        case 'p':
            fmt_pid = true;
            break;
        case 't':
            fmt_tid = true;
            break;
        case ' ':
        case ':':
            prefix = fmt.substr(i + 1);
            goto end_for;
        default:
            goto end_for;
        }
end_for:
    ;
}

} // namespace coro
