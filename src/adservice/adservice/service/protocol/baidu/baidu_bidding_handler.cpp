//
// Created by guoze.lin on 16/5/3.
//

#include "baidu_bidding_handler.h"
#include "core/core_ip_manager.h"
#include "logging.h"
#include "utility/utility.h"

namespace protocol {
namespace bidding {

    using namespace protocol::Baidu;
    using namespace adservice::utility::serialize;
    using namespace adservice::server;

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
            LOG_WARN << "BaiduBiddingHandler::baiduHtmlSnippet,extShowBuf buffer size not enough,needed:" << len;
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

    bool BaiduBiddingHandler::fillSpecificLog(const AdSelectCondition & selectCondition,
                                              protocol::log::LogItem & logItem, bool isAccepted)
    {
        logItem.userAgent = bidRequest.user_agent();
        logItem.ipInfo.proxy = selectCondition.ip;
        logItem.referer = bidRequest.has_referer() ? bidRequest.referer() : "";
        if (isAccepted) {
            if (bidRequest.has_mobile()) {
                const BidRequest_Mobile & mobile = bidRequest.mobile();
                logItem.deviceInfo = mobile.DebugString();
            }
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
        std::vector<AdSelectCondition> queryConditions{ AdSelectCondition() };
        AdSelectCondition & queryCondition = queryConditions[0];
        queryCondition.adxid = ADX_BAIDU;
        queryCondition.adxpid = std::to_string(pid);
        queryCondition.ip = bidRequest.ip();
        if (!filterCb(this, queryConditions)) {
            adInfo.bidSize = makeBidSize(adSlot.width(), adSlot.height());
            return bidFailedReturn();
        }
        return isBidAccepted = true;
    }

    void BaiduBiddingHandler::buildBidResult(const AdSelectCondition & queryCondition,
                                             const MT::common::SelectResult & result, int seq)
    {
        if (seq == 0) {
            bidResponse.Clear();
            bidResponse.set_id(bidRequest.id());
            bidResponse.clear_ad();
        }
        BidResponse_Ad * adResult = bidResponse.add_ad();
        const MT::common::Solution & finalSolution = result.solution;
        const MT::common::ADPlace & adplace = result.adplace;
        const MT::common::Banner & banner = result.banner;
        int advId = finalSolution.advId;
        const BidRequest_AdSlot & adSlot = bidRequest.adslot(seq);
        int maxCpmPrice = max(result.bidPrice, adSlot.minimum_cpm());
        adResult->set_max_cpm(maxCpmPrice);
        adResult->set_advertiser_id(advId);
        adResult->set_creative_id(banner.bId);
        adResult->set_height(banner.height);
        adResult->set_width(banner.width);
        adResult->set_sequence_id(adSlot.sequence_id());
        //缓存最终广告结果
        adInfo.pid = adplace.pId;
        adInfo.advId = advId;
        adInfo.sid = finalSolution.sId;
        adInfo.adxid = ADX_BAIDU;
        adInfo.adxpid = adplace.adxPId;
        adInfo.adxuid = bidRequest.baidu_user_id();
        adInfo.bannerId = banner.bId;
        adInfo.cid = adplace.cId;
        adInfo.mid = adplace.mId;
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
            LOG_WARN << "failed to write protobuf object in BaiduBiddingHandler::match";
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
            LOG_WARN << "failed to write protobuf object in BaiduBiddingHandler::reject";
            return;
        }
        response.status(200);
        response.set_body(result);
    }
}
}
