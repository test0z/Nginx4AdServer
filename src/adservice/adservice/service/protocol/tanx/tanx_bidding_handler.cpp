//
// Created by guoze.lin on 16/5/3.
//

#include "tanx_bidding_handler.h"
#include "core/core_ip_manager.h"
#include "core/core_typetable.h"
#include "core/logic/show_query_task.h"
#include "logging.h"
#include "utility/utility.h"

extern std::string tanxDeviceId(const std::string & src);

namespace protocol {
namespace bidding {

    using namespace protocol::Tanx;
    using namespace adservice::utility;
    using namespace adservice::utility::serialize;
    using namespace adservice::utility::json;
    using namespace adservice::utility::userclient;
    using namespace adservice::utility::cypher;
    using namespace adservice::server;

#define AD_TX_CLICK_MACRO "%%CLICK_URL_PRE_ENC%%"
#define AD_TX_CLICK_UNENC_MACRO "%%CLICK_URL_PRE_UNENC%%"
#define AD_TX_PRICE_MACRO "%%SETTLE_PRICE%%"
#define TANX_COOKIEMAPPING_URL "http://cms.tanx.com/t.gif?tanx_nid=113168313&tanx_cm"

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

    std::string TanxBiddingHandler::tanxHtmlSnippet(const std::string & cookieMappingUrl, bool useHttps)
    {
        std::string bid = bidRequest.bid();
        // bool isMobile = bidRequest.has_mobile();
        const BidRequest_AdzInfo & adzInfo = bidRequest.adzinfo(0);
        std::string bannerSize = adzInfo.size();
        int width, height;
        extractSize(bannerSize, width, height);
        std::string cmImage;
        if (!cookieMappingUrl.empty()) {
            cmImage = cmImage + "<img src=\"" + cookieMappingUrl + "\"/>";
        }
        return generateHtmlSnippet(bid, width, height, NULL, cmImage.c_str(), useHttps);
    }

    std::string TanxBiddingHandler::generateHtmlSnippet(const std::string & bid, int width, int height,
                                                        const char * extShowBuf, const char * cookieMappingUrl,
                                                        bool useHttps)
    {
        char html[4096];
        url::URLHelper showUrlParam;
        getShowPara(showUrlParam, bid);
        showUrlParam.add(URL_IMP_OF, "3");
        showUrlParam.addMacro(URL_EXCHANGE_PRICE, AD_TX_PRICE_MACRO);
        snprintf(feedbackUrl, sizeof(feedbackUrl), "%s?%s", useHttps ? SNIPPET_SHOW_URL_HTTPS : SNIPPET_SHOW_URL,
                 showUrlParam.cipherParam().c_str());
        showUrlParam.add(URL_IMP_OF, "2");
        showUrlParam.removeMacro(URL_EXCHANGE_PRICE);
        showUrlParam.addMacro(URL_ADX_MACRO, AD_TX_CLICK_MACRO);
        int len = snprintf(html, sizeof(html), SNIPPET_IFRAME_SUPPORT_CM, width, height,
                           useHttps ? SNIPPET_SHOW_URL_HTTPS : SNIPPET_SHOW_URL, "", showUrlParam.cipherParam().c_str(),
                           cookieMappingUrl);
        return std::string(html, html + len);
    }

    bool TanxBiddingHandler::parseRequestData(const std::string & data)
    {
        bidRequest.Clear();
        return getProtoBufObject(bidRequest, data);
    }

