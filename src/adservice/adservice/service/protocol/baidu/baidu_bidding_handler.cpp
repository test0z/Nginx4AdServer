//
// Created by guoze.lin on 16/5/3.
//

#include "baidu_bidding_handler.h"
#include "core/core_ip_manager.h"
#include "utility/utility.h"
#include "logging.h"

namespace protocol {
namespace bidding {

    using namespace protocol::Baidu;
    using namespace adservice::utility::serialize;
    using namespace adservice::server;
    using namespace Logging;

#define AD_BD_CLICK_MACRO "%%CLICK_URL_ESC%%"
#define AD_BD_PRICE_MACRO "%%PRICE%%"
#define AD_COOKIEMAPPING_BAIDU ""

    inline int max(const int & a, const int & b)
    {
        return a > b ? a : b;
    }

    bool BaiduBiddingHandler::parseRequestData(const std::string & data)
    {
        bidRequest.Clear();
        return getProtoBufObject(bidRequest, data);
    }

    std::string BaiduBiddingHandler::baiduHtmlSnippet()
    {
        char extShowBuf[1024];
        const std::string & bid = bidRequest.id();
        const BidRequest_AdSlot & adSlot = bidRequest.adslot(0);
        int width = adSlot.width();
        int height = adSlot.height();
        size_t len
            = (size_t)snprintf(extShowBuf, sizeof(extShowBuf), "p=%s&l=%s&", AD_BD_PRICE_MACRO, AD_BD_CLICK_MACRO);
        if (len >= sizeof(extShowBuf)) {
            LOG_WARN<<"BaiduBiddingHandler::baiduHtmlSnippet,extShowBuf buffer size not enough,needed:"<<
                                 len;
        }
        return generateHtmlSnippet(bid, width, height, extShowBuf);
    }

    std::string BaiduBiddingHandler::baiduHtmlScript()
    {
        char extParam[1024];
        const std::string & bid = bidRequest.id();
        const BidRequest_AdSlot & adSlot = bidRequest.adslot(0);
        int width = adSlot.width();
        int height = adSlot.height();
        snprintf(extParam, sizeof(extParam), "p=%s", AD_BD_PRICE_MACRO);
        const std::string sHtml = "";
        return generateScript(bid, width, height, sHtml.data(), AD_BD_CLICK_MACRO, extParam);
    }

    bool BaiduBiddingHandler::fillLogItem(protocol::log::LogItem & logItem)
    {
        logItem.reqStatus = 200;
        logItem.userAgent = bidRequest.user_agent();
        logItem.ipInfo.proxy = bidRequest.ip();
        logItem.adInfo.adxid = ADX_BAIDU;
        if (isBidAccepted) {
            if (bidRequest.has_mobile()) {
                const BidRequest_Mobile & mobile = bidRequest.mobile();
                logItem.deviceInfo = mobile.DebugString();
            }
            logItem.adInfo.sid = adInfo.sid;
            logItem.adInfo.advId = adInfo.advId;
            logItem.adInfo.adxid = adInfo.adxid;
            logItem.adInfo.adxpid = adInfo.adxpid;
            logItem.adInfo.adxuid = adInfo.adxuid;
            logItem.adInfo.bannerId = adInfo.bannerId;
            logItem.adInfo.cid = adInfo.cid;
            logItem.adInfo.mid = adInfo.mid;
            logItem.adInfo.cpid = adInfo.cpid;
            logItem.adInfo.offerPrice = adInfo.offerPrice;
            logItem.adInfo.priceType = adInfo.priceType;
            logItem.adInfo.ppid = adInfo.ppid;
            adservice::utility::url::extractAreaInfo(adInfo.areaId.data(), logItem.geoInfo.country,
                                                     logItem.geoInfo.province, logItem.geoInfo.city);
            logItem.adInfo.bidSize = adInfo.bidSize;
            logItem.referer = bidRequest.has_referer() ? bidRequest.referer() : "";
        }
        return true;
    }

    bool BaiduBiddingHandler::filter(const BiddingFilterCallback & filterCb)
    {
        if (bidRequest.is_ping() || bidRequest.is_test()) {
            return bidFailedReturn();
        }
        //从BID Request中获取请求的广告位信息,目前只取第一个
        const BidRequest_AdSlot & adSlot = bidRequest.adslot(0);
        long pid = adSlot.ad_block_key();
        AdSelectCondition queryCondition;
        queryCondition.adxid = ADX_BAIDU;
        queryCondition.adxpid = std::to_string(pid);
        queryCondition.ip = bidRequest.ip();
        if (!filterCb(this, queryCondition)) {
            return bidFailedReturn();
        }
        return isBidAccepted = true;
    }

    void BaiduBiddingHandler::buildBidResult(const AdSelectCondition & queryCondition, const AdSelectResult & result)
    {
        bidResponse.Clear();
        bidResponse.set_id(bidRequest.id());
        bidResponse.clear_ad();
        BidResponse_Ad * adResult = bidResponse.add_ad();
        const AdSolution & finalSolution = result.solution;
        const AdAdplace & adplace = result.adplace;
        const AdBanner & banner = result.banner;
        int advId = finalSolution.advId;
        const BidRequest_AdSlot & adSlot = bidRequest.adslot(0);
        int maxCpmPrice = max(result.bidPrice, adSlot.minimum_cpm());
        adResult->set_max_cpm(maxCpmPrice);
        adResult->set_advertiser_id(advId);
        adResult->set_creative_id(banner.bannerId);
        adResult->set_height(banner.height);
        adResult->set_width(banner.width);
        adResult->set_sequence_id(adSlot.sequence_id());
        //缓存最终广告结果
        adInfo.pid = adplace.adplaceId;
        adInfo.advId = advId;
        adInfo.sid = finalSolution.solutionId;
        adInfo.adxid = ADX_BAIDU;
        adInfo.adxpid = adplace.adxPid;
        adInfo.adxuid = bidRequest.baidu_user_id();
        adInfo.bannerId = banner.bannerId;
        adInfo.cid = adplace.cid;
        adInfo.mid = adplace.mid;
        adInfo.cpid = adInfo.advId;
        adInfo.offerPrice = maxCpmPrice;
        adInfo.priceType = finalSolution.priceType;
        adInfo.ppid = result.ppid;
        adInfo.bidSize = makeBidSize(banner.width, banner.height);
        const std::string & userIp = bidRequest.ip();
        IpManager & ipManager = IpManager::getInstance();
        adInfo.areaId = ipManager.getAreaCodeStrByIp(userIp.data());
        adResult->set_html_snippet(baiduHtmlSnippet());
    }

    void BaiduBiddingHandler::match(adservice::utility::HttpResponse & response)
    {
        std::string result;
        if (!writeProtoBufObject(bidResponse, &result)) {
            LOG_WARN<<"failed to write protobuf object in BaiduBiddingHandler::match";
            reject(response);
            return;
        }
        response.status(200);
        response.set_body(result);
    }

    void BaiduBiddingHandler::reject(adservice::utility::HttpResponse & response)
    {
        bidResponse.Clear();
        bidResponse.set_id(bidRequest.id());
        std::string result;
        if (!writeProtoBufObject(bidResponse, &result)) {
            LOG_WARN<<"failed to write protobuf object in BaiduBiddingHandler::reject";
            return;
        }
        response.status(200);
        response.set_body(result);
    }
}
}
