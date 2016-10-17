#ifndef MTCORENGINX_LOGGING_H
#define MTCORENGINX_LOGGING_H


#include "utility/utility.h"
#include <string>
#include <memory>
#include <boost/log/common.hpp>
#include <boost/log/sinks/text_file_backend.hpp>


enum LoggingLevel {
	TRACE_,
	DEBUG_,
	INFO_,
	WARNING_,
	ERROR_,
	FATAL_,
	SIZE_,
};

class AdServiceLog {
public:
	explicit AdServiceLog(LoggingLevel lv,const std::string& logDirectory = "logs/");

	~AdServiceLog();

	BOOST_LOG_GLOBAL_LOGGER(ServiceLog, boost::log::sources::severity_logger_mt<LoggingLevel>)
public:
	static LoggingLevel globalLoggingLevel;

private:
	LoggingLevel currentLevel;
};

typedef std::shared_ptr<AdServiceLog> AdServiceLogPtr;
inline AdServiceLog::ServiceLog::logger_type AdServiceLog::ServiceLog::construct_logger(){
	return AdServiceLog::ServiceLog::logger_type();
}

#define LOG_TRACE BOOST_LOG_SEV(AdServiceLog::ServiceLog::get(),LoggingLevel::TRACE_)
#define LOG_DEBUG BOOST_LOG_SEV(AdServiceLog::ServiceLog::get(), LoggingLevel::DEBUG_)
#define LOG_INFO BOOST_LOG_SEV(AdServiceLog::ServiceLog::get(), LoggingLevel::INFO_)
#define LOG_WARN BOOST_LOG_SEV(AdServiceLog::ServiceLog::get(), LoggingLevel::WARNING_)
#define LOG_ERROR BOOST_LOG_SEV(AdServiceLog::ServiceLog::get(), LoggingLevel::ERROR_)
#define LOG_FATAL BOOST_LOG_SEV(AdServiceLog::ServiceLog::get(), LoggingLevel::FATAL_)

#endif // __MT_LOG_H__
