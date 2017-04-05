#ifndef TOOL_QUERY_TASK_H
#define TOOL_QUERY_TASK_H

#include "abstract_query_task.h"

namespace adservice {
namespace corelogic {

    /**
     * 处理工具模块逻辑的类
     */
    class HandleToolQueryTask : public AbstractQueryTask {
    public:
        explicit HandleToolQueryTask(adservice::utility::HttpRequest & request,
                                     adservice::utility::HttpResponse & response)
            : AbstractQueryTask(request, response)
        {
        }

        protocol::log::LogPhaseType currentPhase();

        // 期望http 请求状态
        int expectedReqStatus();

        void customLogic(ParamMap & paramMap, protocol::log::LogItem & log, adservice::utility::HttpResponse & resp);

        void onError(std::exception & e, adservice::utility::HttpResponse & resp);
    };

} // namespace corelogic
} // namespace adservice

#endif // TOOL_QUERY_TASK_H
