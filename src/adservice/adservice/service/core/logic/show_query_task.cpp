//
// Created by guoze.lin on 16/4/29.
//

#include "show_query_task.h"

#include <mtty/aerospike.h>
#include <mtty/requestcounter.h>
#include <mtty/usershowcounter.h>

#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include "common/atomic.h"
#include "core/adselectv2/ad_select_client.h"
#include "core/adselectv2/ad_select_interface.h"
#include "core/config_types.h"
#include "core/core_cm_manager.h"
#include "core/core_ip_manager.h"
#include "logging.h"
#include "service/protocol/dsps/dsp_handler_manager.h"
#include "utility/utility.h"
#include <mtty/constants.h>
#include <mtty/trafficcontrollproxy.h>
#include <mtty/usershowcounter.h>

extern adservice::adselectv2::AdSelectClientPtr adSelectClient;

extern MT::common::Aerospike aerospikeClient;
extern GlobalConfig globalConfig;

extern MT::common::RequestCounter requestCounter;

std::shared_ptr<MT::common::HttpClient> globalHttpClient = std::make_shared<MT::common::HttpClient>(true, true);

namespace protocol {
namespace bidding {

    extern cppcms::json::value bannerJson2HttpsIOS(bool isIOS, const std::string & bannerJson, int bannerType);
}

namespace dsp {

    extern thread_local DSPHandlerManager dspHandlerManager;
}
}

namespace adservice {
namespace corelogic {

    using namespace adservice;
    using namespace adservice::utility;
    using namespace adservice::utility::time;
    using namespace adservice::adselectv2;
    using protocol::bidding::bannerJson2HttpsIOS;

    static long handleShowRequests = 0;
    static long updateShowRequestsTime = 0;

    int HandleShowQueryTask::initialized = 0;
    char HandleShowQueryTask::showAdxTemplate[1024];
    char HandleShowQueryTask::showSspTemplate[1024];

    // static long adStatusShowTime = 0;

    void HandleShowQueryTask::loadTemplates()
    {
        if (initialized == 1 || !ATOM_CAS(&initialized, 0, 1))
            return;
        loadFile(showAdxTemplate, TEMPLATE_SHOW_ADX_PATH);
        loadFile(showSspTemplate, TEMPLATE_SHOW_SSP_PATH);
    }

    void HandleShowQueryTask::filterParamMapSafe(ParamMap & paramMap)
    {
        AbstractQueryTask::filterParamMapSafe(paramMap);
        if (paramMap.find(URL_CREATIVE_ID) != paramMap.end())
            stringSafeNumber(paramMap[URL_CREATIVE_ID]);
        if (paramMap.find(URL_SSP_PID) != paramMap.end())
            stringSafeNumber(paramMap[URL_SSP_PID]);
    }

    namespace {

        /**
         * 因为业务数据库数据插入ElasticSearch时会对引号进行转义
         * 此函数用于反转义引号
         */
        void tripslash(char * str)
        {
            char *p1 = str, *p2 = p1;
            while (*p2 != '\0') {
                if (*p2 == '\\' && p2[1] == '\'') {
                    p2++;
                }
                *p1++ = *p2++;
            }
            *p1 = '\0';
        }

        void tripslash2(char * str)
        {
            char *p1 = str, *p2 = p1;
            while (*p2 != '\0') {
                if (*p2 == '\\' && p2[1] == '\"') {
                    p2++;
                }
                *p1++ = *p2++;
            }
            *p1 = '\0';
        }

        /**
         * 判断曝光的类型
         * of=0 DSP,此为默认的情况
         * of=1 SSP
         * of=2 移动展示
         * of=3 移动日志
         * of=4 移动SSP
         */
        bool isShowForSSP(ParamMap & paramMap)
        {
            ParamMap::const_iterator iter = paramMap.find(URL_IMP_OF);
            if (iter == paramMap.end()) {
                return false;
            } else {
                return iter->second == OF_SSP || iter->second == OF_SSP_MOBILE;
            }
        }

        /**
         * 判断是否移动曝光
         */
        bool isShowForMobile(ParamMap & paramMap)
        {
            ParamMap::const_iterator iter = paramMap.find(URL_IMP_OF);
            if (iter == paramMap.end()) {
                return false;
            } else {
                return iter->second == OF_DSP_MOBILE_SHOW || iter->second == OF_DSP_MOBILE_LOG;
            }
        }

        /**
         * 根据OF类型判定是否需要显示创意和纪录日志,http://redmine.mtty.com/redmine/issues/48
         */
        void dspSetParam(ParamMap & paramMap, bool & showCreative, bool & shouldLog)
        {
            ParamMap::const_iterator iter = paramMap.find(URL_IMP_OF);
            if (iter == paramMap.end() || iter->second == OF_DSP) {
                showCreative = true;
                shouldLog = true;
            } else if (iter->second == OF_DSP_MOBILE_SHOW) {
                showCreative = true;
                shouldLog = false;
            } else if (iter->second == OF_DSP_MOBILE_LOG) {
                showCreative = false;
                shouldLog = true;
            }
        }