    bool TanxBiddingHandler::fillSpecificLog(const AdSelectCondition & selectCondition,
                                             protocol::log::LogItem & logItem, bool isAccepted)
    {
        logItem.userAgent = bidRequest.user_agent();
        logItem.ipInfo.proxy = bidRequest.ip();
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
        if (bidRequest.content_categories().size() > 0) {
            const BidRequest_ContentCategory & category = bidRequest.content_categories(0);
            TypeTableManager & typeTableManager = TypeTableManager::getInstance();
            queryCondition.mttyContentType = typeTableManager.getContentType(ADX_TANX, std::to_string(category.id()));
        }
        if (bidRequest.has_mobile()) {
            const BidRequest_Mobile & mobile = bidRequest.mobile();
            const BidRequest_Mobile_Device & device = mobile.device();
            queryCondition.mobileDevice = getDeviceType(device.platform());
            queryCondition.flowType = SOLUTION_FLOWTYPE_MOBILE;
            queryCondition.adxid = ADX_TANX_MOBILE;
            if (mobile.has_is_app() && mobile.is_app()) {
                if (mobile.has_package_name()) {
                    queryCondition.adxpid = mobile.package_name();
                }
                if (mobile.app_categories().size() > 0) {
                    const BidRequest_Mobile_AppCategory & category = mobile.app_categories(0);
                    TypeTableManager & typeTableManager = TypeTableManager::getInstance();
                    queryCondition.mttyContentType
                        = typeTableManager.getContentType(ADX_TANX, std::to_string(category.id()));
                }
            }
            if (device.has_network()) {
                queryCondition.mobileNetwork = getNetWork(device.network());
            }
            cookieMappingKeyMobile(
                md5_encode(device.has_idfa() ? (queryCondition.idfa = tanxDeviceId(device.idfa())) : ""),
                md5_encode(device.has_imei() ? (queryCondition.imei = tanxDeviceId(device.imei())) : ""),
                md5_encode(device.has_android_id() ? (queryCondition.androidId = tanxDeviceId(device.android_id()))
                                                   : ""),
                md5_encode(device.has_mac() ? (queryCondition.mac = tanxDeviceId(device.mac())) : ""));
        } else {
            queryCondition.pcOS = getOSTypeFromUA(bidRequest.user_agent());
            queryCondition.pcBrowserStr = getBrowserTypeFromUA(bidRequest.user_agent());
            queryCondition.flowType = SOLUTION_FLOWTYPE_PC;
            cookieMappingKeyPC(ADX_TANX, bidRequest.has_tid() ? bidRequest.tid() : "");
        }
        queryCookieMapping(cmInfo.queryKV, queryCondition);

        if (!filterCb(this, queryConditions)) {
            return bidFailedReturn();
        }
        return isBidAccepted = true;
    }

    void TanxBiddingHandler::buildBidResult(const AdSelectCondition & queryCondition,
                                            const MT::common::SelectResult & result, int seq)
    {
        if (seq == 0) {
            bidResponse.Clear();
            bidResponse.set_version(bidRequest.version());
            bidResponse.set_bid(bidRequest.bid());
            bidResponse.clear_ads();
        }
        BidResponse_Ads * adResult = bidResponse.add_ads();
        const MT::common::Banner & banner = result.banner;
        std::string adxAdvIdStr = banner.adxAdvId;
        int adxAdvId = extractRealValue(adxAdvIdStr.data(), ADX_TANX);
        std::string adxIndustryTypeStr = banner.adxIndustryType;
        int adxIndustryType = extractRealValue(adxIndustryTypeStr.data(), ADX_TANX);
        const BidRequest_AdzInfo & adzInfo = bidRequest.adzinfo(seq);
        int maxCpmPrice = (int)result.bidPrice;

        adResult->set_max_cpm_price(maxCpmPrice);
        adResult->set_adzinfo_id(adzInfo.id());
        adResult->set_ad_bid_count_idx(0);
        adResult->add_creative_type(banner.bannerType);
        adResult->add_category(adxIndustryType);
        //缓存最终广告结果
        fillAdInfo(queryCondition, result, bidRequest.tid());

        std::string cookieMappingUrl = redoCookieMapping(ADX_TANX, TANX_COOKIEMAPPING_URL);
        bool isIOS = queryCondition.mobileDevice == SOLUTION_DEVICE_IPHONE
                     || queryCondition.mobileDevice == SOLUTION_DEVICE_IPAD;
        std::string strBannerJson = banner.json;
        urlHttp2HttpsIOS(isIOS, strBannerJson);
        cppcms::json::value bannerJson;
        parseJson(strBannerJson.c_str(), bannerJson);
        const cppcms::json::array & mtlsArray = bannerJson["mtls"].array();
        std::string destUrl = mtlsArray[0].get("p1", "");
        if (destUrl.empty()) {
            LOG_WARN << "destUrl should not be empty!!";
        }
        adResult->add_destination_url(destUrl);
        adResult->add_click_through_url(destUrl);
        adResult->set_creative_id(std::to_string(adInfo.bannerId));
        adResult->add_advertiser_ids(adxAdvId); // adx_advid
        adResult->set_html_snippet(tanxHtmlSnippet(cookieMappingUrl, isIOS));
        adResult->set_feedback_address(feedbackUrl);
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
