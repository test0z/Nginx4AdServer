#include "logging.h"
#include <iostream>

LoggingLevel AdServiceLog::globalLoggingLevel = LoggingLevel::INFO_;

AdServiceLog::AdServiceLog(LoggingLevel lv)
{
	currentLevel = lv;
	ss << adservice::utility::time::getCurrentTimeUtcString() << " ";
	appendFormatLogLevel();
}

AdServiceLog::~AdServiceLog()
{
	ss << '\n';
	//ngx_log_error(NGX_LOG_EMERG, globalLog, 0, ss.str().data());
	std::cerr<<ss.str();
}

void AdServiceLog::appendFormatLogLevel()
{
	const char * logLevelStr[] = { "[TRACE] ", "[DEBUG] ", "[INFO] ", "[WARNING] ", "[ERROR] ", "[FATAL] " };
	ss << logLevelStr[currentLevel];
}
