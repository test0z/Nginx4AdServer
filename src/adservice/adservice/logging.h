#ifndef MTCORENGINX_LOGGING_H
#define MTCORENGINX_LOGGING_H

#include "utility/utility.h"
#include <boost/log/common.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <memory>
#include <string>

enum LoggingLevel {
    TRACE_,
    DEBUG_,
    INFO_,
    WARNING_,
    ERROR_,
    FATAL_,
    NORMAL_LOG_END_ = 6,
    DEBUG_SESSION_TRACE_ = 6,
    DEBUG_SESSION_DEBUG_,
    DEBUG_SESSION_INFO_,
    DEBUG_SESSION_WARNING_,
    DEBUG_SESSION_ERROR_,
    DEBUG_SESSION_FATAL_,
    SIZE_,
};

class AdServiceLog {
public:
    explicit AdServiceLog(LoggingLevel lv, const std::string & logDirectory = "logs/");

    ~AdServiceLog();

    BOOST_LOG_GLOBAL_LOGGER(ServiceLog, boost::log::sources::severity_logger_mt<LoggingLevel>)
public:
    static LoggingLevel globalLoggingLevel;

private:
    LoggingLevel currentLevel;
};

typedef std::shared_ptr<AdServiceLog> AdServiceLogPtr;
inline AdServiceLog::ServiceLog::logger_type AdServiceLog::ServiceLog::construct_logger()
{
    return AdServiceLog::ServiceLog::logger_type();
}

extern bool inDebugSession;
extern void * debugSession;

#define LOG_TRACE                                  \
    BOOST_LOG_SEV(AdServiceLog::ServiceLog::get(), \
                  (inDebugSession ? LoggingLevel::DEBUG_SESSION_TRACE_ : LoggingLevel::TRACE_))
#define LOG_DEBUG                                  \
    BOOST_LOG_SEV(AdServiceLog::ServiceLog::get(), \
                  (inDebugSession ? LoggingLevel::DEBUG_SESSION_DEBUG_ : LoggingLevel::DEBUG_))
#define LOG_INFO                                   \
    BOOST_LOG_SEV(AdServiceLog::ServiceLog::get(), \
                  (inDebugSession ? LoggingLevel::DEBUG_SESSION_INFO_ : LoggingLevel::INFO_))
#define LOG_WARN                                   \
    BOOST_LOG_SEV(AdServiceLog::ServiceLog::get(), \
                  (inDebugSession ? LoggingLevel::DEBUG_SESSION_WARNING_ : LoggingLevel::WARNING_))
#define LOG_ERROR                                  \
    BOOST_LOG_SEV(AdServiceLog::ServiceLog::get(), \
                  (inDebugSession ? LoggingLevel::DEBUG_SESSION_ERROR_ : LoggingLevel::ERROR_))
#define LOG_FATAL                                  \
    BOOST_LOG_SEV(AdServiceLog::ServiceLog::get(), \
                  (inDebugSession ? LoggingLevel::DEBUG_SESSION_FATAL_ : LoggingLevel::FATAL_))

#endif // __MT_LOG_H__
