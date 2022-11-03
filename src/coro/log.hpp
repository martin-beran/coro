#pragma once

/*! \file
 * \brief Logging and temporary debugging messages
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
#define ENABLE_TEMPORARY_DEBUG
#endif

#ifndef ENABLE_LOG
//! This macro globally enables or disables logging
#define ENABLE_LOG true
#endif

#ifndef LOG_OUTPUT_ENV
//! The name of the environment variable that selects a logging output.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_OUTPUT_ENV "LOG_OUTPUT"
#endif

#ifndef LOG_FORMAT_ENV
//! The name of the environment variable specifying a logging format.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_FORMAT_ENV "LOG_FORMAT"
#endif

namespace coro {

//! An instance of this class is used to write a single log message.
/*! This class is intended to be used similarly to an output stream, using \c
 * operator<<() to compose a message. Values of any type, which has a stream
 * output \c operator<<(), can be written. Each message is terminated by a
 * newline and the output stream is flushed. Example:
 * \code
 * log() << "value=" << val;
 * \endcode
 *
 * Destination of debugging messages is controlled by the environment variable
 * which name is the value of macro LOG_OUTPUT_ENV.
 * Possible values:
 * \arg The empty value means that debugging message will not be written.
 * \arg cout -- Write to \c std::cout
 * \arg cerr -- Write to \c std::cerr
 * \arg Any other value is taken as a file name, which will be opened for
 * appending.
 *
 * If the environment variable does not exist, the default value is \c cerr.
 * The log output can be changed programmaticaly by setting the underlying
 * output stream by set_stream().
 * \threadsafe{safe\, each message is written atomically\, parts of messages
 * from different threads are not mixed in the output, unsafe}
 *
 * Formatting of messages is controlled by the environment variable which name
 * is the value of macro LOG_FORMAT_ENV. It contains zero or more formatting
 * flags:
 * \arg p -- Includes PID in each message.
 * \arg t -- Includes thread ID in each message.
 *
 * No flags are enabled by default. If the flags are followed by a space or
 * colon and some (possibly emtpy) text, this text is used as the message
 * prefix in the derived class \ref DEBUG, instead of the default \c DBG.
 *
 * \note If several instances of \ref log exist in a thread at the same time,
 * only the one created first produces output. This rule prevents deadlocks or
 * garbled output if a \ref log is invoked recursively when generating a
 * log message, e.g., by a log message in an \c operator<<().
 *
 * \note Log messages generated after destroying file_os will not be
 * written. Such message can be written, e.g., by destructors of objects with
 * static storage duration destroyed after file_os.
 * \test in file test_log.cpp */
class log {
public:
    //! Enables or disables logging globally
    /*! \note Using this member variable instead of using the macro directly
     * silences some warnings about Boolean literals in conditions. */
    static constexpr bool log_enable = ENABLE_LOG;
    //! The name of the environment variable that selects a debugging output.
    static constexpr std::string_view env_var_output = LOG_OUTPUT_ENV;
    //! The name of the environment variable that defines a message format.
    static constexpr std::string_view env_var_format = LOG_FORMAT_ENV;
    //! Default constructor.
    /*! \param[in] loc the location in the source code where this message is
     * generated */
#ifdef __cpp_lib_source_location
    // NOLINTNEXTLINE(modernize-use-equals-delete)
    explicit log(const std::source_location& loc =
                 std::source_location::current()): log(false, loc) {}
#else
    explicit log(const boost::source_location& loc = {}): log(false, loc) {}
#endif
public: // NOLINT(readability-redundant-access-specifiers)
    //! No copying
    log(const log&) = delete;
    //! No moving
    log(log&&) = delete;
    //! The destructor finishes writing the message.
    ~log();
    //! No copying
    log& operator=(const log&) = delete;
    //! No moving
    log& operator=(log&&) = delete;
    //! Sets the output stream for all future instances of this class
    /*! It overrides any logging configuration done by environment variables.
     * \param[in] os the output stream, or \c nullptr to disable logging
     * \threadsafe{unsafe, unsafe} */
    static void set_stream(std::ostream* os) {
        common_os = os;
        stream_override = true;
    }
    //! Sets the output format
    /*! It overrides any formatting flags defined by an environment variable.
     * \param[in] pid whether to include PID in messages
     * \param[in] tid whether to include thread ID in messages */
    static void set_format(bool pid, bool tid) {
        fmt_pid = pid;
        fmt_tid = tid;
        fmt_override = true;
    }
protected:
    //! Creates the object and writes the initial part of a message.
    /*! \param[in] write_prefix whether to write \ref prefix
     * \param[in] loc the location in the source code where this message is
     * generated */
#ifdef __cpp_lib_source_location
    // NOLINTNEXTLINE(modernize-use-equals-delete)
    explicit log(bool write_prefix, const std::source_location& loc);
#else
    explicit log(bool write_prefix, const boost::source_location& loc);
#endif
private:
    //! Global initialization of debugging output
    /*! It opens the output stream and initializes a mutex for synchronization
     * of \ref log instances in different threads.
     * \return a lock to be stored in \ref lck */
    static std::unique_lock<std::mutex> init();
    //! An internal function used by init()
    static void init_once();
    std::unique_lock<std::mutex> lck; //!< The lock held by this object
    //! The output stream; \c nullptr if logging output is disabled
    std::ostream* os = nullptr;
    //! Used to detect if a \ref log instance exists in the current thread.
    static thread_local bool active;
    static std::mutex mtx; //!< The mutex for synchronizing different instances
    //! The output stream; \c nullptr if log output is disabled.
    static std::ostream* common_os;
    //! Indicates that set_stream() has been called.
    static bool stream_override;
    //! The output file stream; \c nullptr if not using a file
    static std::unique_ptr<std::ofstream> file_os;
    static std::string prefix; //!< A message prefix for class \ref DEBUG
    static bool fmt_pid; //!< Add PID to messages
    static bool fmt_tid; //!< Add thread ID to messages
    //! Indicates that set_format() has been called.
    static bool fmt_override;
    //! Appends a value to the debugging message currently being built.
    /*! \tparam T the type of a written value
     * \param[in] dbg a debugging object
     * \param[in] v a value to be written
     * \return \a dbg */
    template <class T> friend log&& operator<<(log&& dbg, T&& v) {
        if constexpr (log_enable)
            if (dbg.os)
                *dbg.os << std::forward<T>(v);
        return std::move(dbg);
    }
};

