//
// Created by guoze.lin on 16/5/3.
//

#include "protocol/baidu/baidu_bidding_handler.h"
#include "core/core_ad_sizemap.h"
#include "core/core_ip_manager.h"
#include "core/core_typetable.h"
#include "core/logic/show_query_task.h"
#include "logging.h"
#include "utility/json.h"
#include "utility/userclient.h"
#include "utility/utility.h"
#include <boost/lexical_cast.hpp>

namespace protocol {
namespace bidding {

    using namespace protocol::baidu;
    using namespace adservice::utility;
    using namespace adservice::utility::serialize;
    using namespace adservice::server;
    using namespace adservice::utility::userclient;
    using namespace adservice::utility::json;
    using namespace adservice::utility::cypher;
    using namespace adservice::utility::AdSizeMap;

#define AD_BD_CLICK_MACRO "%%CLICK_URL_0%%"
#define AD_BD_PRICE_MACRO "%%PRICE%%"
#define AD_COOKIEMAPPING_BAIDU "http://cm.pos.baidu.com/pixel"

    inline int max(const int & a, const int & b)
    {
        return a > b ? a : b;
    }

    int getDevicebyPlat(int os_type, int device_type)
    {
        switch (os_type) {
        case 1:
            switch (device_type) {
            case 1:
                return SOLUTION_DEVICE_IPHONE;
            case 2:
                return SOLUTION_DEVICE_IPAD;
            default:
                return SOLUTION_DEVICE_OTHER;
            }
        case 2:
            switch (device_type) {
            case 1:
                return SOLUTION_DEVICE_ANDROID;
            case 2:
                return SOLUTION_DEVICE_ANDROIDPAD;
            default:
                return SOLUTION_DEVICE_OTHER;
            }
        case 3:
            return SOLUTION_DEVICE_WINDOWSPHONE;
        default:
            break;
        }
        return SOLUTION_DEVICE_OTHER;
    }
    int getMobileNet(int net_type)
    {
        switch (net_type) {
        case 1:
            return SOLUTION_NETWORK_WIFI;
        case 2:
            return SOLUTION_NETWORK_2G;
        case 3:
            return SOLUTION_NETWORK_3G;
        case 4:
            return SOLUTION_NETWORK_4G;
        default:
            break;
        }
        return SOLUTION_NETWORK_OTHER;
    }
    void getIDFA_AND(AdSelectCondition & select, const BidRequest_Mobile_ForAdvertisingID & temp)
    {
        switch (temp.type()) {
        case 4:
            select.androidId = temp.id();
            select.idfa = "";
            break;
        case 5:
            select.idfa = temp.id();
            select.androidId = "";
            break;
        default:
            select.androidId = "";
            select.idfa = "";
            break;
        }
    }
    void getMAC_IMEI(AdSelectCondition & select, const BidRequest_Mobile_MobileID & temp)
    {
        switch (temp.type()) {
        case 1:
            select.imei = temp.id();
            select.mac = "";
            break;
        case 2:
            select.mac = temp.id();
            select.imei = "";
            break;
        default:
            select.imei = "";
            select.mac = "";
            break;
        }
    }
    bool BaiduBiddingHandler::parseRequestData(const std::string & data)
    {
        bidRequest.Clear();
        bidResponse.Clear();
        return getProtoBufObject(bidRequest, data);
    }

