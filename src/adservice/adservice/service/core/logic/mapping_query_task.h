#ifndef MAPPING_QUERY_TASK_H
#define MAPPING_QUERY_TASK_H

#include "abstract_query_task.h"

namespace adservice {
namespace corelogic {

    /**
     * 处理点击模块逻辑的类
     */
    class HandleMappingQueryTask : public AbstractQueryTask {
    public:
        explicit HandleMappingQueryTask(adservice::utility::HttpRequest & request,
                                        adservice::utility::HttpResponse & response)
            : AbstractQueryTask(request, response)
        {
        }

        protocol::log::LogPhaseType currentPhase();

        // 期望http 请求状态
        int expectedReqStatus();

        void customLogic(ParamMap & paramMap, protocol::log::LogItem & log, adservice::utility::HttpResponse & resp);

        void onError(std::exception & e, adservice::utility::HttpResponse & resp);

        static void prepareImage();

    private:
        static std::string imageData;
    };

} // namespace corelogic
} // namespace adservice

#endif // MAPPING_QUERY_TASK_H
