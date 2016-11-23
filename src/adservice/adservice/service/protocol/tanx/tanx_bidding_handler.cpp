//
// Created by guoze.lin on 16/5/3.
//

#include "tanx_bidding_handler.h"
#include "core/core_ip_manager.h"
#include "core/logic/show_query_task.h"
#include "logging.h"
#include "utility/utility.h"

namespace protocol {
namespace bidding {

    using namespace protocol::Tanx;
    using namespace adservice::utility;
    using namespace adservice::utility::serialize;
    using namespace adservice::utility::json;
    using namespace adservice::utility::userclient;
    using namespace adservice::server;

#define AD_TX_CLICK_MACRO "%%CLICK_URL_PRE_ENC%%"
#define AD_TX_CLICK_UNENC_MACRO "%%CLICK_URL_PRE_UNENC%%"
#define AD_TX_PRICE_MACRO "%%SETTLE_PRICE%%"
#define AD_COOKIEMAPPING_TANX ""

    inline int max(const int & a, const int & b)
    {
        return a > b ? a : b;
    }

    static int getDeviceType(const std::string & deviceInfo)
    {
        if (deviceInfo.find("iphone") != std::string::npos) {
            return SOLUTION_DEVICE_IPHONE;
        } else if (deviceInfo.find("android") != std::string::npos) {
            return SOLUTION_DEVICE_ANDROID;
        } else if (deviceInfo.find("ipad") != std::string::npos) {
            return SOLUTION_DEVICE_IPAD;
        } else
            return SOLUTION_DEVICE_OTHER;
    }

    static int getNetWork(int network)
    {
        switch (network) {
        case 0:
            return SOLUTION_NETWORK_ALL;
        case 1:
            return SOLUTION_NETWORK_WIFI;
        case 2:
            return SOLUTION_NETWORK_2G;
        case 3:
            return SOLUTION_NETWORK_3G;
        case 4:
            return SOLUTION_NETWORK_4G;
        default:
            return SOLUTION_NETWORK_ALL;
        }
    }

    std::string TanxBiddingHandler::tanxHtmlSnippet()
    {
        std::string bid = bidRequest.bid();
        // bool isMobile = bidRequest.has_mobile();
        const BidRequest_AdzInfo & adzInfo = bidRequest.adzinfo(0);
        std::string bannerSize = adzInfo.size();
        int width, height;
        extractSize(bannerSize, width, height);
        return generateHtmlSnippet(bid, width, height, NULL);
    }

    std::string TanxBiddingHandler::generateHtmlSnippet(const std::string & bid, int width, int height,
                                                        const char * extShowBuf, const char * cookieMappingUrl)
    {
        char showBuf[2048];
        // char clickBuf[2048];
        char html[4096];
        getShowPara(bid, showBuf, sizeof(showBuf));
        size_t len = (size_t)snprintf(feedbackUrl, sizeof(feedbackUrl), "%s?%s%s&of=3", SNIPPET_SHOW_URL,
                                      "p=" AD_TX_PRICE_MACRO "&", showBuf);
        if (len >= sizeof(feedbackUrl)) {
            LOG_WARN << "feedbackUrl buffer size not enough,needed:" << len;
        }
        strncat(showBuf, "&of=2", 5);
        len = (size_t)snprintf(html, sizeof(html), SNIPPET_IFRAME, width, height, SNIPPET_SHOW_URL,
                               "l=" AD_TX_CLICK_MACRO "&", showBuf, cookieMappingUrl);
        if (len >= sizeof(html)) {
            LOG_WARN << "generateHtmlSnippet buffer size not enough,needed:" << len;
        }
        return std::string(html, html + len);
    }

    bool TanxBiddingHandler::parseRequestData(const std::string & data)
    {
        bidRequest.Clear();
        return getProtoBufObject(bidRequest, data);
    }

