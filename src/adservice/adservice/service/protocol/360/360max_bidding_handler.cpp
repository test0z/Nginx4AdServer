#include "360max_bidding_handler.h"
#include "core/core_ip_manager.h"
#include "core/core_typetable.h"
#include "core/logic/show_query_task.h"
#include "logging.h"
#include "utility/utility.h"

namespace protocol {
namespace bidding {

    using namespace protocol::MAX;
    using namespace adservice::utility;
    using namespace adservice::utility::serialize;
    using namespace adservice::utility::json;
    using namespace adservice::utility::userclient;
    using namespace adservice::utility::cypher;
    using namespace adservice::server;

#define AD_MAX_CLICK_MACRO "%%CLICK_URL_ESC%%"
#define AD_MAX_CLICK_UNENC_MACRO "%%CLICK_URL_UNESC%%"
#define AD_MAX_PRICE_MACRO "%%WIN_PRICE%%"
#define MAX_COOKIEMAPPING_URL "http://ck.adserver.com/mvdid=1"

    inline int max(const int & a, const int & b)
    {
        return a > b ? a : b;
    }

    static int getDeviceTypeByOS(const std::string & os)
    {
        if (os.find("ios") != std::string::npos) {
            return SOLUTION_DEVICE_IPHONE;
        } else if (os.find("android") != std::string::npos) {
            return SOLUTION_DEVICE_ANDROID;
        } else {
            return SOLUTION_DEVICE_OTHER;
        }
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

    std::string JuxiaoMaxBiddingHandler::juxiaoHtmlSnippet(const std::string & cookieMappingUrl, bool useHttps)
    {
        std::string bid = bidRequest.bid();
        // bool isMobile = bidRequest.has_mobile();
        const BidRequest_AdSlot & adzInfo = bidRequest.adslot(0);
        std::string cmImage;
        if (!cookieMappingUrl.empty()) {
            cmImage = cmImage + "<img src=\"" + cookieMappingUrl + "\"/>";
        }
        return generateHtmlSnippet(bid, adzInfo.width(), adzInfo.height(), NULL, cmImage.c_str(), useHttps);
    }

    std::string JuxiaoMaxBiddingHandler::generateHtmlSnippet(const std::string & bid, int width, int height,
                                                             const char * extShowBuf, const char * cookieMappingUrl,
                                                             bool useHttps)
    {
        char html[4096];
        url::URLHelper showUrlParam;
        getShowPara(showUrlParam, bid);
        showUrlParam.add(URL_IMP_OF, "3");
        showUrlParam.addMacro(URL_EXCHANGE_PRICE, AD_MAX_PRICE_MACRO);
        snprintf(feedbackUrl, sizeof(feedbackUrl), "%s?%s", useHttps ? SNIPPET_SHOW_URL_HTTPS : SNIPPET_SHOW_URL,
                 showUrlParam.cipherParam().c_str());
        showUrlParam.add(URL_IMP_OF, "2");
        showUrlParam.removeMacro(URL_EXCHANGE_PRICE);
        showUrlParam.addMacro(URL_ADX_MACRO, AD_MAX_CLICK_MACRO);
        int len = snprintf(html, sizeof(html), SNIPPET_IFRAME_SUPPORT_CM, width, height,
                           useHttps ? SNIPPET_SHOW_URL_HTTPS : SNIPPET_SHOW_URL, "", showUrlParam.cipherParam().c_str(),
                           cookieMappingUrl);
        return std::string(html, html + len);
    }

    bool JuxiaoMaxBiddingHandler::parseRequestData(const std::string & data)
    {
        bidRequest.Clear();
        return getProtoBufObject(bidRequest, data);
    }

    bool JuxiaoMaxBiddingHandler::fillSpecificLog(const AdSelectCondition & selectCondition,
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
                    adInfo.adxid = logItem.adInfo.adxid = ADX_360_MAX_MOBILE;
                }
            }
        }
        return true;
    }

    bool JuxiaoMaxBiddingHandler::filter(const BiddingFilterCallback & filterCb)
    {
        //从BID Request中获取请求的广告位信息,目前只取第一个
        if (bidRequest.adslot_size() <= 0)
            return bidFailedReturn();
        const BidRequest_AdSlot & adzInfo = bidRequest.adslot(0);
        const std::string & pid = adzInfo.pid();
        std::vector<AdSelectCondition> queryConditions{ AdSelectCondition() };
        AdSelectCondition & queryCondition = queryConditions[0];
        queryCondition.adxid = ADX_360_MAX_PC;
        queryCondition.adxpid = pid;
        queryCondition.ip = bidRequest.ip();
        queryCondition.basePrice = (adzInfo.has_min_cpm_price() ? adzInfo.min_cpm_price() : 0) / 1000;
        queryCondition.width = adzInfo.width();
        queryCondition.height = adzInfo.height();
        if (bidRequest.content_category_size() > 0) {
            const BidRequest_ContentCategory & category = bidRequest.content_category(0);
            TypeTableManager & typeTableManager = TypeTableManager::getInstance();
            queryCondition.mttyContentType
                = typeTableManager.getContentType(ADX_360_MAX_PC, std::to_string(category.id()));
        }
        if (bidRequest.has_mobile()) {
            const BidRequest_Mobile & mobile = bidRequest.mobile();
            const BidRequest_Mobile_Device & device = mobile.device();
            queryCondition.mobileDevice = getDeviceTypeByOS(device.os());
            queryCondition.flowType = SOLUTION_FLOWTYPE_MOBILE;
            queryCondition.adxid = ADX_360_MAX_MOBILE;
            if (device.has_network()) {
                queryCondition.mobileNetwork = getNetWork(device.network());
            }
            if (mobile.has_is_app() && mobile.is_app()) { // app
                if (mobile.has_package_name()) {
                    queryCondition.adxpid = mobile.package_name();
                }
                if (mobile.app_category_size() > 0) {
                    TypeTableManager & typeTableManager = TypeTableManager::getInstance();
                    queryCondition.mttyContentType
                        = typeTableManager.getContentType(ADX_TANX, std::to_string(mobile.app_category(0)));
                }
                cookieMappingKeyMobile(
                    md5_encode(device.has_idfa() ? (queryCondition.idfa = device.idfa()) : ""),
                    md5_encode(device.has_imei() ? (queryCondition.imei = device.imei()) : ""),
                    md5_encode(device.has_android_id() ? (queryCondition.androidId = device.android_id()) : ""),
                    md5_encode(device.has_mac() ? (queryCondition.mac = device.mac()) : ""));
            } else { // wap
                cookieMappingKeyWap(ADX_360_MAX_MOBILE, bidRequest.has_mv_user_id() ? bidRequest.mv_user_id() : "");
            }
        } else {
            queryCondition.pcOS = getOSTypeFromUA(bidRequest.user_agent());
            queryCondition.pcBrowserStr = getBrowserTypeFromUA(bidRequest.user_agent());
            queryCondition.flowType = SOLUTION_FLOWTYPE_PC;
            cookieMappingKeyPC(ADX_360_MAX_PC, bidRequest.has_mv_user_id() ? bidRequest.mv_user_id() : "");
        }
        queryCookieMapping(cmInfo.queryKV, queryCondition);

        if (!filterCb(this, queryConditions)) {
            return bidFailedReturn();
        }
        return isBidAccepted = true;
    }

    void JuxiaoMaxBiddingHandler::buildBidResult(const AdSelectCondition & queryCondition,
                                                 const MT::common::SelectResult & result, int seq)
    {
        if (seq == 0) {
            bidResponse.Clear();
            bidResponse.set_bid(bidRequest.bid());
            bidResponse.clear_ads();
        }
        BidResponse_Ads * adResult = bidResponse.add_ads();
        const MT::common::Solution & finalSolution = result.solution;
        const MT::common::Banner & banner = result.banner;
        std::string adxAdvIdStr = banner.adxAdvId;
        int adxAdvId = extractRealValue(adxAdvIdStr.data(), ADX_360_MAX_PC);
        std::string adxIndustryTypeStr = banner.adxIndustryType;
        int adxIndustryType = extractRealValue(adxIndustryTypeStr.data(), ADX_360_MAX_PC);
        const BidRequest_AdSlot & adzInfo = bidRequest.adslot(seq);
        int maxCpmPrice = (int)result.bidPrice * 1000;

        adResult->set_max_cpm_price(maxCpmPrice);
        adResult->set_adslot_id(adzInfo.id());
        adResult->add_creative_type(banner.bannerType);
        adResult->add_category(adxIndustryType);
        adResult->set_width(banner.width);
        adResult->set_height(banner.height);
        //缓存最终广告结果
        fillAdInfo(queryCondition, result, bidRequest.has_mv_user_id() ? bidRequest.mv_user_id() : "");

        std::string cookieMappingUrl = redoCookieMapping(ADX_360_MAX_PC, MAX_COOKIEMAPPING_URL);
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
        adResult->set_creative_id(std::to_string(adInfo.bannerId));
        adResult->set_advertiser_id(std::to_string(adxAdvId)); // adx_advid
        if (queryCondition.adxid != ADX_360_MAX_MOBILE) {
            adResult->set_html_snippet(juxiaoHtmlSnippet(cookieMappingUrl, isIOS));
            adResult->set_nurl(feedbackUrl);
        } else {
            bannerJson["advid"] = finalSolution.advId;
            bannerJson["adxpid"] = adInfo.adxpid;
            bannerJson["arid"] = adInfo.areaId;
            bannerJson["gpid"] = finalSolution.sId;
            bannerJson["pid"] = adInfo.pid;
            bannerJson["ppid"] = adInfo.ppid;
            bannerJson["price"] = adInfo.offerPrice;
            bannerJson["pricetype"] = adInfo.priceType;
            bannerJson["unid"] = adInfo.adxid;
            bannerJson["of"] = "0";
            bannerJson["width"] = banner.width;
            bannerJson["height"] = banner.height;
            bannerJson["xcurl"] = AD_MAX_CLICK_UNENC_MACRO;
            bannerJson["rs"] = true;
            url::URLHelper clickUrlParam;
            getClickPara(clickUrlParam, bidRequest.bid(), "", destUrl);
            bannerJson["clickurl"]
                = std::string(isIOS ? SNIPPET_CLICK_URL_HTTPS : SNIPPET_CLICK_URL) + "?" + clickUrlParam.cipherParam();
            std::string mtadInfoStr = adservice::utility::json::toJson(bannerJson);
            char admBuffer[4096];
            snprintf(admBuffer, sizeof(admBuffer), adservice::corelogic::HandleShowQueryTask::showAdxTemplate,
                     mtadInfoStr.data());
            adResult->set_html_snippet(admBuffer);
            url::URLHelper showUrlParam;
            getShowPara(showUrlParam, bidRequest.bid());
            showUrlParam.add(URL_IMP_OF, "3");
            showUrlParam.addMacro(URL_EXCHANGE_PRICE, AD_MAX_PRICE_MACRO);
            adResult->set_nurl(std::string(isIOS ? SNIPPET_SHOW_URL_HTTPS : SNIPPET_SHOW_URL) + "?"
                               + showUrlParam.cipherParam());
        }
    }

    void JuxiaoMaxBiddingHandler::match(adservice::utility::HttpResponse & response)
    {
        std::string result;
        if (!writeProtoBufObject(bidResponse, &result)) {
            LOG_ERROR << "failed to write protobuf object in JuxiaoMaxBiddingHandler::match";
            reject(response);
            return;
        }
        response.status(200);
        response.set_content_header("application/octet-stream");
        response.set_body(result);
    }

    void JuxiaoMaxBiddingHandler::reject(adservice::utility::HttpResponse & response)
    {
        bidResponse.Clear();
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
