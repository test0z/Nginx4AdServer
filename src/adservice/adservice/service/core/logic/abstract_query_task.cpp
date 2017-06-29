//
// Created by guoze.lin on 16/4/29.
//

#include "abstract_query_task.h"
#include "common/spinlock.h"
#include "core/core_cm_manager.h"
#include "logging.h"
#include "protocol/360/360max_price.h"
#include "protocol/baidu/baidu_price.h"
#include "protocol/guangyin/guangyin_price.h"
#include "protocol/kupai/kupai_price_decode.h"
#include "protocol/netease/nex_price.h"
#include "protocol/sohu/sohu_price.h"
#include "protocol/tanx/tanx_price.h"
#include "protocol/tencent_gdt/tencent_gdt_price.h"
#include "protocol/youku/youku_price.h"
#include "utility/mttytime.h"
#include <mtty/mtuser.h>
#include <mtty/trafficcontrollproxy.h>

namespace adservice {
namespace corelogic {

    using namespace utility;

    static int threadSeqId = 0;
    struct spinlock slock = { 0 };
    thread_local TaskThreadLocal * threadData = new TaskThreadLocal;

    void TaskThreadLocal::updateSeqId()
    {
        if (seqId >= 0) {
            return;
        }
        spinlock_lock(&slock);
        seqId = threadSeqId++;
        spinlock_unlock(&slock);
        LOG_INFO << "seqId:" << seqId;
    }

    namespace {
        /**
         * 对ADX成交价进行解密
         */
        int decodeAdxExchangePrice(int adx, const std::string & input)
        {
            if (input.empty())
                return 0;
            switch (adx) {
            case ADX_TANX:
            case ADX_TANX_MOBILE:
                return tanx_price_decode(input);
            case ADX_YOUKU:
            case ADX_YOUKU_MOBILE:
                return youku_price_decode(input);
            case ADX_BAIDU:
            case ADX_BAIDU_MOBILE:
                return baidu_price_decode(input);
            case ADX_GUANGYIN:
            case ADX_GUANGYIN_MOBILE:
                return guangyin_price_decode(input);
            case ADX_TENCENT_GDT:
            case ADX_GDT_MOBILE:
                return gdt_price_decode(input);
            case ADX_NETEASE_MOBILE:
            case ADX_YIDIAN:
                return std::stoi(input);
            case ADX_SOHU_PC:
            case ADX_SOHU_MOBILE:
                return sohu_price_decode(input);
            case ADX_INMOBI:
                try {
                    return int(std::stof(input) * 1.06 * 100);
                } catch (...) {
                    LOG_ERROR << "adx inmobi price error,input:" << input;
                    return 0;
                }
            case ADX_LIEBAO_MOBILE:
                try {
                    return std::stoi(input);
                } catch (...) {
                    LOG_ERROR << "adx liebao price error,input:" << input;
                    return 0;
                }
            case ADX_NEX_PC:
            case ADX_NEX_MOBILE:
                return nex_price_decode(input);
            case ADX_KUPAI_MOBILE:
                return kupai_price_decode(input);
            case ADX_360_MAX_PC:
            case ADX_360_MAX_MOBILE:
                return max360_price_decode(input) / 10000;
            default:
                return 0;
            }
        }

        void calcPrice(int adx, bool isDeal, int decodePrice, int offerPrice, double feeRate, int & cost,
                       int & bidPrice, protocol::log::LogPhaseType logPhase, int priceType = PRICETYPE_RRTB_CPM)
        {
            if (isDeal || adx == ADX_NETEASE_MOBILE || adx == ADX_YIDIAN) {
                cost = decodePrice;
                bidPrice = offerPrice == 0 ? cost : offerPrice;
            } else {
                cost = decodePrice;
                bidPrice = std::ceil(decodePrice * feeRate);
            }
            if (priceType == PRICETYPE_RRTB_CPC || priceType == PRICETYPE_RCPC) {
                if (logPhase != protocol::log::LogPhaseType::CLICK) {
                    bidPrice = 0;
                } else {
                    cost = 0;
                    bidPrice = offerPrice
                               * 1000; // 华哥那边对cpc计费按照cpm公式来算，但cpc是按个数结算的，为了兼容要乘以1000
                }
            }
        }