    bool TanxBiddingHandler::fillLogItem(protocol::log::LogItem & logItem, bool isAccepted)
    {
        logItem.reqStatus = 200;
        logItem.userAgent = bidRequest.user_agent();
        logItem.ipInfo.proxy = bidRequest.ip();
        logItem.adInfo.adxid = adInfo.adxid;
        logItem.adInfo.adxpid = adInfo.adxpid;
        logItem.referer = bidRequest.has_url() ? bidRequest.url() : "";
        if (isAccepted) {
            if (bidRequest.has_mobile()) {
                const BidRequest_Mobile & mobile = bidRequest.mobile();
                if (mobile.has_device()) {
                    const BidRequest_Mobile_Device & device = mobile.device();
                    logItem.deviceInfo = device.DebugString();
                    adInfo.adxid = logItem.adInfo.adxid = ADX_TANX_MOBILE;
                }
            }
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
            url::extractAreaInfo(adInfo.areaId.data(), logItem.geoInfo.country, logItem.geoInfo.province,
                                 logItem.geoInfo.city);
            logItem.adInfo.bidSize = adInfo.bidSize;
            logItem.adInfo.orderId = adInfo.orderId;
        } else {
            logItem.adInfo.pid = adInfo.pid;
            logItem.adInfo.bidSize = adInfo.bidSize;
        }
        return true;
    }

    bool TanxBiddingHandler::filter(const BiddingFilterCallback & filterCb)
    {
        if (bidRequest.is_ping() != 0) {
            return bidFailedReturn();
        }

        //从BID Request中获取请求的广告位信息,目前只取第一个
        if (bidRequest.adzinfo_size() <= 0)
            return bidFailedReturn();
        const BidRequest_AdzInfo & adzInfo = bidRequest.adzinfo(0);
        const std::string & pid = adzInfo.pid();
        std::vector<AdSelectCondition> queryConditions{ AdSelectCondition() };
        AdSelectCondition & queryCondition = queryConditions[0];
        queryCondition.adxid = ADX_TANX;
        queryCondition.adxpid = pid;
        queryCondition.ip = bidRequest.ip();
        queryCondition.basePrice = adzInfo.has_min_cpm_price() ? adzInfo.min_cpm_price() : 0;
        extractSize(adzInfo.size(), queryCondition.width, queryCondition.height);
        if (bidRequest.has_mobile()) {
            const BidRequest_Mobile & mobile = bidRequest.mobile();
            const BidRequest_Mobile_Device & device = mobile.device();
            queryCondition.mobileDevice = getDeviceType(device.platform());
            queryCondition.flowType = SOLUTION_FLOWTYPE_MOBILE;
            queryCondition.adxid = ADX_TANX_MOBILE;
            if (mobile.has_is_app() && mobile.is_app() && mobile.has_package_name()) {
                queryCondition.adxpid = mobile.package_name();
            }
            std::string deviceId = device.idfa().empty() ? device.android_id() : device.idfa();
            strncpy(biddingFlowInfo.deviceIdBuf, deviceId.data(), deviceId.size());
            if (device.has_network()) {
                queryCondition.mobileNetwork = getNetWork(device.network());
            }
        } else {
            queryCondition.pcOS = getOSTypeFromUA(bidRequest.user_agent());
            queryCondition.pcBrowserStr = getBrowserTypeFromUA(bidRequest.user_agent());
            queryCondition.flowType = SOLUTION_FLOWTYPE_PC;
        }
        if (!filterCb(this, queryConditions)) {
            adInfo.pid = std::to_string(queryCondition.mttyPid);
            adInfo.adxpid = queryCondition.adxpid;
            adInfo.adxid = queryCondition.adxid;
            adInfo.bidSize = makeBidSize(queryCondition.width, queryCondition.height);
            return bidFailedReturn();
        }
        return isBidAccepted = true;
    }

