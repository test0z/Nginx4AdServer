#include "trace_task.h"

#include <curl/curl.h>

#include <mtty/aerospike.h>
#include <mtty/requestcounter.h>

#include "core/config_types.h"
#include "core/model/source_id.h"

extern MT::common::Aerospike aerospikeClient;
extern GlobalConfig globalConfig;
extern MT::common::RequestCounter requestCounter;

namespace adservice {
namespace corelogic {

    namespace {

        using namespace adservice::utility;
        using namespace adservice::utility::stringtool;

        const int TRACE_ID_ARRIVE = 5;

        // 版本号
        const std::string URL_VERSION = "v";
        // 设备类型：1 PC端  2 移动端
        const std::string URL_DEVICE_TYPE = "k";
        /*
         * 请求类型：
         * 6 记录一次PV请求
         * 7 记录一次注册请求
         * 8 记录一次订单请求
         * 1001 记录一次自定义行为
         * 12 记录一次下载请求<mobile>
         * 13 记录一次安装请求<mobile>
         * 14 记录一次激活请求<mobile>
         */
        const std::string URL_REQUEST_TYPE = "y";
        // source_id
        const std::string URL_SOURCE_ID = "g";
        // 用户ID/订单ID/设备ID
        const std::string URL_USER_OR_ORDER_ID = "t1";
        // 用户名/商品名／设备ID类型 IDFA/IMEI/AndroidID
        const std::string URL_USER_OR_PRODUCT_NAME = "t2";
        // 商品ID
        const std::string URL_PRODUCT_ID = "t3";
        // 商品价格
        const std::string URL_PRODUCT_PRICE = "t4";
        // 商品数量
        const std::string URL_PRODUCT_QUANTITY = "t5";
        // 订单总金额
        const std::string URL_ORDER_PRICE = "t6";
        // 商品图片地址
        const std::string URL_PRODUCT_IMAGE_URL = "t7";
        // 商品URL地址
        const std::string URL_PRODUCT_URL = "t8";

        const std::string URL_TAG9 = "t9";

        const std::string URL_TAG10 = "t10";
        // 时间戳
        const std::string URL_TIME = "t";

        // 协议，0为http，1为https
        const std::string URL_PROTOCOL = "i";

        const std::string deviceType[] = { "idfa", "imei", "androidId", "mac" };

        void fillLog(protocol::log::LogItem & log,
                     ParamMap & paramMap,
                     const core::model::SourceRecord & sourceRecord,
                     const std::string & version,
                     const std::string & device,
                     const std::string & sourceId)
        {
            log.traceInfo.version = version;
            log.traceInfo.deviceType = device;
            log.traceInfo.sourceid = sourceId;
            log.traceInfo.tag1 = paramMap[URL_USER_OR_ORDER_ID];
            log.traceInfo.tag2 = paramMap[URL_USER_OR_PRODUCT_NAME];
            log.traceInfo.tag3 = paramMap[URL_PRODUCT_ID];
            log.traceInfo.tag4 = paramMap[URL_PRODUCT_PRICE];
            log.traceInfo.tag5 = paramMap[URL_PRODUCT_QUANTITY];
            log.traceInfo.tag6 = paramMap[URL_ORDER_PRICE];
            log.traceInfo.tag7 = paramMap[URL_PRODUCT_IMAGE_URL];
            log.traceInfo.tag8 = paramMap[URL_PRODUCT_URL];
            log.traceInfo.tag9 = paramMap[URL_TAG9];
            log.traceInfo.tag10 = paramMap[URL_TAG10];
            if (sourceRecord.time() != 0) {
                try {
                    log.adInfo.advId = safeconvert(stoi, sourceRecord.advId());
                    log.adInfo.adxpid = sourceRecord.adxPid();
                    log.adInfo.pid = sourceRecord.pid();
                    log.adInfo.areaId = sourceRecord.geoId();
                    log.adInfo.bannerId = safeconvert(stoi, sourceRecord.createId());
                    log.adInfo.sid = safeconvert(stoi, sourceRecord.sid());
                    log.adInfo.bidPrice = safeconvert(stoi, sourceRecord.bidPrice());
                    log.adInfo.adxid = safeconvert(stoi, sourceRecord.adxId());
                    log.adInfo.ppid = safeconvert(stoi, sourceRecord.ppId());
                    log.adInfo.orderId = safeconvert(stoi, sourceRecord.oId());
                    log.adInfo.priceType = safeconvert(stoi, sourceRecord.priceType());
                    log.adInfo.mid = safeconvert(stoi, sourceRecord.mId());
                    log.adInfo.imp_id = sourceRecord.impId();
                    url::extractAreaInfo(
                        log.adInfo.areaId.c_str(), log.geoInfo.country, log.geoInfo.province, log.geoInfo.city);
                } catch (std::exception & e) {
                }
            }
        }