        int buildResponseForDsp(const MT::common::Banner & banner, ParamMap & paramMap, std::string & json,
                                const char * templateFmt, char * buffer, int bufferSize)
        {
            cppcms::json::value mtAdInfo;
            utility::json::parseJson(json.c_str(), mtAdInfo);
            //准备ADX宏
            char adxbuffer[1024];
            std::string adxMacro;
            urlDecode_f(paramMap[URL_ADX_MACRO], adxMacro, adxbuffer);
            adxMacro += ADX_MACRO_SUFFIX;
            //填充JSON对象
            mtAdInfo["pid"] = paramMap[URL_MTTYADPLACE_ID];
            mtAdInfo["adxpid"] = paramMap[URL_ADPLACE_ID];
            mtAdInfo["impid"] = paramMap[URL_EXPOSE_ID];
            mtAdInfo["unid"] = paramMap[URL_ADX_ID];
            mtAdInfo["plid"] = "";
            mtAdInfo["gpid"] = paramMap[URL_EXEC_ID];
            mtAdInfo["arid"] = paramMap[URL_AREA_ID];
            mtAdInfo["xcurl"] = adxMacro;
            mtAdInfo["of"] = OF_DSP;
            mtAdInfo["pricetype"] = paramMap[URL_PRICE_TYPE];
            mtAdInfo["price"] = paramMap[URL_BID_PRICE];
            mtAdInfo["ppid"] = paramMap[URL_PRODUCTPACKAGE_ID];
            std::string width = std::to_string(banner.width);
            std::string height = std::to_string(banner.height);
            mtAdInfo["width"] = width;
            mtAdInfo["height"] = height;
            mtAdInfo["oid"] = paramMap[URL_ORDER_ID];
            int32_t flowType = paramMap.find(URL_FLOWTYPE) != paramMap.end() ? std::stoi(paramMap[URL_FLOWTYPE])
                                                                             : SOLUTION_FLOWTYPE_UNKNOWN;
            if (flowType == SOLUTION_FLOWTYPE_MOBILE) {
                mtAdInfo["rs"] = true;
            }
            URLHelper clickUrl(globalConfig.urlConfig.sslClickUrl, false);
            for (auto iter : paramMap) {
                clickUrl.add(iter.first, iter.second);
            }
            clickUrl.add(URL_IMP_OF, OF_DSP);
            clickUrl.add(URL_ADX_MACRO, adxMacro);
            clickUrl.add(URL_DEVICE_IDFA, paramMap[URL_DEVICE_IDFA]);
            clickUrl.add(URL_DEVICE_IMEI, paramMap[URL_DEVICE_IMEI]);
            clickUrl.add(URL_DEVICE_ANDOROIDID, paramMap[URL_DEVICE_ANDOROIDID]);
            clickUrl.add(URL_DEVICE_MAC, paramMap[URL_DEVICE_MAC]);
            clickUrl.add(URL_EXCHANGE_PRICE, paramMap[URL_EXCHANGE_PRICE]);
            clickUrl.add(URL_FEE_RATE, paramMap[URL_FEE_RATE]);
            cppcms::json::value & mtlsArray = mtAdInfo["mtls"];
            cppcms::json::array & mtls = mtlsArray.array();
            char landingPageBuffer[1024];
            std::string landingUrl
                = banner.bannerType == BANNER_TYPE_PRIMITIVE ? mtls[0].get("p9", "") : mtls[0].get("p1", "");
            url_replace(landingUrl, "{{click}}", "");
            std::string encodedLandingUrl;
            urlEncode_f(landingUrl, encodedLandingUrl, landingPageBuffer);
            clickUrl.add(URL_LANDING_URL, encodedLandingUrl);
            mtAdInfo["clickurl"] = clickUrl.cipherUrl();
            std::string jsonResult = utility::json::toJson(mtAdInfo);
            int len = snprintf(buffer, bufferSize - 1, templateFmt, jsonResult.c_str());
            if (len >= bufferSize) {
                LOG_WARN << "in buildResponseForDsp buffer overflow,length:" << len;
                return bufferSize;
            }
            return len;
        }

