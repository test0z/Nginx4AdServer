#ifndef MTCORENGINX_LOGGING_H
#define MTCORENGINX_LOGGING_H


extern "C" {
#include <ngx_core.h>
}

#include <typeinfo>
#include <string>
#include "utility/utility.h"


extern ngx_log_t* globalLog;

namespace Logging {

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

        explicit AdServiceLog(LoggingLevel lv) {
            currentLevel = lv;
            ss << adservice::utility::time::getCurrentTimeUtcString()<<" ";
            appendFormatLogLevel();
        }

        ~AdServiceLog();

        template<typename T>
        AdServiceLog &operator<<(const T &v){
            ss<<v;
            return *this;
        }

        void appendFormatLogLevel(){
            const char* logLevelStr[] = {"[TRACE] ","[DEBUG] ","[INFO] ","[WARNING] ","[ERROR] ","[FATAL] "};
            ss << logLevelStr[currentLevel];
        }

    public:
        static LoggingLevel globalLoggingLevel;
    private:
        std::stringstream ss;
        LoggingLevel currentLevel;
    };
}

#define LOG_TRACE if(AdServiceLog::globalLoggingLevel<=Logging::TRACE_)AdServiceLog(Logging::TRACE_)
#define LOG_DEBUG if(AdServiceLog::globalLoggingLevel<=Logging::DEBUG_)AdServiceLog(Logging::DEBUG_)
#define LOG_INFO if(AdServiceLog::globalLoggingLevel<=Logging::INFO_)AdServiceLog(Logging::INFO_)
#define LOG_WARN if(AdServiceLog::globalLoggingLevel<=Logging::WARNING_)AdServiceLog(Logging::WARNING_)
#define LOG_ERROR if(AdServiceLog::globalLoggingLevel<=Logging::ERROR_)AdServiceLog(Logging::ERROR_)
#define LOG_FATAL if(AdServiceLog::globalLoggingLevel<=Logging::FATAL_)AdServiceLog(Logging::FATAL_)


#endif	// __MT_LOG_H__