        std::string aliLog(const protocol::log::LogItem & log,
                           const core::model::SourceRecord & sourceRecord,
                           const std::string & ownerId,
                           const std::string & sourceId,
                           const std::string & requestTypeStr)
        {
            std::string result
                = "http://mtty.cn-beijing-vpc.log.aliyuncs.com/logstores/mt-log/track.gif?APIVersion=0.6.0&t="
                  + std::to_string(log.timeStamp);

            if (!log.traceInfo.version.empty()) {
                result += "&v=" + log.traceInfo.version;
            }
            if (!ownerId.empty()) {
                result += "&d=" + ownerId;
            }
            if (!log.traceInfo.deviceType.empty()) {
                result += "&k=" + log.traceInfo.deviceType;
            }
            if (log.traceId == TRACE_ID_ARRIVE) {
                result += "&y=" + std::to_string(log.traceId);
            } else if (!requestTypeStr.empty()) {
                result += "&y=" + requestTypeStr;
            }
            if (!sourceId.empty()) {
                result += "&g=" + sourceId;
            }
            if (!log.traceInfo.tag1.empty()) {
                result += "&t1=" + log.traceInfo.tag1;
            }
            if (!log.traceInfo.tag2.empty()) {
                result += "&t2=" + log.traceInfo.tag2;
            }
            if (!log.traceInfo.tag3.empty()) {
                result += "&t3=" + log.traceInfo.tag3;
            }
            if (!log.traceInfo.tag4.empty()) {
                result += "&t4=" + log.traceInfo.tag4;
            }
            if (!log.traceInfo.tag5.empty()) {
                result += "&t5=" + log.traceInfo.tag5;
            }
            if (!log.traceInfo.tag6.empty()) {
                result += "&t6=" + log.traceInfo.tag6;
            }
            if (!log.traceInfo.tag7.empty()) {
                result += "&t7=" + log.traceInfo.tag7;
            }
            if (!log.traceInfo.tag8.empty()) {
                result += "&t8=" + log.traceInfo.tag8;
            }
            if (!log.traceInfo.tag9.empty()) {
                result += "&t9=" + log.traceInfo.tag9;
            }
            if (!log.traceInfo.tag10.empty()) {
                result += "&t10=" + log.traceInfo.tag10;
            }
            if (!sourceRecord.geoId().empty()) {
                result += "&a=" + sourceRecord.geoId();
            }
            if (!sourceRecord.bidPrice().empty()) {
                result += "&b=" + sourceRecord.bidPrice();
            }
            if (!sourceRecord.createId().empty()) {
                result += "&c=" + sourceRecord.createId();
            }
            if (!sourceRecord.sid().empty()) {
                result += "&e=" + sourceRecord.sid();
            }
            if (!sourceRecord.requestId().empty()) {
                result += "&r=" + sourceRecord.requestId();
            }
            if (!sourceRecord.adxPid().empty()) {
                result += "&s=" + sourceRecord.adxPid();
            }
            if (!sourceRecord.adxId().empty()) {
                result += "&x=" + sourceRecord.adxId();
            }
            if (!sourceRecord.pid().empty()) {
                result += "&o=" + sourceRecord.pid();
            }
            if (!sourceRecord.ppId().empty()) {
                result += "&ep=" + sourceRecord.ppId();
            }
            if (!sourceRecord.oId().empty()) {
                result += "&od=" + sourceRecord.oId();
            }
            if (!sourceRecord.priceType().empty()) {
                result += "&pt=" + sourceRecord.priceType();
            }
            if (!log.userId.empty()) {
                result += "&u=" + log.userId;
            }
            LOG_DEBUG << "阿里云日志记录URL：" << result;

            return result;
        }
        static size_t writeData(void * ptr, size_t size, size_t nmemb, void * userdata)
        {
            std::string * result = (std::string *)userdata;
            result->append((char *)ptr, size * nmemb);

            return (size * nmemb);
        }

