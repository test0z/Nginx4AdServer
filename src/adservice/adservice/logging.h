#ifndef MTCORENGINX_LOGGING_H
#define MTCORENGINX_LOGGING_H

extern "C" {
#include <ngx_core.h>
}

#include "utility/utility.h"
#include <string>

extern ngx_log_t * globalLog;

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
	explicit AdServiceLog(LoggingLevel lv);

	~AdServiceLog();

	template <typename T>
	AdServiceLog & operator<<(const T & v)
	{
		ss << v;
		return *this;
	}

	void appendFormatLogLevel();

public:
	static LoggingLevel globalLoggingLevel;

private:
	std::stringstream ss;
	LoggingLevel currentLevel;
};

#define LOG_TRACE                                                 \
	if (AdServiceLog::globalLoggingLevel <= LoggingLevel::TRACE_) \
	AdServiceLog(LoggingLevel::TRACE_)
#define LOG_DEBUG                                                 \
	if (AdServiceLog::globalLoggingLevel <= LoggingLevel::DEBUG_) \
	AdServiceLog(LoggingLevel::DEBUG_)
#define LOG_INFO                                                 \
	if (AdServiceLog::globalLoggingLevel <= LoggingLevel::INFO_) \
	AdServiceLog(LoggingLevel::INFO_)
#define LOG_WARN                                                    \
	if (AdServiceLog::globalLoggingLevel <= LoggingLevel::WARNING_) \
	AdServiceLog(LoggingLevel::WARNING_)
#define LOG_ERROR                                                 \
	if (AdServiceLog::globalLoggingLevel <= LoggingLevel::ERROR_) \
	AdServiceLog(LoggingLevel::ERROR_)
#define LOG_FATAL                                                 \
	if (AdServiceLog::globalLoggingLevel <= LoggingLevel::FATAL_) \
	AdServiceLog(LoggingLevel::FATAL_)

#endif // __MT_LOG_H__