        int decodeOfferPrice(const std::string & input, bool useAes = false)
        {
            try {
                if (!useAes) {
                    int output = std::stoi(input);
                    if (output < 0) {
                        return -output;
                    } else {
                        return output;
                    }
                } else {
                    std::string result;
                    aes_ecbdecode((const unsigned char *)OFFERPRICE_ENCODE_KEY, input, result);
                    return std::stoi(result);
                }
            } catch (std::exception & e) {
                LOG_ERROR << "in decodeOfferPrice," << e.what() << "," << input;
            }
            return 0;
        }

        std::string countedUserIp(int adxId, const std::string & ip, ParamMap & paramMap)
        {
            switch (adxId) {
            case ADX_TENCENT_GDT:
            case ADX_GDT_MOBILE:
                return paramMap.find(URL_IP) != paramMap.end() ? paramMap[URL_IP] : ip;
            default:
                return ip;
            }
        }

        /**
         * 将请求参数按需求转换为Log对象
         * 鉴于不同的模块的参数有所差异,所以为了提高parse的速度,可以做parser分发,这是后续优化的点
         */
        void parseObjectToLogItem(ParamMap & paramMap, protocol::log::LogItem & log, const char * allQuery = NULL)
        { //考虑将临时对象作为static const对象来处理
            char buf[1024];
            std::string output;
            ParamMap::iterator iter;
            try {
                if ((iter = paramMap.find(URL_LANDING_URL)) != paramMap.end()) { //落地页
                    std::string & url = iter->second;                            // paramMap[URL_LANDING_URL];
                    urlDecode_f(url, output, buf);
                    log.adInfo.landingUrl = buf;
                }
                if ((iter = paramMap.find(URL_REFERER)) != paramMap.end()) { // ref
                    std::string & f = iter->second;                          // paramMap[URL_REFERER];
                    urlDecode_f(f, output, buf);
                    log.referer = f;
                }
                if ((iter = paramMap.find(URL_ADPLACE_ID)) != paramMap.end()) { // ADX广告位Id
                    std::string & s = iter->second;                             // paramMap[URL_ADPLACE_ID];
                    log.adInfo.pid = "0";
                    log.adInfo.adxpid = s;
                }
                if ((iter = paramMap.find(URL_MTTYADPLACE_ID)) != paramMap.end()) { //广告位Id
                    std::string & s = iter->second;                                 // paramMap[URL_ADPLACE_ID];
                    log.adInfo.pid = s;
                }
                if ((iter = paramMap.find(URL_EXPOSE_ID)) != paramMap.end()) { //曝光Id
                    std::string & r = iter->second;                            // paramMap[URL_EXPOSE_ID];
                    log.adInfo.imp_id = r;
                }
                if ((iter = paramMap.find(URL_SITE_ID)) != paramMap.end()) { //网站id
                    std::string & mid = iter->second;
                    log.adInfo.mid = URLParamMap::stringToInt(mid);
                }
                if ((iter = paramMap.find(URL_ADOWNER_ID)) != paramMap.end()) { //广告主Id
                    std::string & d = iter->second;                             // paramMap[URL_ADOWNER_ID];
                    log.adInfo.advId = URLParamMap::stringToInt(d);
                    log.adInfo.cpid = log.adInfo.advId;
                }
                if ((iter = paramMap.find(URL_EXEC_ID)) != paramMap.end()) { // 投放单元Id
                    std::string & e = iter->second;                          // paramMap[URL_EXEC_ID];
                    log.adInfo.sid = URLParamMap::stringToInt(e);
                }
                if ((iter = paramMap.find(URL_CREATIVE_ID)) != paramMap.end()) { // 创意Id
                    std::string & c = iter->second;                              // paramMap[URL_CREATIVE_ID];
                    log.adInfo.bannerId = URLParamMap::stringToInt(c);
                }
                if ((iter = paramMap.find(URL_ADX_ID)) != paramMap.end()) { // 平台Id,adxId
                    std::string & x = iter->second;                         // paramMap[URL_ADX_ID];
                    log.adInfo.adxid = URLParamMap::stringToInt(x);
                }
                if ((iter = paramMap.find(URL_CLICK_ID)) != paramMap.end()) { // clickId
                    std::string & h = iter->second;                           // paramMap[URL_CLICK_ID];
                    log.adInfo.clickId = h;
                }
                if ((iter = paramMap.find(URL_AREA_ID)) != paramMap.end()) { // areaId
                    std::string & a = iter->second;                          // paramMap[URL_AREA_ID];
                    log.adInfo.areaId = a;
                    int country, province, city;
                    extractAreaInfo(a.c_str(), country, province, city);
                    log.geoInfo.country = country;
                    log.geoInfo.province = province;
                    log.geoInfo.city = city;
                }
                if ((iter = paramMap.find(URL_CLICK_X)) != paramMap.end()) { //点击坐标x
                    std::string & sx = iter->second;                         // paramMap[URL_CLICK_X];
                    log.clickx = URLParamMap::stringToInt(sx);
                }
                if ((iter = paramMap.find(URL_CLICK_Y)) != paramMap.end()) { //点击坐标y
                    std::string & sy = iter->second;                         // paramMap[URL_CLICK_Y];
                    log.clicky = URLParamMap::stringToInt(sy);
                }
                if ((iter = paramMap.find(URL_PRICE_TYPE)) != paramMap.end()) { //出价类型
                    std::string & pricetype = iter->second;
                    log.adInfo.priceType = URLParamMap::stringToInt(pricetype);
                } else {
                    log.adInfo.priceType = 0;
                }
                if ((iter = paramMap.find(URL_ORDER_ID)) != paramMap.end()) { //订单id
                    std::string & orderId = iter->second;
                    log.adInfo.orderId = URLParamMap::stringToInt(orderId);
                }
                if ((iter = paramMap.find(URL_FEE_RATE_STRS)) != paramMap.end()) {
                    std::string feeRateDetail = iter->second;
                    log.adInfo.feeRateDetail = feeRateDetail;
                }
                double feeRate = 1.0;
                if ((iter = paramMap.find(URL_FEE_RATE)) != paramMap.end()) {
                    feeRate = stringtool::safeconvert(stringtool::stod, iter->second);
                    feeRate = feeRate <= 1.0 ? 1.0 : feeRate;
                }
                int offerPrice
                    = paramMap.find(URL_BID_PRICE) != paramMap.end() ? decodeOfferPrice(paramMap[URL_BID_PRICE]) : 0;
                if ((iter = paramMap.find(URL_EXCHANGE_PRICE)) != paramMap.end()) { //成交价格
                    std::string & price = iter->second;                             // paramMap[URL_EXCHANGE_PRICE];
                    int decodePrice = decodeAdxExchangePrice(log.adInfo.adxid, price);
                    bool isDeal = paramMap.find(URL_DEAL_ID) != paramMap.end() && !paramMap[URL_DEAL_ID].empty();
                    calcPrice(log.adInfo.adxid, isDeal, decodePrice, offerPrice, feeRate, log.adInfo.cost,
                              log.adInfo.bidPrice, log.logType, log.adInfo.priceType);
                } else {
                    calcPrice(log.adInfo.adxid, false, offerPrice, offerPrice, feeRate, log.adInfo.cost,
                              log.adInfo.bidPrice, log.logType, log.adInfo.priceType);
                }
                if ((iter = paramMap.find(URL_PRODUCTPACKAGE_ID)) != paramMap.end()) { //产品包id
                    std::string & ppid = iter->second;
                    log.adInfo.ppid = URLParamMap::stringToInt(ppid);
                }
                if ((iter = paramMap.find(URL_DEVICE_IDFA)) != paramMap.end()) { // idfa
                    std::string & deviceId = iter->second;
                    log.device.deviceID += std::string("idfa:") + deviceId + ",";
                }
                if ((iter = paramMap.find(URL_DEVICE_IMEI)) != paramMap.end()) { // imei
                    std::string & deviceId = iter->second;
                    log.device.deviceID += std::string("imei:") + deviceId + ",";
                }
                if ((iter = paramMap.find(URL_DEVICE_ANDOROIDID)) != paramMap.end()) { // androidid
                    std::string & deviceId = iter->second;
                    log.device.deviceID += std::string("androidid:") + deviceId + ",";
                }
                if ((iter = paramMap.find(URL_DEVICE_MAC)) != paramMap.end()) { // mac
                    std::string & deviceId = iter->second;
                    log.device.deviceID += std::string("mac:") + deviceId + ",";
                }
            } catch (std::exception & e) {
                log.reqStatus = 500;
                LOG_ERROR << "error:" << e.what() << ",when processing query " << allQuery;
            }
        }