    std::string BaiduBiddingHandler::baiduHtmlSnippet(const std::string & cookieMappingUrl, bool useHttps)
    {
        std::string bid = bidRequest.id();
        // bool isMobile = bidRequest.has_mobile();
        const BidRequest_AdSlot & adSlot = bidRequest.adslot(0);
        int width = adSlot.width();
        int height = adSlot.height();
        std::string cmImage;
        if (!cookieMappingUrl.empty()) {
            cmImage = cmImage + "<img src=\"" + cookieMappingUrl + "\"/>";
        }
        return generateHtmlSnippet(bid, width, height, NULL, cmImage.c_str(), useHttps);
    }
    std::string BaiduBiddingHandler::generateHtmlSnippet(const std::string & bid, int width, int height,
                                                         const char * extShowBuf, const char * cookieMappingUrl,
                                                         bool useHttps)
    {
        char html[4096];
        memset(html, 0x00, 4096);
        // url::URLHelper showUrlParam;
        // getShowPara(showUrlParam, bid);
        // showUrlParam.add(URL_IMP_OF, "2");
        // showUrlParam.addMacro(URL_ADX_MACRO, AD_BD_CLICK_MACRO);
        // int length
        //   = snprintf(html, sizeof(html), "< a herf=%s /> %s", showUrlParam.cipherParam().c_str(), cookieMappingUrl);
        int length = snprintf(html, sizeof(html), "< a herf=%s /> %s", AD_BD_CLICK_MACRO, cookieMappingUrl);
        return std::string(html, html + length);
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

        std::vector<AdSelectCondition> queryConditions(bidRequest.adslot_size());
        for (int i = 0; i < bidRequest.adslot_size(); i++) {
            AdSelectCondition & queryCondition = queryConditions[i];
            const BidRequest_AdSlot & adSlot = bidRequest.adslot(i);
            long pid = adSlot.ad_block_key();
            queryCondition.adxid = ADX_BAIDU;
            queryCondition.adxpid = std::to_string(pid);
            queryCondition.ip = bidRequest.ip();
            //获取广告位底价
            queryCondition.basePrice = adSlot.has_minimum_cpm() ? adSlot.minimum_cpm() : 0;
            //获取广告位宽高
            queryCondition.width = adSlot.width();
            queryCondition.height = adSlot.height();
            //获取网站类型
            if (bidRequest.has_site_category()) {
                TypeTableManager & typeTableManager = TypeTableManager::getInstance();
                queryCondition.mttyContentType
                    = typeTableManager.getContentType(ADX_BAIDU, std::to_string(bidRequest.site_category()));
            }
            if (!adSlot.allowed_non_nativead()) {
                queryCondition.bannerType = BANNER_TYPE_PRIMITIVE;
                const BidRequest_AdSlot_NativeAdParam_ImageEle & res_img = adSlot.nativead_param().image();
                const AdSizeMap & adSizeMap = AdSizeMap::getInstance();
                // auto sizePair = adSizeMap.get({ queryCondition.width, queryCondition.height });
                auto sizePair = adSizeMap.get({ res_img.width(), res_img.height() });
                queryCondition.width = sizePair.first;
                queryCondition.height = sizePair.second;
            }
            //移动端处理
            if (i == 0) {
                if (bidRequest.has_mobile()) {
                    const BidRequest_Mobile & mobile = bidRequest.mobile();
                    queryCondition.mobileDevice = getDevicebyPlat(mobile.platform(), mobile.device_type());
                    if (queryCondition.mobileDevice == SOLUTION_DEVICE_OTHER) {
                        queryCondition.mobileDevice
                            = adservice::utility::userclient::getMobileTypeFromUA(bidRequest.user_agent());
                    }
                    queryCondition.adxid = ADX_BAIDU_MOBILE;
                    queryCondition.width = adSlot.actual_width();
                    queryCondition.height = adSlot.actual_height();
                    queryCondition.flowType = SOLUTION_FLOWTYPE_MOBILE;
                    if (mobile.has_wireless_network_type())
                        queryCondition.mobileNetwork = getMobileNet(mobile.wireless_network_type());
                    if (mobile.has_mobile_app()) {
                        if (mobile.mobile_app().has_app_id())
                            queryCondition.adxpid = mobile.mobile_app().app_bundle_id();
                        if (mobile.mobile_app().has_app_category()) {
                            TypeTableManager & typeTableManager = TypeTableManager::getInstance();
                            queryCondition.mttyContentType = typeTableManager.getContentType(
                                ADX_BAIDU_MOBILE, std::to_string(mobile.mobile_app().app_category()));
                        }
                        queryCondition.bannerType = BANNER_TYPE_PRIMITIVE;
                        AdSizeMap & adSize_map = AdSizeMap::getInstance();
                        const AdSizeMap & adSizeMap = AdSizeMap::getInstance();
                        // const BidRequest_AdSlot_NativeAdParam_ImageEle & res_img = adSlot.nativead_param().image();
                        // auto sizePair = adSizeMap.get({ res_img.width(), res_img.height() });
                        auto sizePair = adSizeMap.get({ queryCondition.width, queryCondition.height });
                        queryCondition.width = sizePair.first;
                        queryCondition.height = sizePair.second;
                        if (mobile.for_advertising_id_size() > 0) {
                            const BidRequest_Mobile_ForAdvertisingID bid_fa_id = mobile.for_advertising_id(0);
                            getIDFA_AND(queryCondition, bid_fa_id);
                        }
                        if (mobile.id_size() > 0) {
                            const BidRequest_Mobile_MobileID bid_mo_id = mobile.id(0);
                            getMAC_IMEI(queryCondition, bid_mo_id);
                        }

                        boost::algorithm::to_upper(queryCondition.idfa);
                        boost::algorithm::to_upper(queryCondition.imei);
                        boost::algorithm::to_upper(queryCondition.androidId);
                        boost::algorithm::to_upper(queryCondition.mac);
                        cookieMappingKeyMobile(md5_encode(queryCondition.idfa), md5_encode(queryCondition.imei),
                                               md5_encode(queryCondition.androidId), md5_encode(queryCondition.mac));
                    } else {
                        cookieMappingKeyWap(
                            ADX_BAIDU_MOBILE,
                            (bidRequest.baidu_id_list_size() > 0) ? bidRequest.baidu_id_list(0).baidu_user_id() : "");
                    }
                } else {
                    queryCondition.mobileDevice = getMobileTypeFromUA(bidRequest.user_agent());
                    if (queryCondition.mobileDevice != SOLUTION_DEVICE_OTHER) {
                        queryCondition.flowType = SOLUTION_FLOWTYPE_MOBILE;
                        queryCondition.adxid = ADX_BAIDU_MOBILE;
                        cookieMappingKeyWap(
                            ADX_BAIDU_MOBILE,
                            (bidRequest.baidu_id_list_size() > 0) ? bidRequest.baidu_id_list(0).baidu_user_id() : "");
                    } else {
                        queryCondition.pcOS = getOSTypeFromUA(bidRequest.user_agent());
                        queryCondition.flowType = SOLUTION_FLOWTYPE_PC;
                        cookieMappingKeyPC(
                            ADX_BAIDU,
                            (bidRequest.baidu_id_list_size() > 0) ? bidRequest.baidu_id_list(0).baidu_user_id() : "");
                    }
                }
                queryCookieMapping(cmInfo.queryKV, queryCondition);
            } else {
                AdSelectCondition & firstqueryCondition = queryConditions[0];
                queryCondition.idfa = firstqueryCondition.idfa;
                queryCondition.androidId = firstqueryCondition.androidId;
                queryCondition.imei = firstqueryCondition.imei;
                queryCondition.pcOS = firstqueryCondition.pcOS;
                queryCondition.flowType = firstqueryCondition.flowType;
                queryCondition.mobileDevice = firstqueryCondition.mobileDevice;
                queryCondition.adxid = firstqueryCondition.adxid;
                queryCondition.adxpid = firstqueryCondition.adxpid;
                queryCondition.mttyContentType = firstqueryCondition.mttyContentType;
                queryCondition.mobileNetwork = firstqueryCondition.mobileNetwork;
                queryCondition.bannerType = firstqueryCondition.bannerType;
                if (queryCondition.flowType == SOLUTION_FLOWTYPE_MOBILE) {
                    queryCondition.width = firstqueryCondition.width;
                    queryCondition.height = firstqueryCondition.height;
                }
            }
        }
        if (!filterCb(this, queryConditions)) {
            // adInfo.bidSize = makeBidSize(bidRequest.adslot(0).width(), bidRequest.adslot(0).height());
            return bidFailedReturn();
        }
        return isBidAccepted = true;
    }

