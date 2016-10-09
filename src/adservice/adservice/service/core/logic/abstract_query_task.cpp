//
// Created by guoze.lin on 16/4/29.
//

#include "abstract_query_task.h"
#include "common/spinlock.h"
#include "logging.h"
#include "protocol/baidu/baidu_price.h"
#include "protocol/guangyin/guangyin_price.h"
#include "protocol/tanx/tanx_price.h"
#include "protocol/tencent_gdt/tencent_gdt_price.h"
#include "protocol/youku/youku_price.h"
#include "utility/mttytime.h"

namespace adservice {
namespace corelogic {

	using namespace utility;

    static int threadSeqId = 0;
    struct spinlock slock = { 0 };

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
            return baidu_price_decode(input);
        case ADX_GUANGYIN:
            return guangyin_price_decode(input);
        case ADX_TENCENT_GDT:
            return gdt_price_decode(input);
        case ADX_NETEASE_MOBILE:
            return std::stoi(input);
        default:
            return 0;
        }
    }

    void calcPrice(int adx, bool isYoukuDeal, int decodePrice, int offerPrice, int & cost, int & bidPrice,
                   protocol::log::LogPhaseType logPhase, int priceType = PRICETYPE_RRTB_CPM)
    {
        if (((adx == ADX_YOUKU || adx == ADX_YOUKU_MOBILE) && isYoukuDeal) || adx == ADX_NETEASE_MOBILE) {
            cost = decodePrice;
            bidPrice = offerPrice == 0 ? cost : offerPrice;
        } else {
            cost = decodePrice;
            bidPrice = decodePrice * AD_OWNER_COST_FACTOR;
        }
        if (priceType == PRICETYPE_RRTB_CPC) {
            if (logPhase != protocol::log::LogPhaseType::CLICK) {
                bidPrice = 0;
            } else {
                cost = 0;
            }
        }
    }

    int decodeOfferPrice(const std::string & input, bool useAes = false)
    {
        try {
            if (!useAes) {
                return std::stoi(input);
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
            if ((iter = paramMap.find(URL_ADOWNER_ID)) != paramMap.end()) { //广告主Id
                std::string & d = iter->second;                             // paramMap[URL_ADOWNER_ID];
                log.adInfo.advId = std::stol(d);
                log.adInfo.cpid = log.adInfo.advId;
            }
            //                if ((iter=paramMap.find(URL_ADPLAN_ID)) != paramMap.end()) { // 推广计划Id
            //                    std::string &t = iter->second;//paramMap[URL_ADPLAN_ID];
            //                    log.adInfo.cpid = std::stol(t);
            //                }
            if ((iter = paramMap.find(URL_EXEC_ID)) != paramMap.end()) { // 投放单元Id
                std::string & e = iter->second;                          // paramMap[URL_EXEC_ID];
                log.adInfo.sid = std::stol(e);
            }
            if ((iter = paramMap.find(URL_CREATIVE_ID)) != paramMap.end()) { // 创意Id
                std::string & c = iter->second;                              // paramMap[URL_CREATIVE_ID];
                log.adInfo.bannerId = std::stol(c);
            }
            if ((iter = paramMap.find(URL_ADX_ID)) != paramMap.end()) { // 平台Id,adxId
                std::string & x = iter->second;                         // paramMap[URL_ADX_ID];
                log.adInfo.adxid = std::stoi(x);
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
                log.clickx = std::stoi(sx);
            }
            if ((iter = paramMap.find(URL_CLICK_Y)) != paramMap.end()) { //点击坐标y
                std::string & sy = iter->second;                         // paramMap[URL_CLICK_Y];
                log.clicky = std::stoi(sy);
            }
            if ((iter = paramMap.find(URL_PRICE_TYPE)) != paramMap.end()) { //出价类型
                std::string & pricetype = iter->second;
                log.adInfo.priceType = std::stoi(pricetype);
            }
            if ((iter = paramMap.find(URL_EXCHANGE_PRICE)) != paramMap.end()) { //成交价格
                std::string & price = iter->second;                             // paramMap[URL_EXCHANGE_PRICE];
                // log.adInfo.cost = decodeAdxExchangePrice(log.adInfo.adxid,price);
                // log.adInfo.bidPrice = (int)(log.adInfo.cost * AD_OWNER_COST_FACTOR);
                int decodePrice = decodeAdxExchangePrice(log.adInfo.adxid, price);
                bool isYoukuDeal = paramMap.find(URL_YOUKU_DEAL) != paramMap.end() && !paramMap[URL_YOUKU_DEAL].empty();
                int offerPrice
                    = paramMap.find(URL_BID_PRICE) != paramMap.end() ? decodeOfferPrice(paramMap[URL_BID_PRICE]) : 0;
                calcPrice(log.adInfo.adxid, isYoukuDeal, decodePrice, offerPrice, log.adInfo.cost, log.adInfo.bidPrice,
                          log.logType, log.adInfo.priceType);
            }
            if ((iter = paramMap.find(URL_PRODUCTPACKAGE_ID)) != paramMap.end()) { //产品包id
                std::string & ppid = iter->second;
                log.adInfo.ppid = std::stoi(ppid);
            }
        } catch (std::exception & e) {
            log.reqStatus = 500;
            LOG_ERROR << "error:" << e.what() << ",when processing query " << allQuery;
        }
    }

    void checkAliEscapeSafe(protocol::log::LogItem & logItem, std::string & input)
    {
        using namespace adservice::utility::escape;
        std::string escape_string = encode4ali(input);
        std::string decode_string = decode4ali(escape_string);
        const char * a = input.c_str();
        const char * b = decode_string.c_str();
        if (input.length() == decode_string.length()) {
            bool isSame = true;
            for (size_t i = 0; i < input.length(); ++i) {
                if (a[i] != b[i]) {
                    isSame = false;
                    break;
                }
            }
            if (isSame) {
                protocol::log::LogItem parseLog;
                getAvroObject(parseLog, (uint8_t *)decode_string.c_str(), decode_string.length());
            } else {
				LOG_WARN << "decoded string not equal origin,escape4ali not safe";
            }
        } else {
			LOG_WARN << "decoded string length not equal origin,escape4ali not safe," << input.length() << " "
					 << decode_string.length();
        }
    }

    bool checkUserCookies(const std::string & oldCookies)
    {
        CypherResult128 cypherResult;
        memcpy((void *)cypherResult.bytes, (void *)oldCookies.c_str(), oldCookies.length());
        DecodeResult64 decodeResult64;
        if (!cookiesDecode(cypherResult, decodeResult64) || decodeResult64.words[0] <= 0
            || decodeResult64.words[0] > time::getCurrentTimeSinceMtty()) {
            return false;
        }
        return true;
    }

    void AbstractQueryTask::updateThreadData()
    {
        pthread_t thread = pthread_self();
        threadData = (TaskThreadLocal *)ThreadLocalManager::getInstance().get(thread);
        if (threadData == NULL) {
            threadData = new TaskThreadLocal;
            threadData->updateSeqId();
            ThreadLocalManager::getInstance().put(thread, threadData, &TaskThreadLocal::destructor);
        }
    }

    void AbstractQueryTask::filterParamMapSafe(ParamMap & paramMap)
    {
        for (ParamMap::iterator iter = paramMap.begin(); iter != paramMap.end(); iter++) {
            stringSafeInput(iter->second, URL_LONG_INPUT_PARAMETER);
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
        log.ipInfo.proxy = userIp;
        if (!isPost) { //对于非POST方法传送的Query
            getParamv2(paramMap, data.c_str());
            filterParamMapSafe(paramMap);
            parseObjectToLogItem(paramMap, log, data.c_str());
            std::string userId = extractCookiesParam(COOKIES_MTTY_ID, userCookies);
            bool needNewCookies = false;
            if (userId.empty() || !checkUserCookies(userId)) {
                CypherResult128 cookiesResult;
                makeCookies(cookiesResult);
                log.userId = (char *)cookiesResult.char_bytes;
                needNewCookies = true;
            } else {
                log.userId = userId;
            }
            if (needNewCookies) { //传入的cookies中没有userId,cookies 传出
                char cookiesString[64];
				sprintf(cookiesString, "%s=%s;Domain=.%s;Max-Age=2617488000;", COOKIES_MTTY_ID, log.userId.c_str(),
						COOKIES_MTTY_DOMAIN);
                resp.set("Set-Cookie", cookiesString);
            }
        } else { //对于POST方法传送过来的Query
            getPostParam(paramMap);
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
        if (serviceLogger.use_count() > 0) {
            serviceLogger->push(logString);
        } else {
            std::string loggerName = usedLoggerName();
            std::string logConfigKey = usedLoggerConfig();
            adservice::log::LogPusherPtr logPusher = adservice::log::LogPusher::getLogger(loggerName, logConfigKey);
            if (logPusher.use_count() > 0)
                logPusher->push(logString);
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
            if (needLog)
                doLog(log);
        } catch (std::exception & e) {
            resp.status(500, "error");
            resp.set_content_header("text/html");
            onError(e, resp);
        }
    }
}
}
