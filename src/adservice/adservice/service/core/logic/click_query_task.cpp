//
// Created by guoze.lin on 16/4/29.
//

#include "click_query_task.h"

#include "core/config_types.h"
#include "core/model/source_id.h"
#include "logging.h"
#include "utility/url.h"

extern MT::common::Aerospike aerospikeClient;
extern GlobalConfig globalConfig;

namespace adservice {
namespace corelogic {

    namespace {

        void fill(char *& p, const char * src)
        {
            while (*src != '\0') {
                *p++ = *src++;
            }
        }

        core::model::SourceId generateSourceId()
        {
            MT::common::ASKey key(globalConfig.aerospikeConfig.nameSpace.c_str(), "source_id_seq", "last_source_id");

            MT::common::ASOperation op(2);
            op.addRead("val");
            op.addIncr("val", (int64_t)1);

            core::model::SourceId sourceId;
            try {
                aerospikeClient.operate(key, op, sourceId);
            } catch (MT::common::AerospikeExcption & e) {
                LOG_ERROR << "生成sourceId失败，" << e.what() << "，code：" << e.error().code << "，msg："
                          << e.error().message << "，调用堆栈：" << std::endl
                          << e.trace();
                return sourceId;
            }

            return sourceId;
        }

        void recordSource(ParamMap & paramMap, protocol::log::LogItem & log)
        {
            std::string userId = log.userId, ownerId = paramMap[URL_ADOWNER_ID];
            if (userId.empty() || ownerId.empty()) {
                LOG_ERROR << "用户id：“" << userId << "”或广告主id：“" << ownerId << "”为空！";
                return;
            }

            /* 记录访问信息到缓存 */
            // 生成source id
            core::model::SourceId sourceId = generateSourceId();
            if (sourceId.get().empty()) {
                LOG_ERROR << "生成的sourceid为空。";
                return;
            }

            paramMap["source_id"] = sourceId.get();

            // 填充数据
            core::model::SourceRecord sourceRecord(paramMap, log);
            MT::common::ASKey key(globalConfig.aerospikeConfig.nameSpace.c_str(), "source_id", sourceId.get().c_str());
            try {
                aerospikeClient.put(key, sourceRecord);
            } catch (MT::common::AerospikeExcption & e) {
                LOG_ERROR << "记录source record失败，" << e.what() << "，code：" << e.error().code << "，msg："
                          << e.error().message << "，调用堆栈：" << std::endl
                          << e.trace();
                return;
            }

            /* 记录索引信息，方便t模块根据用户ID和广告主ID查询到source id */
            MT::common::ASKey indexKey(globalConfig.aerospikeConfig.nameSpace.c_str(), "source_id_index",
                                       (userId + ownerId).c_str());
            try {
                aerospikeClient.put(indexKey, sourceId);
            } catch (MT::common::AerospikeExcption & e) {
                LOG_ERROR << "记录sourceId index失败，" << e.what() << "，code：" << e.error().code << "，msg："
                          << e.error().message << "，调用堆栈：" << std::endl
                          << e.trace();
                return;
            }
        }

        bool replace(std::string & str, const std::string & from, const std::string & to)
        {
            size_t start_pos = str.find(from);
            if (start_pos == std::string::npos)
                return false;
            str.replace(start_pos, from.length(), to);
            return true;
        }

    } // anonymous namespace

    protocol::log::LogPhaseType HandleClickQueryTask::currentPhase()
    {
        return protocol::log::LogPhaseType::CLICK;
    }

    int HandleClickQueryTask::expectedReqStatus()
    {
        return 302;
    }

    void HandleClickQueryTask::handleLandingUrl(protocol::log::LogItem & logItem, ParamMap & paramMap)
    {
        paramMap["advid"] = paramMap[URL_ADOWNER_ID];
        paramMap["pid"] = paramMap[URL_MTTYADPLACE_ID];
        paramMap["adxpid"] = paramMap[URL_ADPLACE_ID];
        paramMap["bid"] = paramMap[URL_CREATIVE_ID];
        paramMap["sid"] = paramMap[URL_EXEC_ID];
        paramMap["idfa"] = paramMap[URL_DEVICE_UID];
        paramMap["ip"] = userIp;

        char buffer[1024];
        char result[1024];
        //	std::string output;
        //	url::urlDecode_f(logItem.adInfo.landingUrl, output, buffer);
        memcpy(buffer, logItem.adInfo.landingUrl.data(), logItem.adInfo.landingUrl.length());
        buffer[logItem.adInfo.landingUrl.length()] = '\0';
        char *p1 = buffer, *p = result;
        while (*p1 != '\0') {
            while (*p1 != '{' && *p1 != '\0') {
                *p++ = *p1++;
            }

            if (*p1 == '\0') {
                break;
            }

            char * p2 = ++p1;
            while (*p2 != '}' && *p2 != '\0') {
                ++p2;
            }

            if (*p2 == '\0') {
                break;
            }
            *p2 = '\0';
            std::string name(p1, p2);
            *p2 = '}';
            fill(p, paramMap[name].c_str());
            p1 = ++p2;
        }
        *p = '\0';
        logItem.adInfo.landingUrl = std::string(result, p);
        replace(logItem.adInfo.landingUrl, "%7badvid%7d", paramMap["advid"]);
        replace(logItem.adInfo.landingUrl, "%7Badvid%7D", paramMap["advid"]);
        replace(logItem.adInfo.landingUrl, "%7bidfa%7d", paramMap["idfa"]);
        replace(logItem.adInfo.landingUrl, "%7Bidfa%7D", paramMap["idfa"]);
        replace(logItem.adInfo.landingUrl, "%7bsource_id%7d", paramMap["source_id"]);
        replace(logItem.adInfo.landingUrl, "%7Bsource_id%7D", paramMap["source_id"]);
    }

    void HandleClickQueryTask::customLogic(ParamMap & paramMap, protocol::log::LogItem & log,
                                           adservice::utility::HttpResponse & resp)
    {
        if (!log.adInfo.landingUrl.empty()) {
            // 记录访问信息到缓存
            recordSource(paramMap, log);

            handleLandingUrl(log, paramMap);
            resp.status(302, "OK");
            resp.set_header("Location", log.adInfo.landingUrl);
            resp.set_body("m");

            int64_t orderId = 0;
            try {
                orderId = std::stoll(paramMap[URL_ORDER_ID]);

                MT::common::ASKey key(globalConfig.aerospikeConfig.nameSpace.c_str(), "order-counter", orderId);
                MT::common::ASOperation op(1);
                op.addIncr("c", (int64_t)1);

                aerospikeClient.operate(key, op);
            } catch (MT::common::AerospikeExcption & e) {
                LOG_ERROR << "记录点击失败，订单ID：" << orderId << "，" << e.what() << "，code:" << e.error().code
                          << e.error().message << "，调用堆栈：" << std::endl
                          << e.trace();
            } catch (std::exception & e) {
                LOG_ERROR << "记录点击失败，订单ID无效：" << paramMap[URL_ORDER_ID] << ",query:" << data;
            }
        } else {
            resp.status(400, "Error,empty landing url");
        }
    }

    void HandleClickQueryTask::onError(std::exception & e, adservice::utility::HttpResponse & resp)
    {
        LOG_ERROR << "error occured in HandleClickQueryTask:" << e.what();
    }

} // namespace corelogic
} // namespace adservice