        void plantNewCookie(const std::string & cookieUid, adservice::utility::HttpResponse & resp)
        {
            std::string cookieString = COOKIES_MTTY_ID "=";
            cookieString += cookieUid;
            cookieString += ";Domain=." COOKIES_MTTY_DOMAIN ";Max-Age=2617488000;";
            resp.set("Set-Cookie", cookieString);
        }

        bool isBussinessLogImportant(protocol::log::LogPhaseType & phaseType)
        {
            if (phaseType == protocol::log::CLICK || phaseType == protocol::log::SHOW
                || phaseType == protocol::log::TRACK) { //点击，曝光，跟踪都是重要日志
                return true;
            } else {
                return false;
            }
        }
    }

    void AbstractQueryTask::updateThreadData()
    {
    }

    void AbstractQueryTask::filterParamMapSafe(ParamMap & paramMap)
    {
        for (ParamMap::iterator iter = paramMap.begin(); iter != paramMap.end(); iter++) {
            stringSafeInput(iter->second, URL_LONG_INPUT_PARAMETER);
        }
    }

    /**
     * 同时取u参数和cookie,进行兼容校验,返回一个可供追溯的有效用户id(明文)
     * u参数与cookie校正方案：
     *    假定cookie与u参数都合法,但不相同。u参数的追溯时间与cookie的追溯时间作比较,cookie追溯时间更早，则使用cookie。
     *    假定cookie不合法或不存在，但u合法，使用u参数。
     *    假定cookie合法，但u不合法或不存在，使用cookie。
     *    假定cookie与u均不合法或不存在,生成新cookie。
     * @brief AbstractQueryTask::userCookieLogic
     * @param paramMap
     * @param resp
     * @return
     */
    std::string AbstractQueryTask::userCookieLogic(ParamMap & paramMap, adservice::utility::HttpResponse & resp)
    {
        std::string cookieEncUid = extractCookiesParam(COOKIES_MTTY_ID, userCookies);
        const std::string & serverEncUid = paramMap.find(URL_MTTY_UID) != paramMap.end() ? paramMap[URL_MTTY_UID] : "";
        MT::User::UserID clientUid(cookieEncUid, true);
        MT::User::UserID serverUid(serverEncUid, true);
        if (clientUid.isValid() && serverUid.isValid()) { //客户端id和服务端id同时存在且合法
                                                          //            int64_t clientUserTime = clientUid.time();
                                                          //            int64_t serverUserTime = serverUid.time();
                                                          //            if (clientUserTime < serverUserTime) {
                                                          //                 return clientUid.text();
                                                          //            }
            if (cookieEncUid != serverEncUid) {
                plantNewCookie(serverUid.cipher(), resp);
            }
            return serverUid.text();
        } else if (!clientUid.isValid() && serverUid.isValid()) { //客户端id不合法,服务端id合法
            plantNewCookie(serverUid.cipher(), resp);
            return serverUid.text();
        } else if (clientUid.isValid() && !serverUid.isValid()) { //客户端id合法，服务端id不合法
            return clientUid.text();
        } else { //客户端id不合法，服务端id也不合法
            // MT::User::UserID newUid(int64_t(utility::rng::randomInt() & 0x0000FFFF));
            auto idSeq = CookieMappingManager::IdSeq();
            MT::User::UserID newUid(idSeq.id(), idSeq.time());
            plantNewCookie(newUid.cipher(), resp);
            return newUid.text();
        }
    }

