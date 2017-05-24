//
// Created by guoze.lin on 16/5/3.
//

#include "abstract_bidding_handler.h"
#include "core/config_types.h"
#include "core/core_ip_manager.h"
#include "logging.h"
#include "utility/utility.h"
#include <mtty/mtuser.h>

extern GlobalConfig globalConfig;

namespace protocol {
namespace bidding {

    using namespace adservice::utility::url;
    using namespace adservice::utility::cypher;
    using namespace adservice::core::model;

    static std::unordered_set<std::string> excludeIDFAs
        = { "9F89C84A559F573636A47FF8DAED0D33", "00000000-0000-0000-0000-000000000000" };

    static std::unordered_set<std::string> excludeIMEIs = { "5284047F4FFB4E04824A2FD1D1F0CD62",
                                                            "000000000000000",
                                                            "21371D265B5711B289344B479F583909",
                                                            "012345678912345",
                                                            "0",
                                                            "111111111111111" };

    static std::unordered_set<std::string> excludeMACs = { "02:00:00:00:00:00", "0F607264FC6318A92B9E13C65DB7CD3C",
                                                           "00:00:00:00:00:00", "528C8E6CD4A3C6598999A0E9DF15AD32" };

    static std::unordered_set<std::string> excludeAndroidIds = { "9774d56d682e549c" };

    static bool filterIDFA(const std::string & idfa)
    {
        return excludeIDFAs.find(idfa) != excludeIDFAs.end();
    }

    static bool filterIMEI(const std::string & imei)
    {
        return excludeIMEIs.find(imei) != excludeIMEIs.end();
    }

    static bool filterMAC(const std::string & mac)
    {
        return excludeMACs.find(mac) != excludeMACs.end();
    }

    static bool filterAndroidID(const std::string & androidId)
    {
        return excludeAndroidIds.find(androidId) != excludeAndroidIds.end();
    }

    void extractSize(const std::string & size, int & width, int & height)
    {
        const char *start = size.data(), *p = start;
        while (*p != '\0' && *p != 'x')
            p++;
        if (*p == '\0') {
            width = 0;
            height = 0;
        } else {
            width = atoi(start);
            height = atoi(p + 1);
        }
    }

    cppcms::json::value bannerJson2HttpsIOS(bool isIOS, const std::string & bannerJson, int bannerType)
    {
        cppcms::json::value banner;
        adservice::utility::json::parseJson(bannerJson.c_str(), banner);
        if (isIOS) {
            cppcms::json::value httpsBanner;
            std::string httpsBannerJson = bannerJson;
            adservice::utility::url::url_replace_all(httpsBannerJson, "http://", "https://");
            adservice::utility::json::parseJson(httpsBannerJson.c_str(), httpsBanner);
            cppcms::json::array & originMtlsArray = banner["mtls"].array();
            cppcms::json::array & httpsMtlsArray = httpsBanner["mtls"].array();
            if (bannerType == BANNER_TYPE_PRIMITIVE) {
                std::string destUrl = originMtlsArray[0]["p9"].str();
                adservice::utility::url::url_replace(destUrl, "{{click}}", "");
                httpsMtlsArray[0]["p9"] = destUrl;
            } else {
                std::string destUrl = originMtlsArray[0]["p1"].str();
                adservice::utility::url::url_replace(destUrl, "{{click}}", "");
                httpsMtlsArray[0]["p1"] = destUrl;
            }
            return httpsBanner;
        } else {
            cppcms::json::array & originMtlsArray = banner["mtls"].array();
            if (bannerType == BANNER_TYPE_PRIMITIVE) {
                std::string destUrl = originMtlsArray[0]["p9"].str();
                adservice::utility::url::url_replace(destUrl, "{{click}}", "");
                originMtlsArray[0]["p9"] = destUrl;
            } else {
                std::string destUrl = originMtlsArray[0]["p1"].str();
                adservice::utility::url::url_replace(destUrl, "{{click}}", "");
                originMtlsArray[0]["p1"] = destUrl;
            }
        }
        return banner;
    }

