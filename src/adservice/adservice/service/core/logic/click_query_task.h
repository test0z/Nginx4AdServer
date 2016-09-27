//
// Created by guoze.lin on 16/4/29.
//

#ifndef ADCORE_CLICK_QUERY_TASK_H
#define ADCORE_CLICK_QUERY_TASK_H

#include "abstract_query_task.h"

namespace adservice {
namespace corelogic {

    /**
     * 处理点击模块逻辑的类
     */
    class HandleClickQueryTask : public AbstractQueryTask {
    public:
        explicit HandleClickQueryTask(adservice::utility::HttpRequest & request, adservice::utility::HttpResponse & response)
            : AbstractQueryTask(request, response)
        {
        }

        protocol::log::LogPhaseType currentPhase();

        // 期望http 请求状态
        int expectedReqStatus();

        void customLogic(ParamMap & paramMap, protocol::log::LogItem & log, adservice::utility::HttpResponse & resp);

        void onError(std::exception & e, adservice::utility::HttpResponse & resp);

        void handleLandingUrl(protocol::log::LogItem & logItem, ParamMap & paramMap);
    };

} // namespace corelogic
} // namespace adservice

#endif // ADCORE_CLICK_QUERY_TASK_H
