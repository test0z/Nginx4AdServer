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
#include "utility/utility.h"
#include <mtty/trafficcontrollproxy.h>
#include <mtty/usershowcounter.h>

extern adservice::adselectv2::AdSelectClientPtr adSelectClient;

extern MT::common::Aerospike aerospikeClient;
extern GlobalConfig globalConfig;

extern MT::common::RequestCounter requestCounter;

namespace protocol {
namespace bidding {

    extern cppcms::json::value bannerJson2HttpsIOS(bool isIOS, const std::string & bannerJson, int bannerType);
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
        URLHelper clickUrl(SNIPPET_CLICK_URL_HTTPS, false);
        for (auto iter : paramMap) {
            clickUrl.add(iter.first, iter.second);
        }
        clickUrl.add(URL_IMP_OF, OF_DSP);
        clickUrl.add(URL_ADX_MACRO, adxMacro);
        clickUrl.add(URL_DEVICE_IDFA, paramMap[URL_DEVICE_IDFA]);
        clickUrl.add(URL_DEVICE_IMEI, paramMap[URL_DEVICE_IMEI]);
        clickUrl.add(URL_DEVICE_ANDOROIDID, paramMap[URL_DEVICE_ANDOROIDID]);
        clickUrl.add(URL_DEVICE_MAC, paramMap[URL_DEVICE_MAC]);
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
            resultLen = snprintf(buffer, bufferSize - 1, templateFmt, paramMap["callback"].c_str(), jsonResult.c_str());
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

    void HandleShowQueryTask::customLogic(ParamMap & paramMap, protocol::log::LogItem & log,
                                          adservice::utility::HttpResponse & response)
    {
        bool isSSP = isShowForSSP(paramMap);
        const char * templateFmt = isSSP ? showSspTemplate : showAdxTemplate;

        int seqId = 0;
        seqId = threadData->seqId;
        std::string respBody;

        int64_t bgid = -1;
        if (isSSP) { // SSP
            std::string & queryPid = paramMap[URL_SSP_PID];
            bool isAdxPid = false;
            if (queryPid.empty() || queryPid == "0") {
                queryPid = paramMap[URL_SSP_ADX_PID];
                if (queryPid.empty()) { // URL 参数有问题
                    requestCounter.increaseShowForSSPFailed();

                    LOG_ERROR << "in show module,ssp url pid is empty";
                    return;
                }
                isAdxPid = true;
            }
            adselectv2::AdSelectCondition condition;
            condition.isFromSSP = true;
            if (isAdxPid)
                condition.adxpid = queryPid;
            else
                condition.mttyPid = std::stoi(queryPid);
            condition.ip = log.ipInfo.proxy;
            server::IpManager & ipManager = IpManager::getInstance();
            condition.dGeo = ipManager.getAreaByIp(condition.ip.data());
            condition.mobileDevice = getDeviceTypeForSsp(paramMap);
            if (userAgent.empty() && paramMap.find(URL_SSP_USERAGENT) != paramMap.end()) {
                char buffer[1024];
                utility::url::urlDecode_f(paramMap[URL_SSP_USERAGENT], userAgent, buffer);
            }
            if (condition.mobileDevice == SOLUTION_DEVICE_OTHER) {
                condition.mobileDevice = utility::userclient::getMobileTypeFromUA(userAgent);
            }
            condition.pcOS = utility::userclient::getOSTypeFromUA(userAgent);
            condition.flowType
                = condition.mobileDevice != SOLUTION_DEVICE_OTHER ? SOLUTION_FLOWTYPE_MOBILE : SOLUTION_FLOWTYPE_PC;
            condition.dHour = adSelectTimeCodeUtc();
            condition.mtUserId = log.userId;
            if (condition.flowType == SOLUTION_FLOWTYPE_MOBILE) {
                condition.adxid = ADX_SSP_MOBILE;
            } else {
                condition.adxid = ADX_SSP_PC;
            }
            MT::common::SelectResult selectResult;
            if (!adSelectClient->search(seqId, true, condition, selectResult)) {
                requestCounter.increaseShowForSSPFailed();

                LOG_DEBUG << "ssp module:adselect failed";
                const MT::common::ADPlace & tmpadplace = selectResult.adplace;
                log.logType = protocol::log::LogPhaseType::BID;
                log.adInfo.adxid = condition.adxid;
                log.adInfo.pid = std::to_string(tmpadplace.pId);
                log.adInfo.adxpid = tmpadplace.adxPId;
                log.reqStatus = 500;
                return;
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
            if (finalSolution.priceType == PRICETYPE_RRTB_CPC || finalSolution.priceType == PRICETYPE_RCPC) {
                log.adInfo.bidPrice = 0;
            } else {
                log.adInfo.bidPrice = selectResult.feePrice; // offerprice
            }
            log.adInfo.cost = adplace.costPrice;
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

                aerospikeClient.operate(key, op);
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
                    aerospikeClient.put(dailyKey, counter);
                } else {
                    MT::common::ASMapOperation mapOP(3, -2);
                    mapOP.addMapIncr("banners", log.adInfo.bannerId, 1);
                    mapOP.addMapIncr("bannergroups", bgid, 1);
                    mapOP.addMapIncr("solutions", log.adInfo.sid, 1);

                    aerospikeClient.operate(dailyKey, mapOP);
                }

                if (!aerospikeClient.exists(hourlyKey)) {
                    adservice::core::model::UserShowCounter counter(log.adInfo.bannerId, bgid, log.adInfo.sid, 60 * 60);
                    aerospikeClient.put(hourlyKey, counter);
                } else {
                    MT::common::ASMapOperation mapOP(3, -2);
                    mapOP.addMapIncr("banners", log.adInfo.bannerId, 1);
                    mapOP.addMapIncr("bannergroups", bgid, 1);
                    mapOP.addMapIncr("solutions", log.adInfo.sid, 1);

                    aerospikeClient.operate(hourlyKey, mapOP);
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