        int buildResponseForSsp(const MT::common::SelectResult & selectResult, ParamMap & paramMap,
                                cppcms::json::value & mtAdInfo, const char * templateFmt, char * buffer, int bufferSize,
                                const std::string & userIp, const std::string & referer, const std::string & impId)
        {
            const MT::common::Solution & solution = selectResult.solution;
            const MT::common::Banner & banner = selectResult.banner;
            const MT::common::ADPlace & adplace = selectResult.adplace;
            std::string pid = std::to_string(adplace.pId);
            mtAdInfo["pid"] = pid;
            mtAdInfo["adxpid"] = pid;
            mtAdInfo["impid"] = impId;
            std::string adxid = std::to_string(adplace.adxId);
            mtAdInfo["unid"] = adxid;
            mtAdInfo["plid"] = "";
            std::string sid = std::to_string(solution.sId);
            mtAdInfo["gpid"] = sid;
            IpManager & ipManager = IpManager::getInstance();
            std::string address = ipManager.getAreaCodeStrByIp(userIp.data());
            mtAdInfo["arid"] = address;
            mtAdInfo["xcurl"] = "";
            std::string width = std::to_string(banner.width);
            std::string height = std::to_string(banner.height);
            mtAdInfo["width"] = width;
            mtAdInfo["height"] = height;
            std::string priceType = std::to_string(solution.priceType);
            mtAdInfo["pricetype"] = priceType;
            std::string price = encodePrice(selectResult.feePrice);
            mtAdInfo["price"] = price;
            std::string ppid = std::to_string(selectResult.ppid);
            mtAdInfo["ppid"] = ppid;
            mtAdInfo["oid"] = selectResult.orderId;
            mtAdInfo["ctype"] = banner.bannerType;
            int advId = solution.advId;
            int bId = banner.bId;
            int resultLen = 0;
            URLHelper clickUrl(globalConfig.urlConfig.sslClickUrl, false);
            clickUrl.add(URL_ADPLACE_ID, pid);
            clickUrl.add(URL_MTTYADPLACE_ID, pid);
            clickUrl.add(URL_ADX_ID, adxid);
            clickUrl.add(URL_EXPOSE_ID, impId);
            clickUrl.add(URL_ADOWNER_ID, std::to_string(advId));
            clickUrl.add(URL_EXEC_ID, sid);
            clickUrl.add(URL_PRODUCTPACKAGE_ID, ppid);
            clickUrl.add(URL_PRICE_TYPE, priceType);
            clickUrl.add(URL_BID_PRICE, price);
            clickUrl.add(URL_CREATIVE_ID, std::to_string(bId));
            clickUrl.add(URL_REFERER, referer);
            clickUrl.add(URL_CLICK_ID, "000");
            clickUrl.add(URL_AREA_ID, address);
            clickUrl.add(URL_SITE_ID, std::to_string(adplace.mId));
            clickUrl.add(URL_ORDER_ID, std::to_string(selectResult.orderId));
            if (adplace.priceType == PRICETYPE_RRTB_CPC) { // cpc采买的广告位成本放在点击阶段
                clickUrl.add(URL_ADPLACE_BUY_TYPE, std::to_string(PRICETYPE_RRTB_CPC));
                clickUrl.add(URL_EXCHANGE_PRICE, std::to_string(adplace.basePrice * 100));
            } // 否则点击不计算成本
            if (solution.priceType == PRICETYPE_RCPC
                || solution.priceType == PRICETYPE_RRTB_CPC) { // cpc结算，费率放到点击
                clickUrl.add(URL_FEE_RATE_STRS, selectResult.costRateDetails.getDetailStr(1.0));
                clickUrl.add(URL_FEE_RATE, std::to_string(selectResult.costRateDetails.spend));
            } // else cpm结算费率放到曝光
            //需求http://redmine.mtty.com/redmine/issues/144
            cppcms::json::value & mtlsArray = mtAdInfo["mtls"];
            cppcms::json::array & mtls = mtlsArray.array();
            char landingPageBuffer[1024];
            std::string landingUrl
                = banner.bannerType == BANNER_TYPE_PRIMITIVE ? mtls[0].get("p9", "") : mtls[0].get("p1", "");
            std::string encodedLandingUrl;
            urlEncode_f(landingUrl, encodedLandingUrl, landingPageBuffer);
            clickUrl.add(URL_LANDING_URL, encodedLandingUrl);
            if (banner.bannerType != BANNER_TYPE_PRIMITIVE) {
                mtls[0].set("p5", clickUrl.cipherUrl());
            } else {
                mtls[0].set("p9", clickUrl.cipherUrl());
            }
            if (paramMap[URL_IMP_OF] == OF_SSP_MOBILE) {
                //只输出标准json
                std::string jsonResult = utility::json::toJson(mtAdInfo);
                resultLen = snprintf(buffer, bufferSize - 1, "%s", jsonResult.c_str());
            } else {
                mtAdInfo["clickurl"] = clickUrl.cipherUrl();
                std::string jsonResult = utility::json::toJson(mtAdInfo);
                resultLen
                    = snprintf(buffer, bufferSize - 1, templateFmt, paramMap["callback"].c_str(), jsonResult.c_str());
            }
            if (resultLen >= bufferSize) {
                LOG_WARN << "in buildResponseForSsp buffer overflow,length:" << resultLen;
                return bufferSize;
            }
            return resultLen;
        }