        std::string httpGet(const std::string & url)
        {
            std::string content;

            auto * curl = curl_easy_init();
            if (curl != nullptr) {
                curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeData);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);

                curl_easy_perform(curl);
            }

            curl_easy_cleanup(curl);
            return content;
        }

    } // anonymous namespace

    protocol::log::LogPhaseType HandleTraceTask::currentPhase()
    {
        return protocol::log::LogPhaseType::TRACK;
    }

    int HandleTraceTask::expectedReqStatus()
    {
        return 302;
    }

    void HandleTraceTask::customLogic(ParamMap & paramMap,
                                      protocol::log::LogItem & log,
                                      adservice::utility::HttpResponse & resp)
    {
        // needLog = false;
        std::string userId = log.userId, ownerId = paramMap[URL_ADOWNER_ID],
                    requestTypeStr = paramMap[URL_REQUEST_TYPE], version = paramMap[URL_VERSION],
                    device = paramMap[URL_DEVICE_TYPE];

        std::string sourceId = paramMap["g"];

        if (sourceId.empty()) {

            /* 根据用户id和广告主id获取source_id */
            core::model::SourceId sourceIdEntity;
            MT::common::ASKey key(globalConfig.aerospikeConfig.funcNamespace(AS_NAMESPACE_TRACE),
                                  "source_id_index",
                                  (userId + ownerId).c_str());
            try {
                aerospikeClient.get(key, sourceIdEntity);
                sourceId = sourceIdEntity.get();
            } catch (MT::common::AerospikeExcption & e) {
                LOG_ERROR << "获取source_id失败！userid:" << userId << ", ownerId:" << ownerId
                          << "，code:" << e.error().code << ", error:" << e.error().message;
            }
        }

        log.traceId = adservice::utility::stringtool::safeconvert(adservice::utility::stringtool::stoi, requestTypeStr);

        core::model::SourceRecord sourceRecord;
        if (!sourceId.empty()) {
            // 判断是否是一次到达，如果是pv，即y=6，限时10秒，否则请求类型保持原样
            MT::common::ASKey key(
                globalConfig.aerospikeConfig.funcNamespace(AS_NAMESPACE_TRACE), "source_id", sourceId.c_str());
            try {
                aerospikeClient.get(key, sourceRecord);
                if (requestTypeStr == "6" && log.timeStamp - sourceRecord.time() <= 10) {
                    log.traceId = TRACE_ID_ARRIVE;
                }
            } catch (MT::common::AerospikeExcption & e) {
                LOG_ERROR << "获取source record失败！sourceId:" << sourceId << "，code:" << e.error().code
                          << ", error:" << e.error().message;
            }
            const std::string & user_id = sourceRecord.mtUid();
            if (requestTypeStr == "14" && !user_id.empty()) {

                for (int i = 0; i < 4; i++) {
                    CookieMappingManager & cmManager = CookieMappingManager::getInstance();
                    adservice::core::model::MtUserMapping temp
                        = cmManager.getUserDeviceMappingByBin("user_id", user_id, deviceType[i]);
                    if (!temp.outerUserId.empty() || !temp.outerUserOriginId.empty()) {
                        paramMap[URL_USER_OR_ORDER_ID]
                            = (temp.outerUserOriginId.empty() ? temp.outerUserId : temp.outerUserOriginId);
                        paramMap[URL_USER_OR_PRODUCT_NAME] = deviceType[i];
                        break;
                    }
                }
            }

            requestCounter.increaseTraceSuccess();
        } else {
            requestCounter.increaseTraceFailed();
        }

        // 记录TraceInfo日志
        fillLog(log, paramMap, sourceRecord, version, device, sourceId);

        std::string aliLogUrl = aliLog(log, sourceRecord, ownerId, sourceId, requestTypeStr);

        // 跳转至阿里云日志服务
        resp.status(200, "OK");
        resp.set_body(httpGet(aliLogUrl));
    }

    void HandleTraceTask::onError(std::exception & e, adservice::utility::HttpResponse & resp)
    {
        LOG_ERROR << "error occured in HandleTraceTask:" << e.what();
    }

} // namespace corelogic
} // namespace adservice