    void TanxBiddingHandler::buildBidResult(const AdSelectCondition & queryCondition,
                                            const MT::common::SelectResult & result)
    {
        bidResponse.Clear();
        bidResponse.set_version(bidRequest.version());
        bidResponse.set_bid(bidRequest.bid());
        bidResponse.clear_ads();
        BidResponse_Ads * adResult = bidResponse.add_ads();
        const MT::common::Banner & banner = result.banner;
        std::string adxAdvIdStr = banner.adxAdvId;
        int adxAdvId = extractRealValue(adxAdvIdStr.data(), ADX_TANX);
        std::string adxIndustryTypeStr = banner.adxIndustryType;
        int adxIndustryType = extractRealValue(adxIndustryTypeStr.data(), ADX_TANX);
        const BidRequest_AdzInfo & adzInfo = bidRequest.adzinfo(0);
        int maxCpmPrice = (int)result.bidPrice;

        adResult->set_max_cpm_price(maxCpmPrice);
        adResult->set_adzinfo_id(adzInfo.id());
        adResult->set_ad_bid_count_idx(0);
        adResult->add_creative_type(banner.bannerType);
        adResult->add_category(adxIndustryType);
        //缓存最终广告结果
        fillAdInfo(queryCondition, result, bidRequest.tid());

        char pjson[2048] = { '\0' };
        std::string strBannerJson = banner.json;
        strncat(pjson, strBannerJson.data(), sizeof(pjson));
        // tripslash2(pjson);
        cppcms::json::value bannerJson;
        parseJson(pjson, bannerJson);
        const cppcms::json::array & mtlsArray = bannerJson["mtls"].array();
        std::string destUrl = mtlsArray[0].get("p1", "");
        if (destUrl.empty()) {
            LOG_WARN << "destUrl should not be empty!!";
        }
        adResult->add_destination_url(destUrl);
        adResult->add_click_through_url(destUrl);
        adResult->set_creative_id(std::to_string(adInfo.bannerId));
        adResult->add_advertiser_ids(adxAdvId); // adx_advid
        adResult->set_html_snippet(tanxHtmlSnippet());
        adResult->set_feedback_address(feedbackUrl);
        //        if (adInfo.adxid == ADX_TANX_MOBILE) {
        //            adResult->set_html_snippet(tanxHtmlSnippet());
        //            adResult->set_feedback_address(feedbackUrl);
        //        } else {
        //            bannerJson["advid"] = finalSolution.advId;
        //            bannerJson["adxpid"] = adInfo.adxpid;
        //            bannerJson["arid"] = adInfo.areaId;
        //            bannerJson["gpid"] = finalSolution.sId;
        //            bannerJson["pid"] = adInfo.pid;
        //            bannerJson["ppid"] = adInfo.ppid;
        //            bannerJson["price"] = adInfo.offerPrice;
        //            bannerJson["pricetype"] = adInfo.priceType;
        //            bannerJson["unid"] = adInfo.adxid;
        //            bannerJson["of"] = "0";
        //            bannerJson["width"] = banner.width;
        //            bannerJson["height"] = banner.height;
        //            bannerJson["xcurl"] = AD_TX_CLICK_UNENC_MACRO;
        //            std::string mtadInfoStr = adservice::utility::json::toJson(bannerJson);
        //            char admBuffer[4096];
        //            snprintf(admBuffer, sizeof(admBuffer), adservice::corelogic::HandleShowQueryTask::showAdxTemplate,
        //                     mtadInfoStr.data());
        //            adResult->set_html_snippet(admBuffer);
        //            char showBuf[2048];
        //            getShowPara(bidRequest.bid(), showBuf, sizeof(showBuf));
        //            snprintf(feedbackUrl, sizeof(feedbackUrl), "%s?%s%s&of=3", SNIPPET_SHOW_URL, "p="
        //            AD_TX_PRICE_MACRO "&",
        //                     showBuf);
        //            adResult->set_feedback_address(feedbackUrl);
        //        }
    }

    void TanxBiddingHandler::match(adservice::utility::HttpResponse & response)
    {
        std::string result;
        if (!writeProtoBufObject(bidResponse, &result)) {
            LOG_ERROR << "failed to write protobuf object in TanxBiddingHandler::match";
            reject(response);
            return;
        }
        response.status(200);
        response.set_content_header("application/octet-stream");
        response.set_body(result);
    }

    void TanxBiddingHandler::reject(adservice::utility::HttpResponse & response)
    {
        bidResponse.Clear();
        bidResponse.set_version(bidRequest.version());
        bidResponse.set_bid(bidRequest.bid());
        std::string result;
        if (!writeProtoBufObject(bidResponse, &result)) {
            LOG_ERROR << "failed to write protobuf object in TanxBiddingHandler::reject";
            return;
        }
        response.status(200);
        response.set_content_header("application/octet-stream");
        response.set_body(result);
    }
}
}