    void BaiduBiddingHandler::buildBidResult(const AdSelectCondition & queryCondition,
                                             const MT::common::SelectResult & result, int seq)
    {
        if (!bidResponse.has_id()) {
            bidResponse.Clear();
            bidResponse.set_id(bidRequest.id());
            bidResponse.clear_ad();
        }
        BidResponse_Ad * adResult = bidResponse.add_ad();
        const MT::common::Solution & finalSolution = result.solution;
        // const MT::common::ADPlace & adplace = result.adplace;
        const MT::common::Banner & banner = result.banner;
        int advId = finalSolution.advId;
        const BidRequest_AdSlot & adSlot = bidRequest.adslot(seq);
        //获取创意行业类型
        std::string adxIndustryTypeStr = banner.adxIndustryType;
        int adxIndustryType = extractRealValue(adxIndustryTypeStr.data(), ADX_BAIDU);
        int maxCpmPrice = max(result.bidPrice, adSlot.minimum_cpm());
        adResult->set_max_cpm(maxCpmPrice);
        adResult->set_advertiser_id(advId);
        adResult->set_creative_id(banner.bId);
        adResult->set_height(banner.height);
        adResult->set_width(banner.width);
        adResult->set_sequence_id(adSlot.sequence_id());
        adResult->set_category(adxIndustryType);
        adResult->set_type(banner.bannerType);
        //缓存最终广告结果
        fillAdInfo(queryCondition, result, bidRequest.id());
        std::string cookieMappingUrl = redoCookieMapping(ADX_BAIDU, AD_COOKIEMAPPING_BAIDU);
        bool isIOS = queryCondition.mobileDevice == SOLUTION_DEVICE_IPHONE
                     || queryCondition.mobileDevice == SOLUTION_DEVICE_IPAD; //|| adSlot.secure();
        std::string strBannerJson = banner.json;
        cppcms::json::value bannerJson = bannerJson2HttpsIOS(isIOS, strBannerJson, banner.bannerType);
        const cppcms::json::array & mtlsArray = bannerJson["mtls"].array();
        // if (queryCondition.flowType == SOLUTION_FLOWTYPE_MOBILE && queryCondition.bannerType ==
        // BANNER_TYPE_PRIMITIVE) {
        if (queryCondition.bannerType == BANNER_TYPE_PRIMITIVE) {
            BidResponse_Ad_NativeAd * native_ad = adResult->mutable_native_ad();

            const BidRequest_AdSlot_NativeAdParam_ImageEle & res_img = adSlot.nativead_param().image();
            const BidRequest_AdSlot_NativeAdParam_ImageEle & res_logo = adSlot.nativead_param().logo_icon();
            BidResponse_Ad_NativeAd_Image * logo = native_ad->mutable_logo_icon();
            std::string landing_url = mtlsArray[0].get("p9", ""); //获取落地页
            adResult->set_landing_page(landing_url);
            url::URLHelper clickUrlParam;
            getClickPara(clickUrlParam, bidRequest.id(), "", landing_url);
            clickUrlParam.add(URL_IMP_OF, "2");
            std::string click_url
                = std::string(isIOS ? SNIPPET_CLICK_URL_HTTPS : SNIPPET_CLICK_URL) + "?" + clickUrlParam.cipherParam();
            adResult->add_target_url(click_url);
            int img_total = 0;
            // std::string type_img = mtlsArray[0].get("p2", "");
            int type = boost::lexical_cast<int>(mtlsArray[0].get("p2", ""));
            if (type == 3) {
                img_total = 3;
            } else if (type < 3) {
                img_total = 1;
            } else {
                img_total = 0;
            }
            //取组图最多三个
            for (int i = 0; i < img_total; i++) {
                if (i == 3)
                    break;
                std::ostringstream out_str;
                out_str << 'p' << 6 + i;
                BidResponse_Ad_NativeAd_Image * native_img = native_ad->add_image();
                std::string img_url = mtlsArray[0].get(out_str.str(), "");
                if (adSlot.secure())
                    adservice::utility::url::url_replace(img_url, "http://", "https://");
                else
                    adservice::utility::url::url_replace(img_url, "https://", "http://");
                native_img->set_url(img_url);
                native_img->set_width(res_img.width());
                native_img->set_height(res_img.height());
            }
            string logo_url = mtlsArray[0].get("p15", "");
            if (!logo_url.empty()) {
                if (adSlot.secure())
                    adservice::utility::url::url_replace(logo_url, "http://", "https://");
                else
                    adservice::utility::url::url_replace(logo_url, "https://", "http://");
                native_ad->set_title(mtlsArray[0].get("p0", ""));
                logo->set_url(logo_url);
                logo->set_width(res_logo.width());
                logo->set_height(res_logo.height());
            }
            url::URLHelper showUrlParam;
            getShowPara(showUrlParam, bidRequest.id());
            showUrlParam.add(URL_IMP_OF, "3");
            showUrlParam.addMacro(URL_EXCHANGE_PRICE, AD_BD_PRICE_MACRO);
            std::string monitor_url
                = std::string(isIOS ? SNIPPET_SHOW_URL_HTTPS : SNIPPET_SHOW_URL) + "?" + showUrlParam.cipherParam();
            adResult->add_monitor_urls(monitor_url);
        } else {
            std::string landing_url = mtlsArray[0].get("p1", "");
            url::URLHelper clickUrlParam;
            getClickPara(clickUrlParam, bidRequest.id(), "", landing_url);
            std::string click_url
                = std::string(isIOS ? SNIPPET_CLICK_URL_HTTPS : SNIPPET_CLICK_URL) + "?" + clickUrlParam.cipherParam();
            adResult->add_target_url(click_url);
            adResult->set_landing_page(landing_url);
            if (queryCondition.flowType == SOLUTION_FLOWTYPE_PC) {
                adResult->set_html_snippet(baiduHtmlSnippet(cookieMappingUrl, isIOS));
                url::URLHelper showUrlParam;
                getShowPara(showUrlParam, bidRequest.id());
                showUrlParam.add(URL_IMP_OF, "3");
                showUrlParam.addMacro(URL_EXCHANGE_PRICE, AD_BD_PRICE_MACRO);
                std::string monitor_url
                    = std::string(isIOS ? SNIPPET_SHOW_URL_HTTPS : SNIPPET_SHOW_URL) + "?" + showUrlParam.cipherParam();
                adResult->add_monitor_urls(monitor_url);
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
                bannerJson["xcurl"] = AD_BD_CLICK_MACRO;
                bannerJson["rs"] = queryCondition.flowType == SOLUTION_FLOWTYPE_MOBILE;
                url::URLHelper clickUrlParam;
                getClickPara(clickUrlParam, bidRequest.id(), "", landing_url);
                clickUrlParam.add(URL_IMP_OF, "2");
                std::string clickUrl = std::string(isIOS ? SNIPPET_CLICK_URL_HTTPS : SNIPPET_CLICK_URL) + "?"
                                       + clickUrlParam.cipherParam();
                bannerJson["clickurl"] = clickUrl;
                std::string mtadInfoStr = adservice::utility::json::toJson(bannerJson);
                char admBuffer[4096];
                snprintf(admBuffer, sizeof(admBuffer), adservice::corelogic::HandleShowQueryTask::showAdxTemplate,
                         mtadInfoStr.data());
                adResult->set_html_snippet(admBuffer);
                url::URLHelper showUrlParam;
                getShowPara(showUrlParam, bidRequest.id());
                showUrlParam.add(URL_IMP_OF, "3");
                showUrlParam.addMacro(URL_EXCHANGE_PRICE, AD_BD_PRICE_MACRO);
                std::string monitor_url
                    = std::string(isIOS ? SNIPPET_SHOW_URL_HTTPS : SNIPPET_SHOW_URL) + "?" + showUrlParam.cipherParam();
                adResult->add_monitor_urls(monitor_url);
            }
        }
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