//! An instance of this class is used to write one temporary debugging message.
/*! This class is the same as \ref log, but it should be used for temporary
 * messages, usually used for debugging, that will be removed as soon as not
 * needed. To distinguish normal and temporary messages, each message generated
 * by \ref DEBUG is prefixed by log::prefix.
 *
 * An instance of \ref DEBUG may be created only if macro
 * ENABLE_TEMPORARY_DEBUG is defined.
 *
 * \note The name of this class is in upper case and does not follow the code
 * style rules in order to be easily found in source code. */
class DEBUG: public log {
#ifdef ENABLE_TEMPORARY_DEBUG
public:
#endif
    //! Default constructor. It writes the message prefix.
    /*! \param[in] loc the location in the source code where this message is
     * generated */
#ifdef __cpp_lib_source_location
    // NOLINTNEXTLINE(modernize-use-equals-delete)
    explicit DEBUG(const std::source_location& loc =
                   std::source_location::current()): log(true, loc) {}
#else
    explicit DEBUG(const boost::source_location& loc = {}): log(true, loc) {}
#endif
};

/*** log *********************************************************************/

inline thread_local bool log::active = false;
inline std::ostream* log::common_os = nullptr;
inline bool log::stream_override = false;
inline std::unique_ptr<std::ofstream> log::file_os = nullptr;
inline bool log::fmt_pid = false;
inline bool log::fmt_tid = false;
inline bool log::fmt_override = false;
inline std::mutex log::mtx;
inline std::string log::prefix{};

#ifdef __cpp_lib_source_location
inline log::log(bool write_prefix, const std::source_location& loc):
#else
inline log::log(bool write_prefix, const boost::source_location& loc):
#endif
    lck(init())
{
    if constexpr (!log_enable)
        return;
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
        using namespace std::string_view_literals;
        auto space = ""sv;
        if (write_prefix) {
            *os << prefix;
            space = " "sv;
        }
        if (fmt_pid) {
            *os << space << boost::this_process::get_id();
            space = " "sv;
        }
        if (fmt_tid) {
            * os << space << std::this_thread::get_id();
            space = " "sv;
        }
        *os << space << t.data() << ' ' << file << ':' << loc.line() << ' ';
    }
}

inline log::~log()
{
    if constexpr (!log_enable)
        return;
    if (lck) {
        std::move(*this) << std::endl<char, std::ostream::traits_type>;
        active = false;
    }
}

inline std::unique_lock<std::mutex> log::init()
{
    if constexpr (!log_enable)
        return {};
    if (active)
        return {};
    static std::once_flag once;
    std::call_once(once, init_once);
    return std::unique_lock{mtx};
}

inline void log::init_once()
{
    using namespace std::string_view_literals;
    if (!stream_override) {
        // NOLINTNEXTLINE(concurrency-mt-unsafe): MT access not expected
        char* p = std::getenv(env_var_output.data());
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
    }
    // NOLINTNEXTLINE(concurrency-mt-unsafe): MT access not expected
    char* p = std::getenv(env_var_format.data());
    prefix = "DBG";
    std::string_view fmt = p ? p : std::string_view{prefix};
    for (size_t i = 0; i < fmt.size(); ++i)
        switch (fmt[i]) {
        case 'p':
            if (!fmt_override)
                fmt_pid = true;
            break;
        case 't':
            if (!fmt_override)
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
