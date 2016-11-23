//
// Created by guoze.lin on 16/6/27.
//

#include "sohu_bidding_handler.h"
#include "core/core_ad_sizemap.h"
#include "core/core_ip_manager.h"
#include "logging.h"
#include "utility/utility.h"

namespace protocol {
namespace bidding {

    using namespace protocol::sohuadx;
    using namespace adservice::utility;
    using namespace adservice::utility::serialize;
    using namespace adservice::utility::json;
    using namespace adservice::utility::userclient;
    using namespace adservice::server;

    namespace {

        int getSohuDeviceType(const std::string & mobile)
        {
            if (!strcasecmp(mobile.data(), "iPhone")) {
                return SOLUTION_DEVICE_IPHONE;
            } else if (!strcasecmp(mobile.data(), "AndroidPhone")) {
                return SOLUTION_DEVICE_ANDROID;
            } else if (!strcasecmp(mobile.data(), "iPad")) {
                return SOLUTION_DEVICE_IPAD;
            } else if (!strcasecmp(mobile.data(), "AndroidPad")) {
                return SOLUTION_DEVICE_ANDROIDPAD;
            } else
                return SOLUTION_DEVICE_OTHER;
        }

        int getSohuNeworkType(const std::string & netType)
        {
            if (!strcasecmp(netType.data(), "WIFI")) {
                return SOLUTION_NETWORK_WIFI;
            } else if (!strcasecmp(netType.data(), "4G")) {
                return SOLUTION_NETWORK_4G;
            } else if (!strcasecmp(netType.data(), "3G")) {
                return SOLUTION_NETWORK_3G;
            } else if (!strcasecmp(netType.data(), "2G")) {
                return SOLUTION_NETWORK_2G;
            } else
                return SOLUTION_NETWORK_OTHER;
        }
    }

    std::string SohuBiddingHandler::getDisplayPara()
    {
        char showBuf[1024];
        getShowPara(bidRequest.bidid(), showBuf, sizeof(showBuf));
        const char * extShowBuf = "&of=3";
        strncat(showBuf, extShowBuf, strlen(extShowBuf));
        return std::string(showBuf);
    }

    std::string SohuBiddingHandler::getSohuClickPara(const std::string & url)
    {
        char clickBuf[1024];
        std::string ref = bidRequest.has_site() ? bidRequest.site().page() : "";
        getClickPara(bidRequest.bidid(), clickBuf, sizeof(clickBuf), ref, url);
        return std::string(clickBuf);
    }

    bool SohuBiddingHandler::parseRequestData(const std::string & data)
    {
        bidRequest.Clear();
        return getProtoBufObject(bidRequest, data);
    }

    boo SohuBiddingHandler::fillSpecificLog(const AdSelectCondition & selectCondition, protocol::log::LogItem & logItem,
                                            bool isAccepted)
    {
        logItem.userAgent = bidRequest.device().ua();
        logItem.ipInfo.proxy = selectCondition.ip;
        logItem.referer = bidRequest.has_site() ? (bidRequest.site().has_page() ? bidRequest.site().page() : "") : "";
        if (isAccepted) {
            if (bidRequest.has_device()) {
                const Request_Device & device = bidRequest.device();
                if (!(device.type() == "PC")) {
                    logItem.deviceInfo = device.DebugString();
                    adInfo.adxid = logItem.adInfo.adxid = ADX_SOHU_MOBILE;
                }
            }
        }
        return true;
    }

    bool SohuBiddingHandler::filter(const BiddingFilterCallback & filterCb)
    {
        if (bidRequest.istest() != 0) {
            return bidFailedReturn();
        }
        //从BID Request中获取请求的广告位信息,目前只取第一个
        if (bidRequest.impression_size() <= 0)
            return bidFailedReturn();
        const Request_Impression & adzInfo = bidRequest.impression(0);
        const std::string & pid = adzInfo.pid();
        std::vector<AdSelectCondition> queryConditions{ AdSelectCondition() };
        AdSelectCondition & queryCondition = queryConditions[0];
        queryCondition.adxid = ADX_SOHU_PC;
        queryCondition.adxpid = pid;
        queryCondition.basePrice = adzInfo.has_bidfloor() ? adzInfo.bidfloor() : 0;
        if (bidRequest.has_device()) {
            auto & device = bidRequest.device();
            queryCondition.ip = device.has_ip() ? device.ip() : "";
            if (strcasecmp(device.type().data(), "PC")) { // mobile
                queryCondition.mobileDevice = getSohuDeviceType(device.mobiletype());
                queryCondition.flowType = SOLUTION_FLOWTYPE_MOBILE;
                queryCondition.adxid = ADX_SOHU_MOBILE;
                queryCondition.mobileNetwork = getSohuNeworkType(device.nettype());
            } else { // pc
                queryCondition.pcOS = getOSTypeFromUA(device.ua());
                queryCondition.flowType = SOLUTION_FLOWTYPE_PC;
            }
        }
        if (adzInfo.has_banner()) {
            const Request_Impression_Banner & banner = adzInfo.banner();
            queryCondition.width = banner.width();
            queryCondition.height = banner.height();
            if (queryCondition.flowType == SOLUTION_FLOWTYPE_PC) {
                queryCondition.bannerType = BANNER_TYPE_PIC;
            } else {
                queryCondition.bannerType = BANNER_TYPE_MOBILE;
            }
        } else if (adzInfo.has_video()) {
            const Request_Impression_Video & video = adzInfo.video();
            queryCondition.width = video.width();
            queryCondition.height = video.height();
            queryCondition.bannerType = BANNER_TYPE_VIDEO;
        }
        if (queryCondition.flowType == SOLUTION_FLOWTYPE_MOBILE) {
            const adservice::utility::AdSizeMap & sizeMap = adservice::utility::AdSizeMap::getInstance();
            auto size = sizeMap.get({ queryCondition.width, queryCondition.height });
            queryCondition.width = size.first;
            queryCondition.height = size.second;
        }
        if (adzInfo.has_ispreferreddeals() && adzInfo.ispreferreddeals()) {
            const std::string & dealId = adzInfo.campaignid();
            queryCondition.dealId = std::string(",") + dealId + ",";
        }
        if (!filterCb(this, queryConditions)) {
            return bidFailedReturn();
        }
        return isBidAccepted = true;
    }

