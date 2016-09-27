#include "logging.h"

namespace Logging{

    LoggingLevel AdServiceLog::globalLoggingLevel = LoggingLevel::INFO_;

    AdServiceLog::~AdServiceLog() {
        ss<<'\n';
        ngx_log_error(NGX_LOG_EMERG, globalLog, 0, ss.str().data());
    }

}