    std::string makeBidSize(int width, int height)
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "%dx%d", width, height);
        return std::string(buf);
    }

    const std::string & AbstractBiddingHandler::getShowBaseUrl(bool ssl)
    {
        if (ssl) {
            return globalConfig.urlConfig.sslShowUrl;
        } else {
            return globalConfig.urlConfig.nonSSLShowUrl;
        }
    }

    const std::string & AbstractBiddingHandler::getClickBaseUrl(bool ssl)
    {
        if (ssl) {
            return globalConfig.urlConfig.sslClickUrl;
        } else {
            return globalConfig.urlConfig.nonSSLClickUrl;
        }
    }

    std::string AbstractBiddingHandler::prepareVast(const MT::common::Banner & banner, const std::string & videoUrl,
                                                    const std::string & tvm, const std::string & cm)
    {
        std::string vastXml;
        ParamMap pm;
        pm.insert({ "title", "" });
        pm.insert({ "impressionUrl", tvm });
        pm.insert({ "duration", "00:00:15" });
        pm.insert({ "clickThrough", cm });
        pm.insert({ "bitrate", "300" });
        pm.insert({ "videoWidth", std::to_string(banner.width) });
        pm.insert({ "videoHeight", std::to_string(banner.height) });
        pm.insert({ "videoUrl", videoUrl });
        vastTemplateEngine.bindFast(pm, vastXml);
        return vastXml;
    }

    bool AbstractBiddingHandler::fillLogItem(const AdSelectCondition & queryCondition, protocol::log::LogItem & logItem,
                                             bool isAccepted)
    {
        logItem.reqStatus = 200;
        if (!isAccepted) {
            logItem.adInfo = protocol::log::AdInfo();
            buildFlowExtraInfo(queryCondition);
        }
        logItem.adInfo.adxid = queryCondition.adxid;
        logItem.adInfo.adxpid = queryCondition.adxpid;
        logItem.adInfo.bidBasePrice = queryCondition.basePrice;
        if (!cmInfo.userMapping.userId.empty()) {
            logItem.userId = cmInfo.userMapping.userId;
        }
        if (queryCondition.mttyContentType != 0) {
            logItem.contentTypes.push_back(queryCondition.mttyContentType);
        }
        if (!queryCondition.keywords.empty()) {
            std::stringstream ss;
            for (auto keyword : queryCondition.keywords) {
                ss << keyword << ",";
            }
            logItem.keyWords = ss.str();
        }
        logItem.device = adFlowExtraInfo.devInfo;
        logItem.dealIds = adFlowExtraInfo.dealIds;
        if (isAccepted) {
            logItem.adInfo.sid = adInfo.sid;
            logItem.adInfo.advId = adInfo.advId;
            logItem.adInfo.pid = adInfo.pid;
            logItem.adInfo.adxpid = adInfo.adxpid;
            logItem.adInfo.adxuid = adInfo.adxuid;
            logItem.adInfo.bannerId = adInfo.bannerId;
            logItem.adInfo.cid = adInfo.cid;
            logItem.adInfo.mid = adInfo.mid;
            logItem.adInfo.cpid = adInfo.cpid;
            logItem.adInfo.offerPrice = adInfo.offerPrice;
            logItem.adInfo.areaId = adInfo.areaId;
            logItem.adInfo.priceType = adInfo.priceType;
            logItem.adInfo.ppid = adInfo.ppid;
            extractAreaInfo(adInfo.areaId.data(), logItem.geoInfo.country, logItem.geoInfo.province,
                            logItem.geoInfo.city);
            logItem.adInfo.bidSize = adInfo.bidSize;
            logItem.adInfo.orderId = adInfo.orderId;
            logItem.dealIds = adFlowExtraInfo.dealIds;
            logItem.contentTypes = adFlowExtraInfo.contentType;
            logItem.mediaType = adFlowExtraInfo.mediaType;
            logItem.adInfo.bidEcpmPrice = adInfo.bidEcpmPrice;
            logItem.adInfo.imp_id = adInfo.imp_id;
        } else {
            logItem.adInfo.pid = std::to_string(queryCondition.mttyPid);
            logItem.adInfo.adxpid = queryCondition.adxpid;
            logItem.adInfo.adxid = queryCondition.adxid;
            logItem.adInfo.bidSize = makeBidSize(queryCondition.width, queryCondition.height);
        }
        return fillSpecificLog(queryCondition, logItem, isAccepted);
    }

    int AbstractBiddingHandler::extractRealValue(const std::string & input, int targetAdx)
    {
        const char * pdata = input.data();
        const char *p1 = pdata, *p2 = p1;
        while (*p2 != '\0') {
            if (*p2 == '|') {
                int adx = atoi(p1);
                p2++;
                p1 = p2;
                while (*p2 != '\0' && *p2 != '|')
                    p2++;
                if (adx == targetAdx) {
                    return atoi(p1);
                }
                if (*p2 == '|') {
                    p2++;
                    p1 = p2;
                }
            } else
                p2++;
        }
        return 0;
    }

    void AbstractBiddingHandler::getShowPara(URLHelper & showUrl, const std::string & bid)
    {
        showUrl.add("a", adInfo.areaId);
        showUrl.add("b", encodePrice(adInfo.offerPrice));
        showUrl.add("c", std::to_string(adInfo.bannerId));
        showUrl.add("d", std::to_string(adInfo.advId));
        showUrl.add("e", std::to_string(adInfo.sid));
        showUrl.add("s", adInfo.adxpid);
        showUrl.add("o", adInfo.pid);
        showUrl.add("x", std::to_string(adInfo.adxid));
        showUrl.add("r", adInfo.imp_id);
        showUrl.add("u", cmInfo.userMapping.cypherUid());
        showUrl.add("tm", std::to_string(time(NULL)));
        showUrl.add("pt", std::to_string(adInfo.priceType));
        showUrl.add("od", std::to_string(adInfo.orderId));
        showUrl.add("ep", std::to_string(adInfo.ppid));
        showUrl.add(URL_SITE_ID, std::to_string(adInfo.mid));
        showUrl.add(URL_CHANNEL, std::to_string(adInfo.cid));
        showUrl.add(URL_FLOWTYPE, std::to_string(adFlowExtraInfo.flowType));
        showUrl.add(URL_DEVICE_IDFA, adFlowExtraInfo.deviceIds[URL_DEVICE_IDFA]);
        showUrl.add(URL_DEVICE_IMEI, adFlowExtraInfo.deviceIds[URL_DEVICE_IMEI]);
        showUrl.add(URL_DEVICE_ANDOROIDID, adFlowExtraInfo.deviceIds[URL_DEVICE_ANDOROIDID]);
        showUrl.add(URL_DEVICE_MAC, adFlowExtraInfo.deviceIds[URL_DEVICE_MAC]);
    }

    void AbstractBiddingHandler::getClickPara(URLHelper & clickUrl, const std::string & bid, const std::string & ref,
                                              const std::string & landingUrl)
    {
        char buffer[1024];
        std::string encodedLandingUrl;
        urlEncode_f(landingUrl, encodedLandingUrl, buffer);
        std::string encodedReferer;
        if (ref.size() > 0)
            urlEncode_f(ref, encodedReferer, buffer);
        clickUrl.add("s", adInfo.adxpid);
        clickUrl.add("o", adInfo.pid);
        clickUrl.add("b", encodePrice(adInfo.offerPrice));
        clickUrl.add("x", std::to_string(adInfo.adxid));
        clickUrl.add("r", adInfo.imp_id);
        clickUrl.add("d", std::to_string(adInfo.advId));
        clickUrl.add("e", std::to_string(adInfo.sid));
        clickUrl.add("ep", std::to_string(adInfo.ppid));
        clickUrl.add("c", std::to_string(adInfo.bannerId));
        clickUrl.add("f", encodedReferer);
        clickUrl.add("h", "000");
        clickUrl.add("a", adInfo.areaId);
        clickUrl.add("u", cmInfo.userMapping.cypherUserId);
        clickUrl.add("pt", std::to_string(adInfo.priceType));
        clickUrl.add("od", std::to_string(adInfo.orderId));
        clickUrl.add("url", encodedLandingUrl);
        clickUrl.add(URL_SITE_ID, std::to_string(adInfo.mid));
        clickUrl.add(URL_CHANNEL, std::to_string(adInfo.cid));
        clickUrl.add(URL_DEVICE_IDFA, adFlowExtraInfo.deviceIds[URL_DEVICE_IDFA]);
        clickUrl.add(URL_DEVICE_IMEI, adFlowExtraInfo.deviceIds[URL_DEVICE_IMEI]);
        clickUrl.add(URL_DEVICE_ANDOROIDID, adFlowExtraInfo.deviceIds[URL_DEVICE_ANDOROIDID]);
        clickUrl.add(URL_DEVICE_MAC, adFlowExtraInfo.deviceIds[URL_DEVICE_MAC]);
    }

    std::string AbstractBiddingHandler::generateHtmlSnippet(const std::string & bid, int width, int height,
                                                            const char * extShowBuf, const char * cookieMappingUrl,
                                                            bool useHttps)
    {
        char html[4096];
        URLHelper showUrlParam;
        getShowPara(showUrlParam, bid);
        ParamMap extParamMap;
        getParamv2(extParamMap, extShowBuf);
        for (auto iter : extParamMap) {
            showUrlParam.add(iter.first, iter.second);
        }
        int len = snprintf(html, sizeof(html), SNIPPET_IFRAME, width, height, getShowBaseUrl(useHttps).c_str(), "",
                           showUrlParam.cipherParam().c_str(), cookieMappingUrl);
        return std::string(html, html + len);
    }

    std::string AbstractBiddingHandler::generateScript(const std::string & bid, int width, int height,
                                                       const char * scriptUrl, const char * clickMacro,
                                                       const char * extParam)
    {
        char script[4096];
        URLHelper showUrlParam;
        getShowPara(showUrlParam, bid);
        ParamMap extParamMap;
        getParamv2(extParamMap, extParam);
        for (auto iter : extParamMap) {
            showUrlParam.add(iter.first, iter.second);
        }
        int len = snprintf(script, sizeof(script), SNIPPET_SCRIPT, width, height, scriptUrl,
                           showUrlParam.cipherParam().c_str(), "", "", "", clickMacro);
        if (len >= (int)sizeof(script)) {
            LOG_WARN << "generateScript buffer size not enough,needed:" << len;
        }
        return std::string(script, script + len);
    }

    void AbstractBiddingHandler::buildFlowExtraInfo(const AdSelectCondition & selectCondition)
    {
        adFlowExtraInfo.devInfo.deviceType = selectCondition.mobileDevice;
        adFlowExtraInfo.devInfo.flowType = selectCondition.flowType;
        adFlowExtraInfo.devInfo.netWork = selectCondition.mobileNetwork;
        adFlowExtraInfo.devInfo.os = selectCondition.pcOS;
        adFlowExtraInfo.devInfo.deviceID = "";
        adFlowExtraInfo.devInfo.make
            = selectCondition.deviceMaker.empty() ? selectCondition.mobileModel : selectCondition.deviceMaker;
        adFlowExtraInfo.deviceIds.clear();
        adFlowExtraInfo.deviceIds.insert({ URL_DEVICE_IDFA, selectCondition.idfa });
        adFlowExtraInfo.deviceIds.insert({ URL_DEVICE_IMEI, selectCondition.imei });
        adFlowExtraInfo.deviceIds.insert({ URL_DEVICE_ANDOROIDID, selectCondition.androidId });
        if (selectCondition.mac != "02:00:00:00:00:00") {
            adFlowExtraInfo.deviceIds.insert({ URL_DEVICE_MAC, selectCondition.mac });
        }
        adFlowExtraInfo.contentType.clear();
        adFlowExtraInfo.contentType.push_back(selectCondition.mttyContentType);
        adFlowExtraInfo.mediaType = selectCondition.mediaType;
        adFlowExtraInfo.flowType = selectCondition.flowType;
        adFlowExtraInfo.dealIds.clear();
        if (!selectCondition.dealId.empty()) {
            adFlowExtraInfo.dealIds.push_back(selectCondition.dealId);
        }
    }

    void AbstractBiddingHandler::fillAdInfo(const AdSelectCondition & selectCondition,
                                            const MT::common::SelectResult & result,
                                            const std::string & adxUser)
    {
        const MT::common::Solution & finalSolution = result.solution;
        const MT::common::ADPlace & adplace = result.adplace;
        const MT::common::Banner & banner = result.banner;

        adInfo.pid = std::to_string(adplace.pId);
        adInfo.adxpid = selectCondition.adxpid;
        adInfo.sid = finalSolution.sId;

        auto advId = finalSolution.advId;
        adInfo.advId = advId;
        adInfo.adxid = selectCondition.adxid;
        adInfo.adxuid = adxUser;
        adInfo.bannerId = banner.bId;
        adInfo.cid = adplace.cId;
        adInfo.mid = adplace.mId;
        adInfo.cpid = adInfo.advId;
        adInfo.bidSize = makeBidSize(banner.width, banner.height);
        adInfo.priceType = finalSolution.priceType;
        adInfo.ppid = result.ppid;
        adInfo.orderId = result.orderId;
        adInfo.offerPrice = result.feePrice;
        adInfo.bidEcpmPrice = result.bidPrice;
        adInfo.bidBasePrice = selectCondition.basePrice;
        adInfo.areaId = adservice::server::IpManager::getInstance().getAreaCodeStrByIp(selectCondition.ip.data());
        auto idSeq = CookieMappingManager::IdSeq();
        adInfo.imp_id = std::to_string(idSeq.time()) + std::to_string(idSeq.id());
        buildFlowExtraInfo(selectCondition);
        std::string deviceId;
        char buffer[1024];
        adservice::utility::url::urlEncode_f(adFlowExtraInfo.devInfo.deviceID, deviceId, buffer);
        adFlowExtraInfo.devInfo.deviceID = deviceId;
        if (!selectCondition.dealId.empty() && finalSolution.dDealId != "0") {
            adFlowExtraInfo.dealIds.clear();
            adFlowExtraInfo.dealIds.push_back(finalSolution.dDealId);
        }
    }

    const CookieMappingQueryKeyValue &
    AbstractBiddingHandler::cookieMappingKeyMobile(const std::string & idfa,
                                                   const std::string & imei,
                                                   const std::string & androidId,
                                                   const std::string & mac,
                                                   const AdSelectCondition & selectCondition,
                                                   int appAdxId,
                                                   const std::string & appUserId)
    {
        cmInfo.queryKV.clearDeviceMapping();
        return cmInfo.queryKV
            .rebindDevice(MtUserMapping::idfaKey(), filterIDFA(selectCondition.idfa) ? "" : idfa, selectCondition.idfa)
            .rebindDevice(MtUserMapping::imeiKey(), filterIMEI(selectCondition.imei) ? "" : imei, selectCondition.imei)
            .rebindDevice(MtUserMapping::androidIdKey(), filterAndroidID(selectCondition.androidId) ? "" : androidId,
                          selectCondition.androidId)
            .rebindDevice(MtUserMapping::macKey(), filterMAC(selectCondition.mac) ? "" : mac, selectCondition.mac)
            .rebindDevice(MtUserMapping::adxUidKey(appAdxId), appUserId, "");
    }

    const CookieMappingQueryKeyValue & AbstractBiddingHandler::cookieMappingKeyPC(int64_t adxId,
                                                                                  const std::string & cookie)
    {
        if (!cookie.empty()) {
            return cmInfo.queryKV.rebind(MtUserMapping::adxUidKey(adxId), cookie, true);
        } else {
            return cmInfo.queryKV.rebind(MtUserMapping::adxUidKey(adxId), "", true);
        }
    }

    void AbstractBiddingHandler::queryCookieMapping(const CookieMappingQueryKeyValue & queryKV,
                                                    AdSelectCondition & selectCondition)
    {
        if (globalConfig.cmConfig.disabledCookieMapping) {
            cmInfo.needReMapping = false;
            cmInfo.needTouchMapping = false;
            cmInfo.needFixMapping = false;
            return;
        }
        cmInfo.needReMapping = false;
        cmInfo.needTouchMapping = false;
        cmInfo.needFixMapping = false;
        cmInfo.userMapping.reset();
        if (!queryKV.isNull()) { //查询键值非空
            CookieMappingManager & cmManager = CookieMappingManager::getInstance();
            // LOG_DEBUG << "cookie mapping query kv:" << queryKV.key << "," << queryKV.value;
            bool isGetTimeout = false;
            ;
            cmInfo.userMapping
                = cmManager.getUserMappingByKey(queryKV.key, queryKV.value, !queryKV.isAdxCookieKey(), isGetTimeout);
            if (cmInfo.userMapping.isValid()) {
                // todo: 填充selectCondition的对应字段
                selectCondition.mtUserId = cmInfo.userMapping.userId;
                if (!queryKV.isAdxCookieKey()
                    && cmInfo.userMapping.outerUserOriginId.empty()) { // device类型但是没有查到原deviceid
                    cmInfo.needFixMapping = true;
                } else { //其它情况
                    cmInfo.needTouchMapping = true;
                }
                return;
            } else if (!isGetTimeout) { //系统不超时而且服务端找不到对应记录，需要重新mapping
                cmInfo.needReMapping = true;
            }                                  //否则aerospike系统繁忙
        } else if (queryKV.isAdxCookieKey()) { //查询键值为空，但是需要植入cookie
            cmInfo.needReMapping = true;
        }
    }

    std::string AbstractBiddingHandler::redoCookieMapping(int64_t adxId, const std::string & adxCookieMappingUrl)
    {
        if (cmInfo.needReMapping) {
            auto idSeq = CookieMappingManager::IdSeq();
            MT::User::UserID userId(idSeq.id(), idSeq.time());
            cmInfo.userMapping.userId = userId.text();
            cmInfo.userMapping.cypherUserId = userId.cipher();
            if (cmInfo.queryKV.isAdxCookieKey()) {  // PC cookie
                if (!adxCookieMappingUrl.empty()) { // adx原生支持image标签cookie mapping
                    return adxCookieMappingUrl + "&u=" + userId.cipher() + "&x=" + std::to_string(adxId);
                }
            } else { // device id
                CookieMappingManager & cmManager = CookieMappingManager::getInstance();
                for (auto kvPair : cmInfo.queryKV.deviceMappings) {
                    // deviceMapping中有可能包含移动APP的用户ID,这类ID也归类到ADX的移动平台ID
                    bool isAdxKey = kvPair.first.find("adx") != std::string::npos;
                    cmManager.updateMappingDeviceAsync(cmInfo.userMapping.userId, kvPair.first, kvPair.second.first,
                                                       kvPair.second.second,
                                                       isAdxKey ? DAY_SECOND * 30 : DAY_SECOND * 900);
                }
            }
            cmInfo.needReMapping = false;
        } else if (cmInfo.needTouchMapping) { //更新mapping ttl
            CookieMappingManager & cmManager = CookieMappingManager::getInstance();
            if (cmInfo.queryKV.isAdxCookieKey()) {
                cmManager.touchMapping(cmInfo.queryKV.key, cmInfo.queryKV.value, cmInfo.userMapping.userId);
            } else {
                for (auto kvPair : cmInfo.queryKV.deviceMappings) {
                    bool isAdxKey = kvPair.first.find("adx") != std::string::npos;
                    cmManager.touchMapping(kvPair.first, kvPair.second.first, cmInfo.userMapping.userId,
                                           isAdxKey ? "" : kvPair.second.second,
                                           isAdxKey ? DAY_SECOND * 30 : DAY_SECOND * 900);
                }
            }
        } else if (cmInfo.needFixMapping) { //计划赶不上变化，修数据吧
            CookieMappingManager & cmManager = CookieMappingManager::getInstance();
            for (auto kvPair : cmInfo.queryKV.deviceMappings) {
                bool isAdxKey = kvPair.first.find("adx") != std::string::npos;
                cmManager.updateMappingDeviceAsync(cmInfo.userMapping.userId, kvPair.first, kvPair.second.first,
                                                   kvPair.second.second, isAdxKey ? DAY_SECOND * 30 : DAY_SECOND * 900);
            }
        }
        return "";
    }
}
}