    void AbstractQueryTask::commonLogic(ParamMap & paramMap, protocol::log::LogItem & log,
                                        adservice::utility::HttpResponse & resp)
    {
        log.userAgent = userAgent;
        log.logType = currentPhase();
        log.reqMethod = reqMethod();
        log.reqStatus = expectedReqStatus();
        log.timeStamp = utility::time::getCurrentTimeStampUtc();
        log.referer = referer;
        if (!isPost) { //对于非POST方法传送的Query
            getParamv2(paramMap, data.c_str());
            ParamMap::iterator iter;
            if ((iter = paramMap.find(URLHelper::ENCODE_HOLDER)) != paramMap.end()) {
                URLHelper urlParam("", "", "", "", data, true);
                auto & newParamMap = urlParam.getParamMap();
                if (newParamMap.find(URL_EXCHANGE_PRICE) == newParamMap.end() && log.logType == protocol::log::SHOW) {
                    newParamMap[URL_EXCHANGE_PRICE] = paramMap[URL_EXCHANGE_PRICE];
                }
                paramMap = newParamMap;
            }
            filterParamMapSafe(paramMap);
            parseObjectToLogItem(paramMap, log, data.c_str());
            log.userId = userCookieLogic(paramMap, resp);
            log.ipInfo.proxy = countedUserIp(log.adInfo.adxid, userIp, paramMap);
        } else { //对于POST方法传送过来的Query
            getPostParam(paramMap);
            log.ipInfo.proxy = userIp;
        }
        resp.set_content_header("text/html");
#ifdef USE_ENCODING_GZIP
        resp.set_header("Content-Encoding", "gzip");
#endif
    }

