//
// Created by guoze.lin on 16/5/3.
//

#include "abstract_bidding_handler.h"
#include "core/core_ip_manager.h"
#include "logging.h"
#include "utility/utility.h"
#include <mtty/mtuser.h>

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
            }
        }
    }

    std::string AbstractBiddingHandler::generateHtmlSnippet(const std::string & bid, int width, int height,
                                                            const char * extShowBuf, const char * cookieMappingUrl)
    {
        char showBuf[2048];
        char html[4096];
        getShowPara(bid, showBuf, sizeof(showBuf));
        size_t len = (size_t)snprintf(html, sizeof(html), SNIPPET_IFRAME, width, height, SNIPPET_SHOW_URL, extShowBuf,
                                      showBuf, cookieMappingUrl);
        if (len >= sizeof(html)) {
            LOG_WARN << "generateHtmlSnippet buffer size not enough,needed:" << len;
        }
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
        adInfo.areaId = adservice::server::IpManager::getInstance().getAreaCodeStrByIp(selectCondition.ip.data());
    }

    const CookieMappingQueryKeyValue & AbstractBiddingHandler::cookieMappingKeyMobile(const std::string & idfa,
                                                                                      const std::string & imei)
    {
        if (!idfa.empty()) {
            return cmInfo.queryKV.rebind(MtUserMapping::idfaKey(), idfa, false);
        } else if (!imei.empty()) {
            return cmInfo.queryKV.rebind(MtUserMapping::imeiKey(), imei, false);
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
        cmInfo.needReMapping = false;
        cmInfo.userMapping.reset();
        if (!queryKV.isNull()) { //查询键值非空
            CookieMappingManager & cmManager = CookieMappingManager::getInstance();
            cmInfo.userMapping = cmManager.getUserMappingByKey(queryKV.key, queryKV.value);
            if (cmInfo.userMapping.isValid()) {
                // todo: 填充selectCondition的对应字段
                return;
            } else { //服务端找不到对应记录，需要重新mapping
                cmInfo.needReMapping = true;
            }
        } else if (queryKV.isAdxCookieKey()) { //查询键值为空，但是需要植入cookie
            cmInfo.needReMapping = true;
        }
    }

    std::string AbstractBiddingHandler::redoCookieMapping(int64_t adxId, const std::string & adxCookieMappingUrl)
    {
        if (cmInfo.needReMapping) {
            MT::User::UserID userId(int16_t(adservice::utility::rng::randomInt() & 0x0000FFFF));
            cmInfo.userMapping.userId = userId.text();
            cmInfo.userMapping.cypherUserId = userId.cipher();
            if (cmInfo.queryKV.isAdxCookieKey()) {  // PC cookie
                if (!adxCookieMappingUrl.empty()) { // adx原生支持image标签cookie mapping
                    return adxCookieMappingUrl + "&u=" + userId.cipher() + "&x=" + std::to_string(adxId);
                }
            } else { // device id
                CookieMappingManager & cmManager = CookieMappingManager::getInstance();
                cmInfo.userMapping.addDeviceMapping(cmInfo.queryKV.key, cmInfo.queryKV.value);
                cmManager.updateUserMappingAsync(cmInfo.userMapping);
            }
        }
        return "";
    }
}
}
