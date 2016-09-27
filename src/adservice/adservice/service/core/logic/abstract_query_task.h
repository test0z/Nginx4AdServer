//
// Created by guoze.lin on 16/4/29.
//

#ifndef ADCORE_ABSTRACT_QUERY_TASK_H
#define ADCORE_ABSTRACT_QUERY_TASK_H

//#include "core_service.h"
#include "common/types.h"
#include "core/logpusher/log_pusher.h"
#include "protocol/log/log.h"
#include "task_thread_data.h"
#include "utility/utility.h"
#ifdef USE_ENCODING_GZIP
#include <muduo/net/ZlibStream.h>
#endif
#include <cppcms/http_cookie.h>
#include <cppcms/http_request.h>
#include <cppcms/http_response.h>
#include <exception>
#include "logging.h"

namespace adservice {

namespace corelogic {

    using namespace Logging;
    using namespace adservice::server;
    using namespace adservice::utility::serialize;
    using namespace adservice::utility::cypher;
    using namespace adservice::utility::url;
    using namespace adservice::utility::hash;
    using namespace adservice::utility::file;

    /**
     * 处理请求的抽象逻辑
     */
    class AbstractQueryTask {
    public:
        explicit AbstractQueryTask(adservice::utility::HttpRequest & request, adservice::utility::HttpResponse & response)
            : resp(response)
        {
            isPost = request.request_method() == "POST";
            if (isPost) {
                const std::string& postdata = request.raw_post_data();
                data = postdata;
            } else {
                data = request.query_string();
            }
            userCookies = request.http_cookie();
            userAgent = request.http_user_agent();
            userIp = request.remote_addr();
            referer = request.http_referer();
            needLog = true;
            updateThreadData();
        }

        void updateThreadData();

        virtual ~AbstractQueryTask()
        {
            //    conn.reset();
        }

        /**
         * 过滤安全参数
         */
        virtual void filterParamMapSafe(ParamMap & paramMap);

        /**
         * 处理请求的通用逻辑
         * 1.装填log 对象并序列化
         * 2.发送日志
         * 3.准备http response
         */
        void commonLogic(ParamMap & paramMap, protocol::log::LogItem & log, adservice::utility::HttpResponse & resp);

        virtual protocol::log::LogPhaseType currentPhase() = 0;

        // 触发的HTTP请求方法
        virtual int reqMethod()
        {
            return HTTP_REQUEST_GET;
        }

        // 期望http 请求状态
        virtual int expectedReqStatus()
        {
            return 200;
        }

        virtual void getPostParam(ParamMap & paramMap)
        {
        }

        // deal with custom bussiness
        virtual void customLogic(ParamMap & paramMap, protocol::log::LogItem & log, adservice::utility::HttpResponse & response)
            = 0;

        // set error detail to response body
        virtual void onError(std::exception & e, adservice::utility::HttpResponse & response) = 0;

        virtual std::string usedLoggerName()
        {
            return MTTY_SERVICE_LOGGER;
        }

        virtual std::string usedLoggerConfig()
        {
            return CONFIG_LOG;
        }

        void setLogger(const adservice::log::LogPusherPtr & logger)
        {
            serviceLogger = logger;
        }

        void doLog(protocol::log::LogItem & log);

        void operator()();

    protected:
        adservice::types::string userCookies;
        adservice::types::string userAgent;
        adservice::types::string userIp;
        adservice::types::string data;
        adservice::types::string referer;
        bool isPost;
        bool needLog;
        adservice::utility::HttpResponse & resp;
        TaskThreadLocal * threadData;
        adservice::log::LogPusherPtr serviceLogger;
    };
}
}

#endif // ADCORE_ABSTRACT_QUERY_TASK_H