    void AbstractQueryTask::doLog(protocol::log::LogItem & log)
    {
        std::shared_ptr<std::string> logString = std::make_shared<std::string>();
        writeAvroObject(log, *(logString.get()));
#if defined(USE_ALIYUN_LOG) && defined(UNIT_TEST)
        checkAliEscapeSafe(log, *(logString.get()));
#endif
        // 将日志对象推送到日志队列
        bool isImportant = isBussinessLogImportant(log.logType);
        if (serviceLogger.use_count() > 0) {
            serviceLogger->push(logString, isImportant);
        } else {
            std::string loggerName = usedLoggerName();
            std::string logConfigKey = usedLoggerConfig();
            adservice::log::LogPusherPtr logPusher = adservice::log::LogPusher::getLogger(loggerName, logConfigKey);
            if (logPusher.use_count() > 0) {
                logPusher->push(logString, isImportant);
            }
        }
        if (log.logType == protocol::log::SHOW) {
            auto trafficControl = MT::common::traffic::TrafficControllProxy::getInstance();
            if (log.adInfo.priceType == PRICETYPE_RRTB_CPM || log.adInfo.priceType == PRICETYPE_RTB) {
                trafficControl->recordCPMShowAsync(log.adInfo.sid, log.adInfo.advId, log.adInfo.bidPrice / 1000.0);
            } else if (log.adInfo.priceType == PRICETYPE_RCPC || log.adInfo.priceType == PRICETYPE_RRTB_CPC) {
                trafficControl->recordCPCShowAsync(log.adInfo.sid);
            }
        } else if (log.logType == protocol::log::CLICK) {
            auto trafficControl = MT::common::traffic::TrafficControllProxy::getInstance();
            if (log.adInfo.priceType == PRICETYPE_RRTB_CPM || log.adInfo.priceType == PRICETYPE_RTB) {
                trafficControl->recordCPMClickAsync(log.adInfo.sid);
            } else if (log.adInfo.priceType == PRICETYPE_RCPC || log.adInfo.priceType == PRICETYPE_RRTB_CPC) {
                trafficControl->recordCPCClickAsync(log.adInfo.sid, log.adInfo.advId,
                                                    log.adInfo.bidPrice / 1000.0); //与历史兼容
            }
        }
        if (log.logType == protocol::log::SHOW && (log.adInfo.adxid == 4 || log.adInfo.adxid > 99)) { //脏数据来源检测
            LOG_ERROR << "captured suspicious input:" << data;
        }
    }

    void AbstractQueryTask::operator()()
    {
        try {
            ParamMap paramMap;
            protocol::log::LogItem log;
            resp.status(expectedReqStatus());
            commonLogic(paramMap, log, resp);
            customLogic(paramMap, log, resp);
            if (needLog) {
                doLog(log);
            }
        } catch (std::exception & e) {
            resp.status(500, "error");
            resp.set_content_header("text/html");
            onError(e, resp);
        }
    }
}
}
