//
// Created by guoze.lin on 16/5/3.
//

#include "abstract_bidding_handler.h"
#include "utility/utility.h"
#include "logging.h"

namespace protocol {
namespace bidding {

    using namespace adservice::utility::url;
    using namespace adservice::utility::cypher;
    using namespace Logging;

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
                           "a=%s&b=%s&c=%ld&d=%ld&e=%ld&s=%s&o=%s&x=%d&r=%s&u=%s&tm=%ld&pt=%d&ep=%d",
                           adInfo.areaId.c_str(), encodePrice(adInfo.offerPrice).c_str(), adInfo.bannerId, adInfo.advId,
                           adInfo.sid, adInfo.adxpid.c_str(), adInfo.pid.c_str(), adInfo.adxid, bid.c_str(),
                           biddingFlowInfo.deviceIdBuf, time(NULL), adInfo.priceType, adInfo.ppid);
            if (len >= showBufSize) {
                LOG_WARN<<"In AbstractBiddingHandler::httpsnippet,showBufSize too small,actual:"<<len;
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
            int len = snprintf(clickParamBuf, clickBufSize,
                               "s=%s&o=%s&b=%s&x=%d&r=%s&d=%ld&e=%ld&ep=%d&c=%ld&f=%s&h=000&a=%s&u=%s&pt=%d&url=%s",
                               adInfo.adxpid.c_str(), adInfo.pid.c_str(), encodePrice(adInfo.offerPrice).c_str(),
                               adInfo.adxid, adInfo.imp_id.c_str(), adInfo.advId, adInfo.sid, adInfo.ppid,
                               adInfo.bannerId, encodedReferer.c_str(), adInfo.areaId.c_str(),
                               biddingFlowInfo.deviceIdBuf, adInfo.priceType, encodedLandingUrl.c_str());
            if (len >= clickBufSize) {
                LOG_WARN<<"in AbstractBiddingHandler::getClickPara,clickBufSize too small,actual:"<<len;
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
            LOG_WARN<<"generateHtmlSnippet buffer size not enough,needed:"<<len;
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
        //            getClickPara(bid,clickBuf,sizeof(clickBuf),)
        int len = snprintf(script, sizeof(script), SNIPPET_SCRIPT, width, height, scriptUrl, showBuf, clickBuf,
                           clickBuf, extParam, clickMacro);
        if (len >= (int)sizeof(script)) {
            LOG_WARN<<"generateScript buffer size not enough,needed:"<<len;
        }
        return std::string(script, script + len);
    }
}
}
