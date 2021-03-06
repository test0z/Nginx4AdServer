//
// Created by guoze.lin on 16/5/3.
//

#include "tanx_bidding_handler.h"
#include "core/core_ad_sizemap.h"
#include "core/core_ip_manager.h"
#include "core/core_typetable.h"
#include "core/logic/show_query_task.h"
#include "logging.h"
#include "utility/utility.h"
#include <boost/format.hpp>

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

    namespace {

        const std::unordered_set<int64_t> videoViewTypes = { 11, 12, 15, 106, 107 };

        int getDeviceType(const std::string & deviceInfo)
        {
            if (!strcasecmp(deviceInfo.c_str(), "iphone")) {
                return SOLUTION_DEVICE_IPHONE;
            } else if (!strcasecmp(deviceInfo.c_str(), "android")) {
                return SOLUTION_DEVICE_ANDROID;
            } else if (!strcasecmp(deviceInfo.c_str(), "ipad")) {
                return SOLUTION_DEVICE_IPAD;
            } else
                return SOLUTION_DEVICE_OTHER;
        }

        int getDeviceTypeFromOs(const std::string & os)
        {
            if (!strcasecmp(os.c_str(), "ios")) {
                return SOLUTION_DEVICE_IPHONE;
            } else if (!strcasecmp(os.c_str(), "android")) {
                return SOLUTION_DEVICE_ANDROID;
            } else
                return SOLUTION_DEVICE_OTHER;
        }

        int getOSTypeFromOs(const std::string & os)
        {
            if (!strcasecmp(os.c_str(), "ios")) {
                return SOLUTION_OS_IOS;
            } else if (!strcasecmp(os.c_str(), "android")) {
                return SOLUTION_OS_ANDROID;
            } else
                return SOLUTION_OS_OTHER;
        }

        int getNetWork(int network)
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

        int getNetworkProvider(int isp)
        {
            switch (isp) {
            case 0:
                return SOLUTION_NETWORK_PROVIDER_ALL;
            case 1:
                return SOLUTION_NETWORK_PROVIDER_CHINAMOBILE;
            case 2:
                return SOLUTION_NETWORK_PROVIDER_CHINAUNICOM;
            case 3:
                return SOLUTION_NETWORK_PROVIDER_CHINATELECOM;
            }
            return SOLUTION_NETWORK_PROVIDER_ALL;
        }

        std::string durationStr(int64_t duration)
        {
            int h = duration / 3600, m = (duration % 3600) / 60, s = duration % 60;
            return boost::str(boost::format("%02d:%02d:%02d") % h % m % s);
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
            cmImage = cmImage + "<img width=\"0\" height=\"0\" src=\"" + cookieMappingUrl + "\"/>";
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
        snprintf(feedbackUrl, sizeof(feedbackUrl), "%s?%s", getShowBaseUrl(useHttps).c_str(),
                 showUrlParam.cipherParam().c_str());
        showUrlParam.add(URL_IMP_OF, "2");
        showUrlParam.removeMacro(URL_EXCHANGE_PRICE);
        showUrlParam.addMacro(URL_ADX_MACRO, AD_TX_CLICK_MACRO);
        int len = snprintf(html, sizeof(html), SNIPPET_IFRAME_SUPPORT_CM, width, height,
                           getShowBaseUrl(useHttps).c_str(), "", showUrlParam.cipherParam().c_str(), cookieMappingUrl);
        return std::string(html, html + len);
    }

    std::string TanxBiddingHandler::prepareVast(int width, int height, const cppcms::json::value & mtls,
                                                const std::string & tvm, const std::string & cm, bool linear)
    {
        std::string vastXml;
        adservice::utility::url::ParamMap pm;
        int duration
            = adservice::utility::stringtool::safeconvert(adservice::utility::stringtool::stoi, mtls.get("p17", "15"));
        pm.insert({ "title", mtls.get("p0", "") });
        pm.insert({ "impressionUrl", tvm });
        pm.insert({ "duration", durationStr(duration) });
        pm.insert({ "clickThrough", cm });
        if (linear) {
            linearVastTemplateEngine.bindFast(pm, vastXml);
        } else {
            std::string image = mtls.get("p6", "");
            std::string creativeType = "image/jpeg";
            if (image.find(".png") != std::string::npos) {
                creativeType = "image/png";
            }
            pm.insert({ "width", std::to_string(width) });
            pm.insert({ "height", std::to_string(height) });
            pm.insert({ "image", image });
            pm.insert({ "creativeType", creativeType });
            nonLinearVastTemplateEngine.bindFast(pm, vastXml);
        }
        return vastXml;
    }

    bool TanxBiddingHandler::parseRequestData(const std::string & data)
    {
        bidRequest.Clear();
        bidResponse.Clear();
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
        std::vector<PreSetAdplaceInfo> adplaceInfos{ PreSetAdplaceInfo() };
        AdSelectCondition & queryCondition = queryConditions[0];
        PreSetAdplaceInfo & pAdplaceInfo = adplaceInfos[0];
        queryCondition.pAdplaceInfo = &pAdplaceInfo;
        queryCondition.adxid = ADX_TANX;
        queryCondition.adxpid = pid;
        queryCondition.ip = bidRequest.ip();
        queryCondition.basePrice = adzInfo.has_min_cpm_price() ? adzInfo.min_cpm_price() : 0;
        queryCondition.requiredCreativeLevel
            = adzInfo.has_allowed_creative_level() ? adzInfo.allowed_creative_level() : 99;
        extractSize(adzInfo.size(), queryCondition.width, queryCondition.height);
        pAdplaceInfo.sizeArray.push_back({ queryCondition.width, queryCondition.height });
        if (bidRequest.content_categories().size() > 0) {
            const BidRequest_ContentCategory & category = bidRequest.content_categories(0);
            TypeTableManager & typeTableManager = TypeTableManager::getInstance();
            queryCondition.mttyContentType = typeTableManager.getContentType(ADX_TANX, std::to_string(category.id()));
        }
        bool isNative = false;
        bool isVideo = false;
        if (adzInfo.view_type_size() > 0) {
            for (int i = 0; i < adzInfo.view_type_size(); i++) {
                int viewType = adzInfo.view_type(i);
                if (viewType >= 104 && viewType <= 111) {
                    isNative = true;
                }
                if (videoViewTypes.find(viewType) != videoViewTypes.end()) {
                    isVideo = true;
                }
            }
        }
        if (isVideo) {
            queryCondition.bannerType = BANNER_TYPE_VIDEO;
        }
        if (bidRequest.has_mobile()) {
            const BidRequest_Mobile & mobile = bidRequest.mobile();
            const BidRequest_Mobile_Device & device = mobile.device();
            queryCondition.mobileDevice = getDeviceType(device.platform());
            if (queryCondition.mobileDevice == SOLUTION_DEVICE_OTHER) {
                queryCondition.mobileDevice = getDeviceTypeFromOs(device.os());
                if (queryCondition.mobileDevice == SOLUTION_DEVICE_OTHER) {
                    queryCondition.mobileDevice
                        = adservice::utility::userclient::getMobileTypeFromUA(bidRequest.user_agent());
                }
            }
            queryCondition.pcOS
                = device.os().empty() ? getOSTypeFromUA(bidRequest.user_agent()) : getOSTypeFromOs(device.os());
            queryCondition.flowType = SOLUTION_FLOWTYPE_MOBILE;
            queryCondition.adxid = ADX_TANX_MOBILE;
            queryCondition.mobileModel = device.has_model() ? device.model() : "";
            queryCondition.mobileNetWorkProvider = getNetworkProvider(device.operator_());
            if (device.has_network()) {
                queryCondition.mobileNetwork = getNetWork(device.network());
            }
            queryCondition.geo = { stringtool::safeconvert(stringtool::stod, device.longitude()),
                                   stringtool::safeconvert(stringtool::stod, device.latitude()) };
            queryCondition.deviceBrand = adservice::utility::userclient::getDeviceBrandFromUA(bidRequest.user_agent());
            if (mobile.has_is_app() && mobile.is_app()) { // app
                if (isNative && !isVideo) {               //原生native
                    const AdSizeMap & adSizeMap = AdSizeMap::getInstance();
                    pAdplaceInfo.sizeArray.clear();
                    auto sizePair = adSizeMap.get({ queryCondition.width, queryCondition.height });
                    for (auto sizeIter : sizePair) {
                        queryCondition.width = sizeIter.first;
                        queryCondition.height = sizeIter.second;
                        queryCondition.pAdplaceInfo->sizeArray.push_back(
                            { queryCondition.width, queryCondition.height });
                    }
                    queryCondition.bannerType = BANNER_TYPE_PRIMITIVE;
                }
                if (mobile.has_package_name()) {
                    queryCondition.adxpid = mobile.package_name();
                }
                if (mobile.app_categories().size() > 0) {
                    const BidRequest_Mobile_AppCategory & category = mobile.app_categories(0);
                    TypeTableManager & typeTableManager = TypeTableManager::getInstance();
                    queryCondition.mttyContentType
                        = typeTableManager.getContentType(ADX_TANX, std::to_string(category.id()));
                }
                cookieMappingKeyMobile(
                    md5_encode(device.has_idfa()
                                   ? (queryCondition.idfa = stringtool::toupper(tanxDeviceId(device.idfa())))
                                   : ""),
                    md5_encode(device.has_imei()
                                   ? (queryCondition.imei = stringtool::toupper(tanxDeviceId(device.imei())))
                                   : ""),
                    md5_encode(device.has_android_id()
                                   ? (queryCondition.androidId = stringtool::toupper(tanxDeviceId(device.android_id())))
                                   : ""),
                    md5_encode(device.has_mac() ? (queryCondition.mac = stringtool::toupper(tanxDeviceId(device.mac())))
                                                : ""),
                    queryCondition,
                    queryCondition.adxid,
                    bidRequest.has_tid() ? bidRequest.tid() : "");
            } else { // wap
                cookieMappingKeyWap(ADX_TANX_MOBILE, bidRequest.has_tid() ? bidRequest.tid() : "");
            }
        } else {
            queryCondition.mobileDevice = getMobileTypeFromUA(bidRequest.user_agent());
            queryCondition.pcOS = getOSTypeFromUA(bidRequest.user_agent());
            // if (queryCondition.mobileDevice != SOLUTION_DEVICE_OTHER) { // wap
            //    queryCondition.flowType = SOLUTION_FLOWTYPE_MOBILE;
            //    queryCondition.adxid = ADX_TANX_MOBILE;
            //    cookieMappingKeyWap(ADX_TANX_MOBILE, bidRequest.has_tid() ? bidRequest.tid() : "");
            //} else { // pc
            queryCondition.pcOS = getOSTypeFromUA(bidRequest.user_agent());
            queryCondition.pcBrowserStr = getBrowserTypeFromUA(bidRequest.user_agent());
            queryCondition.flowType = SOLUTION_FLOWTYPE_PC;
            cookieMappingKeyPC(ADX_TANX, bidRequest.has_tid() ? bidRequest.tid() : "");
            //}
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
        if (!bidResponse.has_bid()) {
            bidResponse.Clear();
            bidResponse.set_version(bidRequest.version());
            bidResponse.set_bid(bidRequest.bid());
            bidResponse.clear_ads();
        }
        BidResponse_Ads * adResult = bidResponse.add_ads();
        const MT::common::Solution & finalSolution = result.solution;
        const MT::common::Banner & banner = result.banner;
        std::string adxAdvIdStr = banner.adxAdvId;
        int adxAdvId = MT::common::stoi_safe(extractRealValue(adxAdvIdStr.data(), ADX_TANX));
        std::string adxIndustryTypeStr = banner.adxIndustryType;
        int adxIndustryType = MT::common::stoi_safe(extractRealValue(adxIndustryTypeStr.data(), ADX_TANX));
        const BidRequest_AdzInfo & adzInfo = bidRequest.adzinfo(seq);
        int maxCpmPrice = (int)result.bidPrice;

        adResult->set_max_cpm_price(maxCpmPrice);
        adResult->set_adzinfo_id(adzInfo.id());
        adResult->set_ad_bid_count_idx(0);
        adResult->add_creative_type(2);
        adResult->add_category(adxIndustryType);
        //缓存最终广告结果
        fillAdInfo(queryCondition, result, bidRequest.tid());

        std::string cookieMappingUrl = redoCookieMapping(ADX_TANX, TANX_COOKIEMAPPING_URL);
        bool isIOS = queryCondition.mobileDevice == SOLUTION_DEVICE_IPHONE
                     || queryCondition.mobileDevice == SOLUTION_DEVICE_IPAD;
        std::string strBannerJson = banner.json;
        cppcms::json::value bannerJson = bannerJson2HttpsIOS(isIOS, strBannerJson, banner.bannerType);
        const cppcms::json::array & mtlsArray = bannerJson["mtls"].array();

        adResult->set_creative_id(std::to_string(adInfo.bannerId));
        adResult->add_advertiser_ids(adxAdvId); // adx_advid
        if (queryCondition.flowType == SOLUTION_FLOWTYPE_MOBILE
            && banner.bannerType == BANNER_TYPE_PRIMITIVE) { //移动原生广告
            std::string destUrl = mtlsArray[0].get("p9", "");
            adResult->add_destination_url(destUrl);
            url::URLHelper clickUrlParam;
            getClickPara(clickUrlParam, bidRequest.bid(), "", destUrl);
            std::string clickUrl = getClickBaseUrl(isIOS) + "?" + clickUrlParam.cipherParam();
            adResult->add_click_through_url(clickUrl);
            std::string iosDownloadUrl = mtlsArray[0].get("p10", "");
            std::string androidDownloadUrl = mtlsArray[0].get("p11", "");
            bool supportDownload = !iosDownloadUrl.empty() || !androidDownloadUrl.empty();
            auto mobileCreative = adResult->mutable_mobile_creative();
            mobileCreative->set_version(bidRequest.version());
            mobileCreative->set_bid(bidRequest.bid());
            auto creative = mobileCreative->add_creatives();
            creative->set_img_url(mtlsArray[0].get("p6", ""));
            creative->set_img_size(adzInfo.size());
            creative->set_title(mtlsArray[0].get("p0", ""));
            creative->set_click_url(clickUrl);
            creative->set_destination_url(destUrl);
            creative->set_creative_id(std::to_string(banner.bId));
            std::string nativeTmpId;
            for (int32_t nativeCounter = 0; nativeCounter != bidRequest.mobile().native_ad_template_size();
                 nativeCounter++) {
                auto & nativeTemplate = bidRequest.mobile().native_ad_template(nativeCounter);
                auto & areaCreative = nativeTemplate.areas(0).creative();
                if ((!supportDownload && areaCreative.action_fields_size() > 0 && areaCreative.action_fields(0) == 1)
                    && nativeCounter != bidRequest.mobile().native_ad_template_size() - 1) {
                    continue;
                }
                nativeTmpId = nativeTemplate.native_template_id();
                for (int32_t f = 0; f < areaCreative.required_fields_size(); f++) {
                    int32_t field = areaCreative.required_fields(f);
                    switch (field) {
                    case 2: // ad_words
                    {
                        auto newAttr = creative->add_attr();
                        newAttr->set_name("ad_words");
                        newAttr->set_value(mtlsArray[0].get("p0", ""));
                    } break;
                    default:
                        break;
                    }
                }
                break;
            }
            if (supportDownload) { //不管action_type是否为1,都设置下载属性
                auto downloadAttr = creative->add_attr();
                downloadAttr->set_name("download_url");
                auto downloadTypeAttr = creative->add_attr();
                downloadTypeAttr->set_name("download_type");
                if (isIOS) {
                    downloadAttr->set_value(iosDownloadUrl);
                    downloadTypeAttr->set_value("4");
                } else {
                    downloadAttr->set_value(androidDownloadUrl);
                    downloadTypeAttr->set_value("2");
                }
            }
            mobileCreative->set_native_template_id(nativeTmpId);
            url::URLHelper showUrlParam;
            getShowPara(showUrlParam, bidRequest.bid());
            showUrlParam.add(URL_IMP_OF, "3");
            showUrlParam.addMacro(URL_EXCHANGE_PRICE, AD_TX_PRICE_MACRO);
            snprintf(feedbackUrl, sizeof(feedbackUrl), "%s?%s", getShowBaseUrl(isIOS).c_str(),
                     showUrlParam.cipherParam().c_str());
            adResult->set_feedback_address(feedbackUrl);
        } else if (banner.bannerType == BANNER_TYPE_VIDEO) { //视频贴片广告
            std::string destUrl = mtlsArray[0].get("p1", "");
            adResult->add_destination_url(destUrl);
            adResult->add_click_through_url(destUrl);
            url::URLHelper clickUrlParam;
            getClickPara(clickUrlParam, bidRequest.bid(), "", destUrl);
            std::string clickUrl = getClickBaseUrl(isIOS) + "?" + clickUrlParam.cipherParam();
            url::URLHelper showUrlParam;
            getShowPara(showUrlParam, bidRequest.bid());
            showUrlParam.add(URL_IMP_OF, "3");
            showUrlParam.addMacro(URL_EXCHANGE_PRICE, AD_TX_PRICE_MACRO);
            std::string impressionUrl = getShowBaseUrl(isIOS) + "?" + showUrlParam.cipherParam();
            bool isLinear = true;
            for (int i = 0; i < adzInfo.view_type_size(); i++) {
                int viewType = adzInfo.view_type(i);
                if (viewType == 12 || viewType == 107) {
                    isLinear = false;
                    break;
                }
            }
            adResult->set_video_snippet(
                prepareVast(banner.width, banner.height, mtlsArray[0], impressionUrl, clickUrl, isLinear));
        } else { //非移动原生广告,非贴片广告
            std::string destUrl = mtlsArray[0].get("p1", "");
            adResult->add_destination_url(destUrl);
            adResult->add_click_through_url(destUrl);
            if (queryCondition.flowType == SOLUTION_FLOWTYPE_PC) {
                adResult->set_html_snippet(tanxHtmlSnippet(cookieMappingUrl, isIOS));
                adResult->set_feedback_address(feedbackUrl);
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
                bannerJson["xcurl"] = AD_TX_CLICK_UNENC_MACRO;
                bannerJson["rs"] = queryCondition.flowType == SOLUTION_FLOWTYPE_MOBILE;
                url::URLHelper clickUrlParam;
                getClickPara(clickUrlParam, bidRequest.bid(), "", destUrl);
                std::string clickUrl = getClickBaseUrl(isIOS) + "?" + clickUrlParam.cipherParam();
                bannerJson["clickurl"] = clickUrl;
                std::string mtadInfoStr = adservice::utility::json::toJson(bannerJson);
                char admBuffer[4096];
                snprintf(admBuffer, sizeof(admBuffer), adservice::corelogic::HandleShowQueryTask::showAdxTemplate,
                         mtadInfoStr.data());
                adResult->set_html_snippet(admBuffer);
                url::URLHelper showUrlParam;
                getShowPara(showUrlParam, bidRequest.bid());
                showUrlParam.add(URL_IMP_OF, "3");
                showUrlParam.addMacro(URL_EXCHANGE_PRICE, AD_TX_PRICE_MACRO);
                snprintf(feedbackUrl, sizeof(feedbackUrl), "%s?%s", getShowBaseUrl(isIOS).c_str(),
                         showUrlParam.cipherParam().c_str());
                adResult->set_feedback_address(feedbackUrl);
            }
        }
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
