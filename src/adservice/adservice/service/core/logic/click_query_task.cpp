//
// Created by guoze.lin on 16/4/29.
//

#include "click_query_task.h"

#include <iomanip>

#include <hiredis/async.h>

#include <aerospike/aerospike_key.h>
#include <aerospike/as_operations.h>
#include <aerospike/as_record.h>

#include "core/model/source_record.h"
#include "logging.h"
#include "utility/aero_spike.h"
#include "utility/url.h"

extern std::shared_ptr<redisAsyncContext> redisConnection;

namespace adservice {
namespace corelogic {

    namespace {

        void fill(char *& p, const char * src)
        {
            while (*src != '\0') {
                *p++ = *src++;
            }
        }

        std::string generateSourceId()
        {
            if (!utility::AeroSpike::instance && !utility::AeroSpike::instance.connect()) {
                auto & error = utility::AeroSpike::instance.error();
                LOG_ERROR << "connect error, code:" << error.code << ", msg:" << error.message;
                return "";
            }

            aerospike * conn = utility::AeroSpike::instance.connection();

            as_error error;

            as_key key;
            as_key_init(&key, utility::AeroSpike::instance.nameSpace().c_str(), "source_id_seq", "last_source_id");

            /* 检查是否存在souce id生成序列 */
            as_record * rec = nullptr;
            if (utility::AeroSpike::noTimeOutExec(&aerospike_key_get, conn, &error, nullptr, &key, &rec) != AEROSPIKE_OK
                || !rec) {
                // 不存在则创建
                rec = as_record_new(1);
                as_record_set_int64(rec, "val", 0);

                if (utility::AeroSpike::noTimeOutExec(&aerospike_key_put, conn, &error, nullptr, &key, rec)
                    != AEROSPIKE_OK) {
                    LOG_ERROR << "put error, code:" << error.code << ", msg:" << error.message;
                    as_record_destroy(rec);
                    as_key_destroy(&key);
                    return "";
                }

                as_record_destroy(rec);
            }

            /* 取id并自增，具有原子性 */
            as_operations ops;
            as_operations_init(&ops, 2);
            as_operations_add_read(&ops, "val");
            as_operations_add_incr(&ops, "val", 1);

            rec = as_record_new(1);

            if (utility::AeroSpike::noTimeOutExec(&aerospike_key_operate, conn, &error, nullptr, &key, &ops, &rec)
                != AEROSPIKE_OK) {
                LOG_ERROR << "operate error, code:" << error.code << ", msg:" << error.message;
                as_record_destroy(rec);
                as_key_destroy(&key);
                return "";
            }

            /* 转化为16进制字符串 */
            int64_t val = as_record_get_int64(rec, "val", -1);
            std::string res;
            if (val != -1) {
                std::stringstream ss;
                ss << std::setw(12) << std::setfill('0') << std::setbase(16) << val;
                ss >> res;
            } else {
                LOG_ERROR << "生成source_id失败";
            }

            as_record_destroy(rec);
            as_key_destroy(&key);

            return res;
        }

        void recordSource(ParamMap & paramMap, protocol::log::LogItem & log)
        {
            std::string userId = log.userId, ownerId = paramMap[URL_ADOWNER_ID];
            if (userId.empty() || ownerId.empty()) {
                LOG_ERROR << "用户id：“" << userId << "”或广告主id：“" << ownerId << "”为空！";
                return;
            }

            if (!utility::AeroSpike::instance && !utility::AeroSpike::instance.connect()) {
                auto & error = utility::AeroSpike::instance.error();
                LOG_ERROR << "connect error, code:" << error.code << ", msg:" << error.message;
                return;
            }

            aerospike * conn = utility::AeroSpike::instance.connection();

            /* 记录访问信息到缓存 */
            // 生成source id
            std::string sourceId = generateSourceId();
            if (sourceId.empty()) {
                LOG_ERROR << "生成的sourceid为空。";
                return;
            }

            paramMap["source_id"] = sourceId;

            // 填充数据
            core::model::SourceRecord sourceRecord(paramMap, log);
            as_record * rec = sourceRecord.record();

            // 生成key
            as_key key;
            as_key_init(&key, utility::AeroSpike::instance.nameSpace().c_str(), "source_id", sourceId.c_str());

            // 放入缓存
            as_error error;

            if (utility::AeroSpike::noTimeOutExec(&aerospike_key_put, conn, &error, nullptr, &key, rec)
                != AEROSPIKE_OK) {
                LOG_ERROR << "put error, code:" << error.code << ", msg:" << error.message;
                as_key_destroy(&key);
                return;
            }

            as_key_destroy(&key);

            /* 记录索引信息，方便t模块根据用户ID和广告主ID查询到source id */
            as_record rec2;
            as_record_inita(&rec2, 1);
            as_record_set_str(&rec2, "source_id", sourceId.c_str());

            as_key_init(&key, utility::AeroSpike::instance.nameSpace().c_str(), "source_id_index",
                        (userId + ownerId).c_str());

            if (utility::AeroSpike::noTimeOutExec(&aerospike_key_put, conn, &error, nullptr, &key, &rec2)
                != AEROSPIKE_OK) {
                LOG_ERROR << "put index error, code:" << error.code << ", msg:" << error.message;
            }

            as_key_destroy(&key);
            as_record_destroy(&rec2);
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

			std::string orderId;

			std::string command = "INCR order-counter:" + orderId + ":c";
			redisAsyncCommand(redisConnection.get(), nullptr, nullptr, command.c_str());
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