    void SohuBiddingHandler::buildBidResult(const AdSelectCondition & queryCondition,
                                            const MT::common::SelectResult & result, int seq)
    {
        if (seq == 0) {
            bidResponse.Clear();
            bidResponse.set_version(bidRequest.version());
            bidResponse.set_bidid(bidRequest.bidid());
            bidResponse.clear_seatbid();
        }
        Response_SeatBid * seatBid = bidResponse.add_seatbid();
        seatBid->set_idx(seq);
        Response_Bid * adResult = seatBid->add_bid();
        const MT::common::Solution & finalSolution = result.solution;
        const MT::common::ADPlace & adplace = result.adplace;
        const MT::common::Banner & banner = result.banner;
        int advId = finalSolution.advId;
        const Request_Impression & adzInfo = bidRequest.impression(seq);
        int maxCpmPrice = result.bidPrice;
        adResult->set_price(maxCpmPrice);

        //缓存最终广告结果
        adInfo.pid = queryCondition.mttyPid;
        adInfo.adxpid = queryCondition.adxpid;
        adInfo.sid = finalSolution.sId;
        adInfo.advId = advId;
        adInfo.adxid = queryCondition.adxid;
        adInfo.adxuid = bidRequest.has_user() ? bidRequest.user().suid() : "";
        adInfo.bannerId = banner.bId;
        adInfo.cid = adplace.cId;
        adInfo.mid = adplace.mId;
        adInfo.cpid = adInfo.advId;
        adInfo.offerPrice = result.feePrice;
        adInfo.priceType = finalSolution.priceType;
        adInfo.ppid = result.ppid;
        adInfo.bidSize = makeBidSize(banner.width, banner.height);
        const std::string & userIp = bidRequest.device().ip();
        IpManager & ipManager = IpManager::getInstance();
        adInfo.areaId = ipManager.getAreaCodeStrByIp(userIp.data());

        char pjson[2048] = { '\0' };
        std::string strBannerJson = banner.json;
        strncat(pjson, strBannerJson.data(), sizeof(pjson));
        // tripslash2(pjson);
        cppcms::json::value bannerJson;
        parseJson(pjson, bannerJson);
        const cppcms::json::array & mtlsArray = bannerJson["mtls"].array();
        std::string materialUrl;
        std::string landingUrl;
        if (banner.bannerType == BANNER_TYPE_PRIMITIVE) {
            materialUrl = mtlsArray[0].get("p6", "");
            landingUrl = mtlsArray[0].get("p9", "");
        } else {
            materialUrl = mtlsArray[0].get("p0", "");
            landingUrl = mtlsArray[0].get("p1", "");
        }
        adResult->set_adurl(materialUrl);
        std::string displayParam = getDisplayPara();
        std::string clickParam = getSohuClickPara(landingUrl);
        if (!queryCondition.dealId.empty()) {
            displayParam += "&" URL_DEAL_ID "=";
            displayParam += adzInfo.campaignid();
            clickParam += "&" URL_DEAL_ID "=";
            clickParam += adzInfo.campaignid();
        }
        adResult->set_displaypara(displayParam);
        adResult->set_clickpara(clickParam);
    }

    void SohuBiddingHandler::match(adservice::utility::HttpResponse & response)
    {
        std::string result;
        if (!writeProtoBufObject(bidResponse, &result)) {
            LOG_ERROR << "failed to write protobuf object in TanxBiddingHandler::match";
            reject(response);
            return;
        }
        response.status(200);
        response.set_content_header("application/x-protobuf");
        response.set_body(result);
    }

    void SohuBiddingHandler::reject(adservice::utility::HttpResponse & response)
    {
        bidResponse.Clear();
        bidResponse.set_version(bidRequest.version());
        bidResponse.set_bidid(bidRequest.bidid());
        std::string result;
        if (!writeProtoBufObject(bidResponse, &result)) {
            LOG_ERROR << "failed to write protobuf object in TanxBiddingHandler::reject";
            return;
        }
        response.status(200);
        response.set_content_header("application/x-protobuf");
        response.set_body(result);
    }
}
}
