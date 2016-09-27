#ifndef __CORE_LOGIC_TRACE_TASK_H__
#define __CORE_LOGIC_TRACE_TASK_H__

#include "abstract_query_task.h"

namespace adservice {
namespace corelogic {

    /**
     * 处理点击模块逻辑的类
     */
    class HandleTraceTask : public AbstractQueryTask {
    public:
        explicit HandleTraceTask(adservice::utility::HttpRequest & request, adservice::utility::HttpResponse & response)
            : AbstractQueryTask(request, response)
        {
        }

        protocol::log::LogPhaseType currentPhase();

        // 期望http 请求状态
        int expectedReqStatus();

        void customLogic(ParamMap & paramMap, protocol::log::LogItem & log, adservice::utility::HttpResponse & resp);

        void onError(std::exception & e, adservice::utility::HttpResponse & resp);

        std::string usedLoggerName()
        {
            return MTTY_TRACK_LOGGER;
        }

        std::string usedLoggerConfig()
        {
            return CONFIG_TRACK_LOG;
        }
    };

} // namespace corelogic
} // namespace adservice

#endif // __CORE_LOGIC_TRACE_TASK_H__
