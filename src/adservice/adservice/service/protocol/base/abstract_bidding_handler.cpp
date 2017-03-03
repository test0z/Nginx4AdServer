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

    std::string makeBidSize(int width, int height)
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "%dx%d", width, height);
        return std::string(buf);
    }

    bool AbstractBiddingHandler::fillLogItem(const AdSelectCondition & queryCondition, protocol::log::LogItem & logItem,
                                             bool isAccepted)
    {
        logItem.reqStatus = 200;
        if (!isAccepted) {
            logItem.adInfo = protocol::log::AdInfo();
        }
        logItem.adInfo.adxid = adInfo.adxid;
        logItem.adInfo.adxpid = adInfo.adxpid;
        logItem.adInfo.bidBasePrice = queryCondition.basePrice;
        if (!cmInfo.userMapping.userId.empty()) {
            logItem.userId = cmInfo.userMapping.userId;
            logItem.device = adFlowExtraInfo.devInfo;
        }
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

    void AbstractBiddingHandler::getShowPara(const std::string & bid, char * showParamBuf, int showBufSize)
    {
        int len;
        if (showParamBuf != NULL) {
            len = snprintf(showParamBuf, showBufSize,
                           "a=%s&b=%s&c=%ld&d=%ld&e=%ld&s=%s&o=%s&x=%d&r=%s&u=%s&tm=%ld&pt=%d&od=%ld&ep=%d",
                           adInfo.areaId.c_str(), encodePrice(adInfo.offerPrice).c_str(), adInfo.bannerId, adInfo.advId,
                           adInfo.sid, adInfo.adxpid.c_str(), adInfo.pid.c_str(), adInfo.adxid, bid.c_str(),
                           cmInfo.userMapping.cypherUserId.c_str(), time(NULL), adInfo.priceType, adInfo.orderId,
                           adInfo.ppid);
            if (len >= showBufSize) {
                LOG_WARN << "In AbstractBiddingHandler::httpsnippet,showBufSize too small,actual:" << len;
            }
        }
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
        showUrl.add(URL_FLOWTYPE, std::to_string(adFlowExtraInfo.flowType));
    }

    void AbstractBiddingHandler::getClickPara(const std::string & bid, char * clickParamBuf, int clickBufSize,
                                              const std::string & ref, const std::string & landingUrl)
    {
        if (clickParamBuf != NULL) {
            char buffer[1024];
            std::string encodedLandingUrl;
            urlEncode_f(landingUrl, encodedLandingUrl, buffer);
            std::string encodedReferer;
            if (ref.size() > 0)
                urlEncode_f(ref, encodedReferer, buffer);
            int len
                = snprintf(clickParamBuf, clickBufSize,
                           "s=%s&o=%s&b=%s&x=%d&r=%s&d=%ld&e=%ld&ep=%d&c=%ld&f=%s&h=000&a=%s&u=%s&pt=%d&od=%ld&url=%s",
                           adInfo.adxpid.c_str(), adInfo.pid.c_str(), encodePrice(adInfo.offerPrice).c_str(),
                           adInfo.adxid, adInfo.imp_id.c_str(), adInfo.advId, adInfo.sid, adInfo.ppid, adInfo.bannerId,
                           encodedReferer.c_str(), adInfo.areaId.c_str(), cmInfo.userMapping.cypherUserId.c_str(),
                           adInfo.priceType, adInfo.orderId, encodedLandingUrl.c_str());
            if (len >= clickBufSize) {
                LOG_WARN << "in AbstractBiddingHandler::getClickPara,clickBufSize too small,actual:" << len;
                return;
            }
            std::string deviceParam = "&";
            deviceParam += adFlowExtraInfo.deviceIdName + "=" + adFlowExtraInfo.devInfo.deviceID;
            strncat(clickParamBuf, deviceParam.c_str(), clickBufSize - len - 1);
        }
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
        clickUrl.add(adFlowExtraInfo.deviceIdName, adFlowExtraInfo.devInfo.deviceID);
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
        int len = snprintf(html, sizeof(html), SNIPPET_IFRAME, width, height,
                           useHttps ? SNIPPET_SHOW_URL_HTTPS : SNIPPET_SHOW_URL, "", showUrlParam.cipherParam().c_str(),
                           cookieMappingUrl);
        return std::string(html, html + len);
    }

    std::string AbstractBiddingHandler::generateScript(const std::string & bid, int width, int height,
                                                       const char * scriptUrl, const char * clickMacro,
                                                       const char * extParam)
    {
        char showBuf[2048] = { "\0" };
        char clickBuf[2048] = { "\0" };
        char script[4096];
        getShowPara(bid, showBuf, sizeof(showBuf));
        int len = snprintf(script, sizeof(script), SNIPPET_SCRIPT, width, height, scriptUrl, showBuf, clickBuf,
                           clickBuf, extParam, clickMacro);
        if (len >= (int)sizeof(script)) {
            LOG_WARN << "generateScript buffer size not enough,needed:" << len;
        }
        return std::string(script, script + len);
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
        adFlowExtraInfo.devInfo.deviceType = selectCondition.mobileDevice;
        adFlowExtraInfo.devInfo.flowType = selectCondition.flowType;
        adFlowExtraInfo.devInfo.netWork = selectCondition.mobileNetwork;
        adFlowExtraInfo.devInfo.os = selectCondition.mobileDevice;
        adFlowExtraInfo.devInfo.deviceID = "";
        if (selectCondition.mobileDevice == SOLUTION_DEVICE_ANDROID
            || selectCondition.mobileDevice == SOLUTION_DEVICE_ANDROIDPAD) {
            if (selectCondition.imei.empty()) {
                adFlowExtraInfo.devInfo.deviceID = selectCondition.androidId;
                adFlowExtraInfo.deviceIdName = URL_DEVICE_ANDOROIDID;
            } else {
                adFlowExtraInfo.devInfo.deviceID = selectCondition.imei;
                adFlowExtraInfo.deviceIdName = URL_DEVICE_IMEI;
            }
        } else if (selectCondition.mobileDevice == SOLUTION_DEVICE_IPHONE
                   || selectCondition.mobileDevice == SOLUTION_DEVICE_IPAD) {
            adFlowExtraInfo.devInfo.deviceID = selectCondition.idfa;
            adFlowExtraInfo.deviceIdName = URL_DEVICE_IDFA;
        }
        if (adFlowExtraInfo.devInfo.deviceID.empty()) {
            adFlowExtraInfo.devInfo.deviceID = selectCondition.mac;
            adFlowExtraInfo.deviceIdName = URL_DEVICE_MAC;
        }
        std::string deviceId;
        char buffer[1024];
        adservice::utility::url::urlEncode_f(adFlowExtraInfo.devInfo.deviceID, deviceId, buffer);
        adFlowExtraInfo.devInfo.deviceID = deviceId;
        adFlowExtraInfo.contentType.clear();
        adFlowExtraInfo.contentType.push_back(selectCondition.mttyContentType);
        adFlowExtraInfo.mediaType = selectCondition.mediaType;
        if (!selectCondition.dealId.empty() && finalSolution.dDealId != "0") {
            adFlowExtraInfo.dealIds.clear();
            adFlowExtraInfo.dealIds.push_back(finalSolution.dDealId);
        }
        adFlowExtraInfo.flowType = selectCondition.flowType;
    }

    const CookieMappingQueryKeyValue & AbstractBiddingHandler::cookieMappingKeyMobile(const std::string & idfa,
                                                                                      const std::string & imei,
                                                                                      const std::string & androidId,
                                                                                      const std::string & mac)
    {
        if (!idfa.empty()) {
            return cmInfo.queryKV.rebind(MtUserMapping::idfaKey(), idfa, false);
        } else if (!imei.empty()) {
            return cmInfo.queryKV.rebind(MtUserMapping::imeiKey(), imei, false);
        } else if (!androidId.empty()) {
            return cmInfo.queryKV.rebind(MtUserMapping::androidIdKey(), androidId, false);
        } else if (!mac.empty()) {
            return cmInfo.queryKV.rebind(MtUserMapping::macKey(), mac, false);
        }
        return cmInfo.queryKV.rebind("", "", false);
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
            return;
        }
        cmInfo.needReMapping = false;
        cmInfo.userMapping.reset();
        if (!queryKV.isNull()) { //查询键值非空
            CookieMappingManager & cmManager = CookieMappingManager::getInstance();
            // LOG_DEBUG << "cookie mapping query kv:" << queryKV.key << "," << queryKV.value;
            cmInfo.userMapping = cmManager.getUserMappingByKey(queryKV.key, queryKV.value);
            if (cmInfo.userMapping.isValid()) {
                // todo: 填充selectCondition的对应字段
                selectCondition.mtUserId = cmInfo.userMapping.userId;
                return;
            } else { //服务端找不到对应记录，需要重新mapping
                cmInfo.needReMapping = true;
            }
        } else if (queryKV.isAdxCookieKey()) { //查询键值为空，但是需要植入cookie
            cmInfo.needReMapping = true;
        }
    }

    /**
     * u参数引入解决的问题：
     * 对于一个新用户，重来没有过麦田cookie,在服务端也没有任何的用户标识，rtb流量因为找不到服务端用户标识而触发cookiemapping,
     * 对于tanx，youku一类的网页平台，由于cookiemapping请求和曝光请求以异步请求形式同时嵌入到网页中，这时种cookie有可能引发并发问题：
     * 即m请求和s请求同时到达服务端,如果由于都没有旧cookie，于是都各自生成一个，真正被种植在客户端的cookie是不确定的，意味着其中一个请求的影响将被忽略。
     * 所以在第一次进行cookie mapping时，将合法的u参数写入mapping url,show url以及click url中。
     * 事实上，当u参数不写入时,相当于忽略上边这个问题带来的影响（最多就是首次曝光频次没有记录上），并不会影响到整个cookiemapping流程
     */

    std::string AbstractBiddingHandler::redoCookieMapping(int64_t adxId, const std::string & adxCookieMappingUrl)
    {
        if (cmInfo.needReMapping) {
            // MT::User::UserID userId(int16_t(adservice::utility::rng::randomInt() & 0x0000FFFF));
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
                cmInfo.userMapping.addDeviceMapping(cmInfo.queryKV.key, cmInfo.queryKV.value);
                cmManager.updateMappingDeviceAsync(cmInfo.userMapping.userId, cmInfo.queryKV.key, cmInfo.queryKV.value);
            }
            cmInfo.needReMapping = false;
        }
        return "";
    }
}
}