        /**
         * 根据输入参数获取设备类型
         */
        int getDeviceTypeForSsp(ParamMap & paramMap)
        {
            std::string devType
                = paramMap.find(URL_SSP_DEVICE) != paramMap.end() ? paramMap[URL_SSP_DEVICE] : SSP_DEVICE_UNKNOWN;
            std::string platform
                = paramMap.find(URL_SSP_PLATFORM) != paramMap.end() ? paramMap[URL_SSP_PLATFORM] : SSP_PLATFORM_UNKNOWN;
            if (devType == SSP_DEVICE_MOBILEPHONE) {
                if (platform == SSP_PLATFORM_IOS) {
                    return SOLUTION_DEVICE_IPHONE;
                } else if (platform == SSP_PLATFORM_ANDROID) {
                    return SOLUTION_DEVICE_ANDROID;
                } else if (platform == SSP_PLATFORM_WINDOWSPHONE) {
                    return SOLUTION_DEVICE_WINDOWSPHONE;
                }
            } else if (devType == SSP_DEVICE_PAD) {
                if (platform == SSP_PLATFORM_IOS) {
                    return SOLUTION_DEVICE_IPAD;
                } else if (platform == SSP_PLATFORM_ANDROID) {
                    return SOLUTION_DEVICE_ANDROIDPAD;
                }
            }
            return SOLUTION_DEVICE_OTHER;
        }

        std::pair<double, double> getSSPGeo(ParamMap & paramMap)
        {
            using namespace adservice::utility::stringtool;
            auto iter = paramMap.find(URL_SSP_LONGITUDE);
            std::string longitude = iter == paramMap.end() ? "0" : iter->second;
            iter = paramMap.find(URL_SSP_LATITUDE);
            std::string latitude = iter == paramMap.end() ? "0" : iter->second;
            return { safeconvert(stod, longitude), safeconvert(stod, latitude) };
        }

        /**
                 * 根据参数填充condition对象
                 * @brief fillQueryConditionForSSP
                 * @param paramMap
                 * @param log
                 * @param condition
                 */
        void fillQueryConditionForSSP(ParamMap & paramMap, protocol::log::LogItem & log,
                                      adselectv2::AdSelectCondition & condition)
        {
            condition.mttyPid = std::stoi(paramMap[URL_SSP_PID]);
            condition.ip = log.ipInfo.proxy;
            server::IpManager & ipManager = IpManager::getInstance();
            condition.dGeo = ipManager.getAreaByIp(condition.ip.data());
            condition.mobileDevice = getDeviceTypeForSsp(paramMap);
            if (log.userAgent.empty() && paramMap.find(URL_SSP_USERAGENT) != paramMap.end()) {
                log.userAgent = utility::url::urlDecode_f(paramMap[URL_SSP_USERAGENT]);
            }
            if (condition.mobileDevice == SOLUTION_DEVICE_OTHER) {
                condition.mobileDevice = utility::userclient::getMobileTypeFromUA(log.userAgent);
            }
            condition.geo = getSSPGeo(paramMap);
            condition.pcOS = utility::userclient::getOSTypeFromUA(log.userAgent);
            condition.flowType
                = condition.mobileDevice != SOLUTION_DEVICE_OTHER ? SOLUTION_FLOWTYPE_MOBILE : SOLUTION_FLOWTYPE_PC;
            condition.dHour = adSelectTimeCodeUtc();
            std::string deviceId = paramMap[URL_SSP_DEVICEID];
            std::string cmDeviceKey;
            if (condition.mobileDevice == SOLUTION_DEVICE_ANDROID
                || condition.mobileDevice == SOLUTION_DEVICE_ANDROIDPAD) {
                condition.imei = deviceId;
                cmDeviceKey = adservice::core::model::MtUserMapping::imeiKey();
            } else if (condition.mobileDevice == SOLUTION_DEVICE_IPHONE
                       || condition.mobileDevice == SOLUTION_DEVICE_IPAD) {
                condition.idfa = deviceId;
                cmDeviceKey = adservice::core::model::MtUserMapping::idfaKey();
            }

            if (!deviceId.empty() && !cmDeviceKey.empty()) { //存在设备ID进行cookiemapping query
                auto & cmManager = adservice::server::CookieMappingManager::getInstance();
                bool isCMTimeout = false;
                adservice::core::model::MtUserMapping mapping
                    = cmManager.getUserMappingByKey(cmDeviceKey, deviceId, true, isCMTimeout);
                if (mapping.isValid()) {
                    condition.mtUserId = mapping.userId;
                    log.userId = mapping.userId;
                } else {
                    condition.mtUserId = log.userId;
                    cmManager.updateMappingDeviceAsync(log.userId, cmDeviceKey, deviceId, "", DAY_SECOND * 600);
                }
            } else {
                condition.mtUserId = log.userId;
            }
            if (condition.flowType == SOLUTION_FLOWTYPE_MOBILE) {
                condition.adxid = ADX_SSP_MOBILE;
            } else {
                condition.adxid = ADX_SSP_PC;
            }
            if (paramMap.find(URL_SSP_NETWORK) != paramMap.end()) {
                condition.mobileNetwork = std::stoi(paramMap[URL_SSP_NETWORK]);
            }
            if (paramMap.find(URL_SSP_SCREENWIDTH) != paramMap.end()
                && paramMap.find(URL_SSP_SCREENHEIGHT) != paramMap.end()) {
                condition.width = std::stoi(paramMap[URL_SSP_SCREENWIDTH]);
                condition.height = std::stoi(paramMap[URL_SSP_SCREENHEIGHT]);
            }
            if (paramMap.find(URL_SSP_CARRIER) != paramMap.end()) {
                condition.mobileNetWorkProvider = std::stoi(paramMap[URL_SSP_CARRIER]);
            }
            condition.userAgent = log.userAgent;
            condition.referer = log.referer;
        }

