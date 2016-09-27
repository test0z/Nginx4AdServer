#include "trace_task.h"

#include <aerospike/aerospike_key.h>

#include "core/model/source_record.h"
#include "utility/aero_spike.h"

namespace adservice {
namespace corelogic {

    namespace {

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

        // sourceid
        bool getSourceId(const std::string & sourceIdIndex, std::string & sourceId)
        {
            if (!utility::AeroSpike::instance && !utility::AeroSpike::instance.connect()) {
                auto & error = utility::AeroSpike::instance.error();
                LOG_ERROR << "connect error, code:" << error.code << ", msg:" << error.message;
                return false;
            }

            aerospike * conn = utility::AeroSpike::instance.connection();

            as_error error;
            as_record * record = nullptr;

            /* 根据用户id和广告主id获取source_id */
            as_key key;
            as_key_init(
                &key, utility::AeroSpike::instance.nameSpace().c_str(), "source_id_index", sourceIdIndex.c_str());
            if (utility::AeroSpike::noTimeOutExec(&aerospike_key_get, conn, &error, nullptr, &key, &record)
                != AEROSPIKE_OK) {
                LOG_ERROR << "get index error, code:" << error.code << ", msg:" << error.message;
                as_key_destroy(&key);
                as_record_destroy(record);
                return false;
            }

            sourceId = as_record_get_str(record, "source_id");

            as_key_destroy(&key);
            as_record_destroy(record);
            return true;
        }

        // sourceidindex
        bool getRecord(const std::string & sourceId, core::model::SourceRecord & sourceRecord)
        {
            if (!utility::AeroSpike::instance && !utility::AeroSpike::instance.connect()) {
                auto & error = utility::AeroSpike::instance.error();
                LOG_ERROR << "connect error, code:" << error.code << ", msg:" << error.message;
                return false;
            }

            aerospike * conn = utility::AeroSpike::instance.connection();

            /* 获取source_id的内容 */
            as_error error;
            as_record * record = nullptr;

            as_key key;
            as_key_init(&key, utility::AeroSpike::instance.nameSpace().c_str(), "source_id", sourceId.c_str());
            if (utility::AeroSpike::noTimeOutExec(&aerospike_key_get, conn, &error, nullptr, &key, &record)
                != AEROSPIKE_OK) {
                LOG_ERROR << "get error, code:" << error.code << ", msg:" << error.message;
                as_key_destroy(&key);
                as_record_destroy(record);
                return false;
            }

            sourceRecord.record(*record);

            return true;
        }

        void fillLog(protocol::log::LogItem & log,
                     ParamMap & paramMap,
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
        }

        std::string aliLog(const std::string & protocol,
                           const protocol::log::LogItem & log,
                           const core::model::SourceRecord & sourceRecord,
                           ParamMap & paramMap,
                           const std::string & ownerId,
                           const std::string & sourceId,
                           const std::string & requestTypeStr)
        {
            std::string result = std::string(protocol == "1" ? "https" : "http")
                                 + "://mtty.cn-beijing.log.aliyuncs.com/logstores/mt-log/track.gif?APIVersion=0.6.0&t="
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
            if (!sourceRecord.pid().empty()) {
                result += "&s=" + sourceRecord.pid();
            }
            if (!sourceRecord.adxId().empty()) {
                result += "&x=" + sourceRecord.adxId();
            }
            if (!log.adInfo.pid.empty()){
                result += "&o=" + log.adInfo.pid;
            }
            if (!log.userId.empty()) {
                result += "&u=" + log.userId;
            }
            LOG_DEBUG<<"阿里云日志记录URL：" <<result;

            return result;
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

    void HandleTraceTask::customLogic(ParamMap & paramMap, protocol::log::LogItem & log, adservice::utility::HttpResponse & resp)
    {
        needLog = false;
        std::string userId = log.userId, ownerId = paramMap[URL_ADOWNER_ID],
                    requestTypeStr = paramMap[URL_REQUEST_TYPE], version = paramMap[URL_VERSION],
                    device = paramMap[URL_DEVICE_TYPE];

        std::string sourceIdIndex = userId + ownerId;

        std::string sourceId = paramMap["g"];
        if (sourceId.empty()) {
            getSourceId(sourceIdIndex, sourceId);
        }

        core::model::SourceRecord sourceRecord;
        if (!sourceId.empty()) {
            // 判断是否是一次到达，如果是pv，即y=6，限时10秒，否则请求类型保持原样
            if (getRecord(sourceId, sourceRecord)) {
                if (requestTypeStr == "6" && log.timeStamp - sourceRecord.time() <= 10) {
                    log.traceId = TRACE_ID_ARRIVE;
                }
            }
        }

        // 记录TraceInfo日志
        fillLog(log, paramMap, version, device, sourceId);

        std::string protocol = paramMap[URL_PROTOCOL];
        std::string aliLogUrl = aliLog(protocol, log, sourceRecord, paramMap, ownerId, sourceId, requestTypeStr);

        // 跳转至阿里云日志服务
        resp.status(302, "OK");
        resp.set_header("Location", aliLogUrl);
    }

    void HandleTraceTask::onError(std::exception & e, adservice::utility::HttpResponse & resp)
    {
        LOG_ERROR << "error occured in HandleTraceTask:" << e.what();
    }

} // namespace corelogic
} // namespace adservice
