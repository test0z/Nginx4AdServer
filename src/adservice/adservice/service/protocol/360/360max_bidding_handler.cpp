#include "360max_bidding_handler.h"
#include "core/core_ad_sizemap.h"
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
#define MAX_COOKIEMAPPING_URL "http://cm.mediav.com/s?mvdid=134"

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

    static int getDeviceOs(const std::string & os)
    {
        if (os.find("ios") != std::string::npos) {
            return SOLUTION_OS_IOS;
        } else if (os.find("android") != std::string::npos) {
            return SOLUTION_OS_ANDROID;
        } else {
            return SOLUTION_OS_OTHER;
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
            cmImage = cmImage + "<img width=\"0\" height=\"0\" src=\"" + cookieMappingUrl + "\"/>";
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
        snprintf(feedbackUrl, sizeof(feedbackUrl), "%s?%s", getShowBaseUrl(useHttps).c_str(),
                 showUrlParam.cipherParam().c_str());
        showUrlParam.add(URL_IMP_OF, "2");
        showUrlParam.removeMacro(URL_EXCHANGE_PRICE);
        showUrlParam.addMacro(URL_ADX_MACRO, AD_MAX_CLICK_MACRO);
        int len = snprintf(html, sizeof(html), SNIPPET_IFRAME_SUPPORT_CM, width, height,
                           getShowBaseUrl(useHttps).c_str(), "", showUrlParam.cipherParam().c_str(), cookieMappingUrl);
        return std::string(html, html + len);
    }

    bool JuxiaoMaxBiddingHandler::parseRequestData(const std::string & data)
    {
        bidRequest.Clear();
        bidResponse.Clear();
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
        PreSetAdplaceInfo pAdplaceInfo;
        queryCondition.pAdplaceInfo = &pAdplaceInfo;
        queryCondition.adxid = ADX_360_MAX_PC;
        queryCondition.adxpid = pid;
        queryCondition.ip = bidRequest.ip();
        queryCondition.basePrice = (adzInfo.has_min_cpm_price() ? adzInfo.min_cpm_price() : 0) / 10000;
        if (adzInfo.has_native()) { //原生请求
            const AdSizeMap & adSizeMap = AdSizeMap::getInstance();
            auto sizePair = adSizeMap.get({ adzInfo.width(), adzInfo.height() });
            for (auto & sizeIter : sizePair) {
                queryCondition.width = sizeIter.first;
                queryCondition.height = sizeIter.second;
                pAdplaceInfo.sizeArray.push_back({ queryCondition.width, queryCondition.height });
            }
            queryCondition.bannerType = BANNER_TYPE_PRIMITIVE;
        } else { //非原生请求
            queryCondition.width = adzInfo.width();
            queryCondition.height = adzInfo.height();
            pAdplaceInfo.sizeArray.push_back({ queryCondition.width, queryCondition.height });
        }
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
            queryCondition.pcOS = getDeviceOs(device.os());
            queryCondition.mobileModel = device.model();
            queryCondition.flowType = SOLUTION_FLOWTYPE_MOBILE;
            queryCondition.adxid = ADX_360_MAX_MOBILE;
            pAdplaceInfo.flowType = queryCondition.flowType;
            if (device.has_network()) {
                queryCondition.mobileNetwork = getNetWork(device.network());
            }
            if (mobile.has_is_app() && mobile.is_app()) { // app
                if (mobile.app_category_size() > 0) {
                    TypeTableManager & typeTableManager = TypeTableManager::getInstance();
                    queryCondition.mttyContentType
                        = typeTableManager.getContentType(ADX_TANX, std::to_string(mobile.app_category(0)));
                }
                cookieMappingKeyMobile(
                    md5_encode(device.has_idfa() ? (queryCondition.idfa = stringtool::toupper(device.idfa())) : ""),
                    md5_encode(device.has_imei() ? (queryCondition.imei = stringtool::toupper(device.imei())) : ""),
                    md5_encode(device.has_android_id()
                                   ? (queryCondition.androidId = stringtool::toupper(device.android_id()))
                                   : ""),
                    md5_encode(device.has_mac() ? (queryCondition.mac = stringtool::toupper(device.mac())) : ""),
                    queryCondition,
                    queryCondition.adxid,
                    bidRequest.has_mv_user_id() ? bidRequest.mv_user_id() : "");
            } else { // wap
                cookieMappingKeyWap(ADX_360_MAX_MOBILE, bidRequest.has_mv_user_id() ? bidRequest.mv_user_id() : "");
            }
        } else {
            queryCondition.pcOS = getOSTypeFromUA(bidRequest.user_agent());
            queryCondition.pcBrowserStr = getBrowserTypeFromUA(bidRequest.user_agent());
            queryCondition.flowType = SOLUTION_FLOWTYPE_PC;
            cookieMappingKeyPC(ADX_360_MAX_PC, bidRequest.has_mv_user_id() ? bidRequest.mv_user_id() : "");
            pAdplaceInfo.flowType = queryCondition.flowType;
        }
        queryCookieMapping(cmInfo.queryKV, queryCondition);
        const auto & deals = adzInfo.deals();
        if (deals.size() > 0) {
            std::stringstream ss;
            ss << ",";
            for (int i = 0; i < deals.size(); ++i) {
                const auto & deal = deals.Get(i);
                ss << deal.deal_id() << ",";
            }
            queryCondition.dealId = ss.str();
        }
        if (!filterCb(this, queryConditions)) {
            return bidFailedReturn();
        }
        return isBidAccepted = true;
    }

    void JuxiaoMaxBiddingHandler::buildBidResult(const AdSelectCondition & queryCondition,
                                                 const MT::common::SelectResult & result, int seq)
    {
        if (!bidResponse.has_bid()) {
            bidResponse.Clear();
            bidResponse.set_bid(bidRequest.bid());
            bidResponse.clear_ads();
        }
        BidResponse_Ads * adResult = bidResponse.add_ads();
        const MT::common::Solution & finalSolution = result.solution;
        const MT::common::Banner & banner = result.banner;
        std::string adxAdvIdStr = banner.adxAdvId;
        int adxAdvId = extractRealValue(adxAdvIdStr.data(), ADX_360_MAX_PC);
        if (adxAdvId == 0) {
            adxAdvId = finalSolution.advId;
        }
        std::string adxIndustryTypeStr = banner.adxIndustryType;
        int adxIndustryType = extractRealValue(adxIndustryTypeStr.data(), ADX_360_MAX_PC);
        const BidRequest_AdSlot & adzInfo = bidRequest.adslot(seq);
        int maxCpmPrice = (int)result.bidPrice * 10000;

        adResult->set_max_cpm_price(maxCpmPrice);
        adResult->set_adslot_id(adzInfo.id());
        adResult->add_creative_type(1);
        adResult->add_category(adxIndustryType);

        //缓存最终广告结果
        fillAdInfo(queryCondition, result, bidRequest.has_mv_user_id() ? bidRequest.mv_user_id() : "");

        std::string cookieMappingUrl = redoCookieMapping(ADX_360_MAX_PC, MAX_COOKIEMAPPING_URL);
        bool isIOS = queryCondition.mobileDevice == SOLUTION_DEVICE_IPHONE
                     || queryCondition.mobileDevice == SOLUTION_DEVICE_IPAD;
        bool isNeedHttps = isIOS || bidRequest.is_https();
        std::string strBannerJson = banner.json;
        cppcms::json::value bannerJson = bannerJson2HttpsIOS(isNeedHttps, strBannerJson, banner.bannerType);
        const cppcms::json::array & mtlsArray = bannerJson["mtls"].array();
        adResult->set_creative_id(std::to_string(adInfo.bannerId));
        adResult->set_advertiser_id(std::to_string(adxAdvId)); // adx_advid
        if (banner.bannerType == BANNER_TYPE_PRIMITIVE) {      //原生广告
            std::string destUrl = mtlsArray[0].get("p9", "");
            adResult->add_destination_url(destUrl);
            const AdSizeMap & adSizeMap = AdSizeMap::getInstance();
            auto sizePair = adSizeMap.rget({ banner.width, banner.height });
            adResult->set_width(sizePair.first);
            adResult->set_height(sizePair.second);
            auto nativeAd = adResult->add_native_ad();
            nativeAd->set_advertiser_id(std::to_string(adxAdvId));
            nativeAd->set_creative_id(std::to_string(adInfo.bannerId));
            nativeAd->set_max_cpm_price(maxCpmPrice);
            nativeAd->add_category(adxIndustryType);
            auto creative = nativeAd->add_creatives();
            std::string title = mtlsArray[0].get("p0", "");
            std::string subTitle = mtlsArray[0].get("p1", "");
            std::string description = mtlsArray[0].get("p5", "");
            std::string imageUrl = mtlsArray[0].get("p6", "");
            std::string logoUrl = mtlsArray[0].get("p15", "");
            creative->set_title(title);
            creative->set_sub_title(subTitle);
            creative->set_description(description);
            creative->set_button_name("麦田科技");
            auto contentImage = creative->mutable_content_image();
            contentImage->set_image_url(imageUrl);
            contentImage->set_image_width(sizePair.first);
            contentImage->set_image_height(sizePair.second);
            auto logoImage = creative->mutable_logo();
            logoImage->set_image_url(logoUrl);
            std::string iosDownloadUrl = mtlsArray[0].get("p10", "");
            std::string androidDownloadUrl = mtlsArray[0].get("p11", "");
            std::string downloadUrl = isIOS ? iosDownloadUrl : androidDownloadUrl;
            url::URLHelper clickUrlParam;
            getClickPara(clickUrlParam, bidRequest.bid(), "", downloadUrl.empty() ? destUrl : downloadUrl);
            std::string linkUrl = getClickBaseUrl(isNeedHttps) + "?" + clickUrlParam.cipherParam();
            auto linkObj = creative->mutable_link();
            linkObj->set_click_url(std::string(AD_MAX_CLICK_UNENC_MACRO) + url::urlEncode(linkUrl));
            linkObj->set_landing_type(0);
            for (int32_t ind = 0; ind < adzInfo.native().landing_type_size(); ind++) {
                if (!downloadUrl.empty() && adzInfo.native().landing_type(ind) == 1) { //支持下载类
                    linkObj->set_landing_type(1);
                    break;
                } else {
                    linkObj->set_landing_type(adzInfo.native().landing_type(ind));
                }
            }
            url::URLHelper showUrlParam;
            getShowPara(showUrlParam, bidRequest.bid());
            showUrlParam.add(URL_IMP_OF, "3");
            showUrlParam.addMacro(URL_EXCHANGE_PRICE, AD_MAX_PRICE_MACRO);
            if (queryCondition.dealId != "0") {
                nativeAd->set_deal_id(std::stoi(finalSolution.dDealId));
                showUrlParam.add(URL_DEAL_ID, finalSolution.dDealId);
            }
            std::string impressionTrack = getShowBaseUrl(isNeedHttps) + "?" + showUrlParam.cipherParam();
            nativeAd->add_impression_tracks()->assign(impressionTrack);
            // adResult->set_nurl(impressionTrack);
        } else { //非原生广告
            std::string destUrl = mtlsArray[0].get("p1", "");
            adResult->add_destination_url(destUrl);
            adResult->set_width(banner.width);
            adResult->set_height(banner.height);
            if (queryCondition.flowType == SOLUTION_FLOWTYPE_PC) { // pc
                adResult->set_html_snippet(juxiaoHtmlSnippet(cookieMappingUrl, isIOS));
                adResult->set_nurl(feedbackUrl);
            } else { //移动wap
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
                bannerJson["clickurl"] = getClickBaseUrl(isIOS) + "?" + clickUrlParam.cipherParam();
                std::string mtadInfoStr = adservice::utility::json::toJson(bannerJson);
                char admBuffer[4096];
                snprintf(admBuffer, sizeof(admBuffer), adservice::corelogic::HandleShowQueryTask::showAdxTemplate,
                         mtadInfoStr.data());
                adResult->set_html_snippet(admBuffer);
                url::URLHelper showUrlParam;
                getShowPara(showUrlParam, bidRequest.bid());
                showUrlParam.add(URL_IMP_OF, "3");
                showUrlParam.addMacro(URL_EXCHANGE_PRICE, AD_MAX_PRICE_MACRO);
                adResult->set_nurl(getShowBaseUrl(isIOS) + "?" + showUrlParam.cipherParam());
            }
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