        struct DSPBidCounter {
            int64_t lastCommitTime{ -1 };
            std::unordered_map<int64_t, int64_t> rcounters;
            std::unordered_map<int64_t, int64_t> bcounters;

            void incCounter(int64_t dspId, bool isBidSuccess)
            {
                {
                    auto iter = rcounters.find(dspId);
                    if (iter == rcounters.end()) {
                        rcounters.insert({ dspId, 1 });
                    } else {
                        iter->second++;
                    }
                }
                if (isBidSuccess) {
                    auto iter = bcounters.find(dspId);
                    if (iter == bcounters.end()) {
                        bcounters.insert({ dspId, 1 });
                    } else {
                        iter->second++;
                    }
                }
            }
            void clear()
            {
                rcounters.clear();
                bcounters.clear();
            }

            bool empty()
            {
                return rcounters.empty() && bcounters.empty();
            }
        };

        thread_local DSPBidCounter dspBidCounter;

        void logDSPBid()
        {
            int64_t currentTime = utility::time::getCurrentTimeStampMs();
            if (currentTime < dspBidCounter.lastCommitTime + 30000 || dspBidCounter.empty()) {
                return;
            }
            try {
                std::string key = utility::time::timeStringFromTimeStamp(utility::time::getCurrentHourTime());
                MT::common::ASKey dailyKey(globalConfig.aerospikeConfig.funcNamespace(AS_NAMESPACE_ORDER), "dsp_bid",
                                           key);
                MT::common::ASOperation op(dspBidCounter.rcounters.size() + dspBidCounter.bcounters.size(),
                                           DAY_SECOND * 3);
                for (auto iter : dspBidCounter.rcounters) {
                    op.addIncr(std::to_string(iter.first) + "_r", (int64_t)iter.second);
                }
                for (auto iter : dspBidCounter.bcounters) {
                    op.addIncr(std::to_string(iter.first) + "_b", (int64_t)iter.second);
                }
                aerospikeClient.operateAsync(dailyKey, op);
                dspBidCounter.clear();
            } catch (MT::common::AerospikeExcption & e) {
                LOG_ERROR << "记录dsp bid计数失败,e:" << e.what() << "，code:" << e.error().code << e.error().message
                          << "，调用堆栈：" << std::endl
                          << e.trace();
            }
            dspBidCounter.lastCommitTime = currentTime;
        }

