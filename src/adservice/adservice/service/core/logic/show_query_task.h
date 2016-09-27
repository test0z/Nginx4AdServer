//
// Created by guoze.lin on 16/4/29.
//

#ifndef ADCORE_SHOW_QUERY_TASK_H
#define ADCORE_SHOW_QUERY_TASK_H

#include "abstract_query_task.h"

namespace adservice{
    namespace corelogic{

        /**
         * 处理曝光模块逻辑的类
         */
        class HandleShowQueryTask : public AbstractQueryTask{
        public:
            static int initialized;
            static char showAdxTemplate[1024];
            static char showSspTemplate[1024];
            //加载模板
            static void loadTemplates();
        public:

            explicit HandleShowQueryTask(adservice::utility::HttpRequest & request, adservice::utility::HttpResponse & response)
                : AbstractQueryTask(request, response)
            {
            }

            virtual ~HandleShowQueryTask(){}

            protocol::log::LogPhaseType currentPhase()
            {
                return protocol::log::LogPhaseType::SHOW;
            }

            virtual int reqMethod()
            {
                return HTTP_REQUEST_GET;
            }

            /**
             * 过滤安全参数
             */
            virtual void filterParamMapSafe(ParamMap& paramMap);

            // 期望http 请求状态
            int expectedReqStatus()
            {
                return 200;
            }

            void customLogic(ParamMap & paramMap, protocol::log::LogItem & log, adservice::utility::HttpResponse & response);

            void onError(std::exception& e, adservice::utility::HttpResponse & resp)
            {
                LOG_ERROR<<"error occured in HandleShowQueryTask:"<<e.what()<<",query:"<<data;
            }
        };

    }
}

#endif //ADCORE_SHOW_QUERY_TASK_H