        /**
         * 向其他DSP发送竞价请求
         * @brief askOtherDsp
         * @param condition
         * @param dspSolutions
         * @param result
         * @return
         */
        bool askOtherDsp(adselectv2::AdSelectCondition & condition, std::vector<MT::common::Solution> & dspSolutions,
                         MT::common::SelectResult & result)
        {
            adservice::utility::PerformanceWatcher pw("ssp::askOtherDsp", 200);
            (void)pw;
            LOG_DEBUG << "向其他DSP转发SSP流量";
            try {
                // protocol::dsp::DSPPromise allBidPromise;
                const MT::common::ADPlace & adplace = result.adplace;
                std::vector<std::shared_ptr<MT::common::HttpRequest>> httpRequests;
                for (auto solution : dspSolutions) {
                    LOG_DEBUG << "向DSP " << solution.advId << "发送请求";
                    //暂且不考虑dspSolutions中有重复advId的情况,如果有，那可能是多个deal
                    auto handler = protocol::dsp::dspHandlerManager.getHandler(solution.advId);
                    // allBidPromise.after(handler->sendBid(condition, adplace));
                    condition.dealId = std::to_string(solution.sId);
                    httpRequests.emplace_back(handler->buildRequest(condition, adplace));
                }
                //等待所有请求
                // allBidPromise.sync();
                const auto & httpResponses = globalHttpClient->execute(httpRequests);
                std::vector<protocol::dsp::DSPBidResult> dspResults;
                int counter = 0;
                for (auto solution : dspSolutions) {
                    auto handler = protocol::dsp::dspHandlerManager.getHandler(solution.advId);
                    auto & httpResponse = httpResponses[counter++];
                    // const protocol::dsp::DSPBidResult & dspResult = handler->getResult();
                    const protocol::dsp::DSPBidResult & dspResult = handler->parseResponse(httpResponse, adplace);
                    if (dspResult.resultOk) {
                        dspResults.push_back(dspResult);
                        dspBidCounter.incCounter(solution.advId, true);
                    } else {
                        dspBidCounter.incCounter(solution.advId, false);
                    }
                    LOG_DEBUG << "DSPID:" << solution.advId << ",是否出价:" << (dspResult.resultOk ? "是" : "否");
                }
                if (dspResults.empty()) {
                    LOG_DEBUG << "所有DSP均无应答或超时";
                    logDSPBid();
                    return false;
                }
                //排序取第一高价
                //排序
                std::sort(dspResults.begin(), dspResults.end(),
                          [](const protocol::dsp::DSPBidResult & a, const protocol::dsp::DSPBidResult & b) {
                              return a.bidPrice > b.bidPrice;
                          });
                //从第一名中随机选取
                int finalIdx = adservice::utility::rankingtool::randomIndex(
                    dspResults.size(), [&dspResults](int idx) { return dspResults[idx].bidPrice; }, { 100, 0, 0 },
                    true);
                protocol::dsp::DSPBidResult & dspResult = dspResults[finalIdx];
                result.banner = dspResult.banner;
                result.bidPrice = dspResult.bidPrice;
                result.feePrice = dspResult.bidPrice;
                for (auto solution : dspSolutions) {
                    if (solution.advId == dspResult.dspId
                        && (dspResult.dealId.empty() || dspResult.dealId == std::to_string(solution.sId))) {
                        result.solution = solution;
                        break;
                    }
                }
                LOG_DEBUG << "最终选择DSP" << result.solution.advId << ",出价:" << dspResult.bidPrice;
                std::string auctionPrice = std::to_string((int64_t)dspResult.bidPrice);
                adservice::utility::url::URLHelper exchangeClickUrl(globalConfig.urlConfig.sslClickUrl, false);
                exchangeClickUrl.add(URL_ADX_ID, std::to_string(condition.adxid));
                exchangeClickUrl.add(URL_ADPLACE_ID, std::to_string(condition.mttyPid));
                exchangeClickUrl.add(URL_ADOWNER_ID, std::to_string(dspResult.dspId));
                std::string clickUrlUnEnc = exchangeClickUrl.toUrl() + "&" URL_LANDING_URL "=";
                std::string clickUrlEnc = adservice::utility::url::urlEncode_f(clickUrlUnEnc);
                //宏替换
                adservice::utility::url::url_replace_all(dspResult.banner.json, "%%WINNING_PRICE%%", auctionPrice);
                adservice::utility::url::url_replace_all(dspResult.banner.json, "%%CLICK_URL_UNESC%%", clickUrlUnEnc);
                adservice::utility::url::url_replace_all(dspResult.banner.json, "%%CLICK_URL_ESC%%", clickUrlEnc);

                for (auto & url : dspResult.laterAccessUrls) {
                    adservice::utility::url::url_replace_all(url, "%%WINNING_PRICE%%", auctionPrice);
                }
                std::vector<std::string> toSendUrls = dspResult.laterAccessUrls;
                //异步发送win notice或曝光监测链接
                protocol::dsp::dspHandlerManager.getExecutor().run([toSendUrls]() {
                    for (auto & url : toSendUrls) {
                        LOG_DEBUG << "request url async " << url;
                        // auto httpClientProxy = adservice::utility::HttpClientProxy::getInstance();
                        // httpClientProxy->getAsync(url, 0);
                        globalHttpClient->executeNoWait(std::make_shared<MT::common::HttpGet>(url, 0, true));
                    }
                });
                //记录日志
                logDSPBid();
                return true;
            } catch (protocol::dsp::DSPHandlerException & dspException) {
                LOG_ERROR << "askOtherDSP has dsp handler exception:" << dspException.what()
                          << ",backtrace:" << dspException.trace();
            } catch (std::exception & e) {
                LOG_ERROR << "askOtherDsp has exception:" << e.what();
            }
            return false;
        }
    }

    void HandleShowQueryTask::customLogic(ParamMap & paramMap, protocol::log::LogItem & log,
                                          adservice::utility::HttpResponse & response)
    {
        utility::PerformanceWatcher performanceWatcher("ShowQueryTask::customLogic");
        (void)performanceWatcher;
        bool isSSP = isShowForSSP(paramMap);
        const char * templateFmt = isSSP ? showSspTemplate : showAdxTemplate;

        int seqId = 0;
        seqId = threadData->seqId;
        std::string respBody;

        int64_t bgid = -1;
        if (isSSP) { // SSP
            adselectv2::AdSelectCondition condition;
            condition.isFromSSP = true;
            fillQueryConditionForSSP(paramMap, log, condition);
            MT::common::SelectResult selectResult;
            bool selectOk = adSelectClient->search(seqId, true, condition, selectResult);
            if (!selectOk) {
                requestCounter.increaseShowForSSPFailed();

                LOG_DEBUG << "ssp module:adselect failed";
                const MT::common::ADPlace & tmpadplace = selectResult.adplace;
                log.logType = protocol::log::LogPhaseType::BID;
                log.adInfo.adxid = condition.adxid;
                log.adInfo.pid = std::to_string(tmpadplace.pId);
                log.adInfo.adxpid = tmpadplace.adxPId;
                log.reqStatus = 500;
            }
            if (!selectOk || selectResult.solution.advId == ADV_BASE) { //麦田SSP找不到正常客户,将流量转给其他DSP
                MT::common::SelectResult dspResult;
                dspResult.adplace = selectResult.adplace;
                if (selectResult.otherDspSolutions.size() > 0
                    && askOtherDsp(condition, selectResult.otherDspSolutions, dspResult)) { //其他DSP有应答
                    selectResult = dspResult;
                } else if (!selectOk) { //其他DSP无应答而且没有打底投放
                    return;
                } //其他DSP无应答但有打底投放
            }

            const MT::common::Banner & banner = selectResult.banner;
            bgid = banner.bgId;
            const MT::common::Solution & finalSolution = selectResult.solution;
            const MT::common::ADPlace & adplace = selectResult.adplace;
            log.adInfo.bannerId = banner.bId;
            log.adInfo.advId = finalSolution.advId;
            log.adInfo.adxid = adplace.adxId;
            log.adInfo.pid = std::to_string(adplace.pId);
            log.adInfo.adxpid = adplace.adxPId;
            log.adInfo.sid = finalSolution.sId;
            log.adInfo.mid = adplace.mId;
            log.adInfo.cid = adplace.cId;
            log.adInfo.priceType = finalSolution.priceType;
            log.adInfo.ppid = selectResult.ppid;
            log.adInfo.orderId = selectResult.orderId;
            log.adInfo.bidEcpmPrice = selectResult.bidPrice;
            log.adInfo.bidBasePrice = adplace.basePrice;
            //财务模块逻辑接入,根据资费表重新计算SSP资源下对广告主的结算资费
            const MT::common::UserCostRateDetails & costDetail = selectResult.costRateDetails;
            double feeRate = 1.0 + costDetail.getFinalFeeRate();
            if (feeRate < costDetail.spend - 1e-5 || feeRate > costDetail.spend + 1e-5) {
                LOG_WARN << "fee rate not equial database fee rate,feeRate:" << feeRate
                         << ",database spend:" << costDetail.spend << ",advId:" << finalSolution.advId
                         << ",mediaOwnerId:" << adplace.mediaOwnerId;
            }
            if (finalSolution.priceType == PRICETYPE_RRTB_CPC
                || finalSolution.priceType == PRICETYPE_RCPC) { // CPC投放单花费放在点击阶段
                log.adInfo.bidPrice = 0;
            } else { // CPM投放单花费放在曝光阶段
                log.adInfo.bidPrice = selectResult.feePrice;
            }
            if (adplace.priceType == PRICETYPE_RRTB_CPM
                || adplace.priceType == 0) { //广告位采买类型为CPM，成本放在曝光阶段
                log.adInfo.cost = adplace.basePrice * 100;
            } else if (adplace.priceType == PRICETYPE_RRTB_CPC) { //广告位采买类型为CPC，成本放在点击阶段
                log.adInfo.cost = 0;
            }
            if (finalSolution.priceType != PRICETYPE_RRTB_CPC
                && finalSolution.priceType != PRICETYPE_RCPC) { // cpm结算的投放单，费率放到曝光
                log.adInfo.feeRateDetail = MT::common::costdetailVec(
                    costDetail.getDetailStr(1.0), log.adInfo.bidPrice / (1.0 + costDetail.getFinalFeeRate()));
            } // else cpc结算的投放单，费率放到点击

            auto & ipManager = adservice::server::IpManager::getInstance();
            ipManager.getAreaCodeByIp(condition.ip.data(), log.geoInfo.country, log.geoInfo.province, log.geoInfo.city);
            std::string bannerJson = banner.json;
            cppcms::json::value mtAdInfo = bannerJson2HttpsIOS(true, bannerJson, banner.bannerType);
            auto idSeq = CookieMappingManager::IdSeq();
            log.adInfo.imp_id = std::to_string(idSeq.time()) + std::to_string(idSeq.id());
            //返回结果
            char buffer[8192];
            int len = buildResponseForSsp(selectResult, paramMap, mtAdInfo, templateFmt, buffer, sizeof(buffer), userIp,
                                          referer, log.adInfo.imp_id);
            respBody = std::string(buffer, buffer + len);
            //狗尾续一个bid日志
            doLog(log);
            log.logType = protocol::log::LogPhaseType::BID;

            requestCounter.increaseShowForSSPSuccess();
        } else { // DSP of=0,of=2,of=3
            bool showCreative = true;
            dspSetParam(paramMap, showCreative, needLog);
            MT::common::Banner adBanner;
            int64_t bId = std::stoll(paramMap[URL_CREATIVE_ID]);
            bool adselectOK = adSelectClient->getBannerById(bId, adBanner);
            if (showCreative) { //需要显示创意
                if (!adselectOK) {
                    log.adInfo.bannerId = 0;
                    log.reqStatus = 500;
                    return;
                }
                bgid = adBanner.bgId;
                std::string bannerJson = adBanner.json;
                char buffer[8192];
                int len = buildResponseForDsp(adBanner, paramMap, bannerJson, templateFmt, buffer, sizeof(buffer));
                respBody = std::string(buffer, buffer + len);
            } else if (adselectOK) {
                bgid = adBanner.bgId;
            }
            requestCounter.increaseShowForDSP();
        }

        if (needLog && log.adInfo.ppid != DEFAULT_PRODUCT_PACKAGE_ID) {
            int64_t orderId = 0;
            try {
                orderId = log.adInfo.orderId;

                MT::common::ASKey key(globalConfig.aerospikeConfig.funcNamespace(AS_NAMESPACE_ORDER), "order-counter",
                                      orderId);
                MT::common::ASOperation op(1);
                op.addIncr("s", (int64_t)1);

                aerospikeClient.operateAsync(key, op);
            } catch (MT::common::AerospikeExcption & e) {
                LOG_ERROR << "记录曝光失败，订单ID：" << orderId << "，" << e.what() << "，code:" << e.error().code
                          << e.error().message << "，调用堆栈：" << std::endl
                          << e.trace();
            } catch (std::exception & e) {
                LOG_ERROR << "记录曝光失败，订单ID无效：" << paramMap[URL_ORDER_ID];
            }
        }

        // 用户曝光频次控制
        if (!log.userId.empty() && needLog) {
            try {
                MT::common::ASKey dailyKey(globalConfig.aerospikeConfig.funcNamespace(AS_NAMESPACE_FREQ), "user-freq",
                                           log.userId + "d");
                MT::common::ASKey hourlyKey(globalConfig.aerospikeConfig.funcNamespace(AS_NAMESPACE_FREQ), "user-freq",
                                            log.userId + "h");

                if (!aerospikeClient.exists(dailyKey)) {
                    boost::posix_time::ptime now = boost::posix_time::second_clock::universal_time();
                    boost::posix_time::ptime todayEnd(now.date(), boost::posix_time::time_duration(24, 0, 0));

                    adservice::core::model::UserShowCounter counter(log.adInfo.bannerId, bgid, log.adInfo.sid,
                                                                    (todayEnd - now).total_seconds());
                    aerospikeClient.putAsync(dailyKey, counter);
                } else {
                    MT::common::ASMapOperation mapOP(3, -2);
                    mapOP.addMapIncr("banners", log.adInfo.bannerId, 1);
                    mapOP.addMapIncr("bannergroups", bgid, 1);
                    mapOP.addMapIncr("solutions", log.adInfo.sid, 1);

                    aerospikeClient.operateAsync(dailyKey, mapOP);
                }

                if (!aerospikeClient.exists(hourlyKey)) {
                    adservice::core::model::UserShowCounter counter(log.adInfo.bannerId, bgid, log.adInfo.sid, 60 * 60);
                    aerospikeClient.putAsync(hourlyKey, counter);
                } else {
                    MT::common::ASMapOperation mapOP(3, -2);
                    mapOP.addMapIncr("banners", log.adInfo.bannerId, 1);
                    mapOP.addMapIncr("bannergroups", bgid, 1);
                    mapOP.addMapIncr("solutions", log.adInfo.sid, 1);

                    aerospikeClient.operateAsync(hourlyKey, mapOP);
                }
            } catch (MT::common::AerospikeExcption & e) {
                LOG_ERROR << "记录曝光频次失败，userId：" << log.userId << "，sid:" << log.adInfo.sid
                          << ",bannerID:" << log.adInfo.bannerId << ",bgid:" << bgid << ",e:" << e.what()
                          << "，code:" << e.error().code << e.error().message << "，调用堆栈：" << std::endl
                          << e.trace();
            }
        }

#ifdef USE_ENCODING_GZIP
        Buffer body;
        muduo::net::ZlibOutputStream zlibOutStream(&body);
        zlibOutStream.write(respBody);
        zlibOutStream.finish();
        response.setBody(body.retrieveAllAsString());
#else
        response.set_body(respBody);
#endif
        response.set_content_header("text/html; charset=utf-8");
        response.set_header("Pragma", "no-cache");
        response.set_header("Cache-Control", "no-cache,no-store;must-revalidate");
        response.set_header("P3p",
                            "CP=\"CURa ADMa DEVa PSAo PSDo OUR BUS UNI PUR INT DEM STA PRE COM NAV OTC NOI DSP COR\"");
        handleShowRequests++;
        if (handleShowRequests % 10000 == 1) {
            int64_t todayStartTime = time::getTodayStartTime();
            if (updateShowRequestsTime < todayStartTime) {
                handleShowRequests = 1;
                updateShowRequestsTime = todayStartTime;
            } else {
                LOG_INFO << "handleShowRequests:" << handleShowRequests;
            }
        }
    }
}
}
