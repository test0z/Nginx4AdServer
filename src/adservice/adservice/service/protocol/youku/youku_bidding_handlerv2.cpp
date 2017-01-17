//
// Created by guoze.lin on 16/5/3.
//
#include <sstream>

#include "core/core_ad_sizemap.h"
#include "core/core_ip_manager.h"
#include "core/core_typetable.h"
#include "logging.h"
#include "utility/utility.h"
#include "youku_bidding_handlerv2.h"

namespace protocol {
namespace bidding {

    using namespace adservice::utility;
    using namespace adservice::utility::serialize;
    using namespace adservice::utility::json;
    using namespace adservice::utility::userclient;
    using namespace adservice::utility::cypher;
    using namespace adservice::server;

#define AD_YOUKU_CM_URL ""
#define AD_YOUKU_FEED "http://show.mtty.com/v?of=3&p=%s&%s"
#define AD_YOUKU_CLICK "http://click.mtty.com/c?%s"
#define AD_YOUKU_PRICE "${AUCTION_PRICE}"
#define YOUKU_DEVICE_HANDPHONE 0
#define YOUKU_DEVICE_PAD 1
#define YOUKU_DEVICE_PC 2
#define YOUKU_DEVICE_TV 3
#define YOUKU_OS_WINDOWS "Windows"
#define YOUKU_OS_ANDROID "Android"
#define YOUKU_OS_iPhone "ios"
#define YOUKU_OS_MAC "Mac"
#define YOUKU_COOKIEMAPPING_URL "http://c.yes.youku.com?dspid=11214"

    static void fromYoukuDevTypeOsType(int devType,
                                       const std::string & osType,
                                       const std::string & ua,
                                       int & flowType,
                                       int & mobileDev,
                                       int & pcOs,
                                       std::string & pcBrowser)
    {
        if (devType == YOUKU_DEVICE_HANDPHONE) {
            flowType = SOLUTION_FLOWTYPE_MOBILE;
            if (!strcasecmp(osType.data(), YOUKU_OS_ANDROID)) {
                mobileDev = SOLUTION_DEVICE_ANDROID;
            } else if (!strcasecmp(osType.data(), YOUKU_OS_iPhone)) {
                mobileDev = SOLUTION_DEVICE_IPHONE;
            } else
                mobileDev = SOLUTION_DEVICE_OTHER;
        } else if (devType == YOUKU_DEVICE_PAD) {
            flowType = SOLUTION_FLOWTYPE_MOBILE;
            if (!strcasecmp(osType.data(), YOUKU_OS_ANDROID)) {
                mobileDev = SOLUTION_DEVICE_ANDROIDPAD;
            } else if (!strcasecmp(osType.data(), YOUKU_OS_iPhone)) {
                mobileDev = SOLUTION_DEVICE_IPAD;
            } else
                mobileDev = SOLUTION_DEVICE_OTHER;
        } else if (devType == YOUKU_DEVICE_PC) {
            flowType = SOLUTION_FLOWTYPE_PC;
            if (osType.empty()) {
                pcOs = getOSTypeFromUA(ua);
            } else {
                if (!strcasecmp(osType.data(), YOUKU_OS_WINDOWS)) {
                    pcOs = SOLUTION_OS_WINDOWS;
                } else if (!strcasecmp(osType.data(), YOUKU_OS_MAC)) {
                    pcOs = SOLUTION_OS_MAC;
                } else
                    pcOs = SOLUTION_OS_OTHER;
            }
            pcBrowser = getBrowserTypeFromUA(ua);
        }
    }

    static int getNetwork(int network)
    {
        switch (network) {
        case 0:
        case 1:
        case 3:
            return SOLUTION_NETWORK_ALL;
        case 2:
            return SOLUTION_NETWORK_WIFI;
        case 4:
            return SOLUTION_NETWORK_2G;
        case 5:
            return SOLUTION_NETWORK_3G;
        default:
            return SOLUTION_NETWORK_ALL;
        }
    }

    static bool replace(std::string & str, const std::string & from, const std::string & to)
    {
        size_t start_pos = str.find(from);
        if (start_pos == std::string::npos)
            return false;
        str.replace(start_pos, from.length(), to);
        return true;
    }

    bool YoukuBiddingHandler::parseRequestData(const std::string & data)
    {
        return parseJson(data.c_str(), bidRequest);
    }

    bool YoukuBiddingHandler::fillSpecificLog(const AdSelectCondition & selectCondition,
                                              protocol::log::LogItem & logItem, bool isAccepted)
    {
        cppcms::json::value & deviceInfo = bidRequest["device"];
        logItem.userAgent = deviceInfo.get("ua", "");
        logItem.ipInfo.proxy = selectCondition.ip;
        logItem.referer = bidRequest.get("site.ref", "");
        if (isAccepted) {
            cppcms::json::value device;
            device["deviceInfo"] = deviceInfo;
            logItem.deviceInfo = toJson(device);
        }
        return true;
    }

    bool YoukuBiddingHandler::filter(const BiddingFilterCallback & filterCb)
    {
        const cppcms::json::value & imp = bidRequest.find("imp");
        if (imp.is_undefined()) {
            return false;
        }
        const cppcms::json::array & impressions = imp.array();
        if (impressions.empty()) {
            return false;
        }
        std::vector<PreSetAdplaceInfo> adplaceInfos(impressions.size());
        std::vector<AdSelectCondition> queryConditions(impressions.size());
        for (uint32_t i = 0; i < impressions.size(); i++) {
            const cppcms::json::value & adzinfo = impressions[i];
            std::string pid = adzinfo.get<std::string>("tagid");

            AdSelectCondition & queryCondition = queryConditions[i];
            queryCondition.adxid = ADX_YOUKU;
            queryCondition.adxpid = pid;
            queryCondition.basePrice = adzinfo.get<int>("bidfloor", 0);
            adInfo.adxid = ADX_YOUKU;
            bool isNative = false;
            PreSetAdplaceInfo & adplaceInfo = adplaceInfos[i];
            cppcms::json::value adObject = adzinfo.find("banner");
            if (adObject.is_undefined()) {
                adObject = adzinfo.find("video");
                if (adObject.is_undefined()) {
                    adObject = adzinfo.find("native");
                    isNative = true;
                }
            }
            if (!isNative) {
                queryCondition.width = adObject.get("w", 0);
                queryCondition.height = adObject.get("h", 0);
            } else {
                const adservice::utility::AdSizeMap & adSizeMap = adservice::utility::AdSizeMap::getInstance();
                queryCondition.bannerType = BANNER_TYPE_PRIMITIVE;
                const cppcms::json::array & assets = adObject.find("assets").array();
                if (assets.size() > 0) {
                    for (uint32_t i = 0; i < assets.size(); i++) {
                        const cppcms::json::value & asset = assets[i];
                        int w = asset.get("image_url.w", 750);
                        int h = asset.get("image_url.h", 350);
                        auto sizePair = adSizeMap.get({ w, h });
                        queryCondition.width = sizePair.first;
                        queryCondition.height = sizePair.second;
                        adplaceInfo.sizeArray.push_back({ queryCondition.width, queryCondition.height });
                    }
                    queryCondition.pAdplaceInfo = &adplaceInfo;
                } else {
                    continue;
                }
            }
            if (i == 0) {
                cppcms::json::value & device = bidRequest["device"];
                if (!device.is_undefined()) {
                    std::string ip = device.get("ip", "");
                    queryCondition.ip = ip;
                    int devType = device.get("devicetype", YOUKU_DEVICE_PC);
                    std::string osType = device.get("os", "");
                    std::string ua = device.get("ua", "");
                    fromYoukuDevTypeOsType(devType,
                                           osType,
                                           ua,
                                           queryCondition.flowType,
                                           queryCondition.mobileDevice,
                                           queryCondition.pcOS,
                                           queryCondition.pcBrowserStr);
                    if (queryCondition.flowType == SOLUTION_FLOWTYPE_MOBILE) {
                        queryCondition.adxid = ADX_YOUKU_MOBILE;
                        adInfo.adxid = ADX_YOUKU_MOBILE;
                    }
                    queryCondition.mobileNetwork = getNetwork(device.get("connectiontype", 0));
                    cookieMappingKeyMobile(
                        md5_encode(queryCondition.idfa = device.get<std::string>("idfa", "")),
                        md5_encode(queryCondition.imei = device.get<std::string>("imei", "")),
                        md5_encode(queryCondition.androidId = device.get<std::string>("androidid", "")),
                        md5_encode(queryCondition.mac = device.get<std::string>("mac", "")));
                } else {
                    cookieMappingKeyPC(ADX_YOUKU, bidRequest.get("user.id", ""));
                }
                queryCookieMapping(cmInfo.queryKV, queryCondition);
            } else {
                AdSelectCondition & firstQueryCondition = queryConditions[0];
                queryCondition.mtUserId = firstQueryCondition.mtUserId;
                queryCondition.ip = firstQueryCondition.ip;
                queryCondition.flowType = firstQueryCondition.flowType;
                queryCondition.mobileDevice = firstQueryCondition.mobileDevice;
                queryCondition.pcOS = firstQueryCondition.pcOS;
                queryCondition.pcBrowserStr = firstQueryCondition.pcBrowserStr;
                queryCondition.mobileNetwork = firstQueryCondition.mobileNetwork;
                queryCondition.adxid = firstQueryCondition.adxid;
                queryCondition.idfa = firstQueryCondition.idfa;
                queryCondition.mac = firstQueryCondition.mac;
                queryCondition.imei = firstQueryCondition.imei;
            }

            const cppcms::json::value & siteContent = bidRequest.find("site.content.ext");
            const cppcms::json::value & appContent = bidRequest.find("app.content.ext");
            const cppcms::json::value & contentExt = siteContent.is_undefined() ? appContent : siteContent;
            if (!contentExt.is_undefined()) {
                std::string channel = contentExt.get("channel", "");
                std::string cs = contentExt.get("cs", "");
                if (cs == "2070") {
                    channel = cs;
                }
                TypeTableManager & typeTableManager = TypeTableManager::getInstance();
                queryCondition.mttyContentType = typeTableManager.getContentType(ADX_YOUKU, channel);
            }
            isDeal = false;
            const cppcms::json::value & pmp = adzinfo.find("pmp");
            if (!pmp.is_undefined()) {
                // deal 请求
                const cppcms::json::array & deals = pmp.find("deals").array();
                if (!deals.empty()) {
                    std::stringstream ss;
                    ss << ",";
                    for (size_t i = 0; i < deals.size(); ++i) {
                        std::string d = deals[i]["id"].str();
                        ss << d << ",";
                    }
                    queryCondition.dealId = ss.str();
                    isDeal = true;
                }
            }
        }
        if (!filterCb(this, queryConditions)) {
            return bidFailedReturn();
        }
        return isBidAccepted = true;
    }

    static const char * BIDRESPONSE_TEMPLATE = R"(
{
	"id":"",
	"bidid":"",
	"seatbid":[
		{
			"bid":[
			]
		}
	]
})";

    void YoukuBiddingHandler::buildBidResult(const AdSelectCondition & queryCondition,
                                             const MT::common::SelectResult & result, int seq)
    {
        std::string requestId = bidRequest["id"].str();
        if (seq == 0) {
            if (!parseJson(BIDRESPONSE_TEMPLATE, bidResponse)) {
                LOG_ERROR << "in YoukuBiddingHandler::buildBidResult parseJson failed";
                isBidAccepted = false;
                return;
            }
            bidResponse["id"] = requestId;
            bidResponse["bidid"] = "1";
        }
        const cppcms::json::value & adzInfo = bidRequest["imp"].array()[seq];
        const MT::common::Solution & finalSolution = result.solution;
        const MT::common::Banner & banner = result.banner;
        cppcms::json::array & bidArrays = bidResponse["seatbid"][0]["bid"].array();
        cppcms::json::value bidValue;
        bidValue["id"] = "1";
        std::string impId = adzInfo["id"].str();
        bidValue["impid"] = impId;

        //缓存最终广告结果
        fillAdInfo(queryCondition, result, bidRequest.get("user.id", ""));

        std::string cookieMappingUrl = redoCookieMapping(ADX_YOUKU, YOUKU_COOKIEMAPPING_URL);

        bool isIOS = queryCondition.mobileDevice == SOLUTION_DEVICE_IPHONE
                     || queryCondition.mobileDevice == SOLUTION_DEVICE_IPAD;
        // html snippet相关
        std::string strBannerJson = banner.json;
        urlHttp2HttpsIOS(isIOS, strBannerJson);
        cppcms::json::value bannerJson;
        parseJson(strBannerJson.c_str(), bannerJson);
        cppcms::json::array & mtlsArray = bannerJson["mtls"].array();
        std::string tview = bannerJson["tview"].str();

        cppcms::json::value extValue;
        url::URLHelper showUrl;
        getShowPara(showUrl, requestId);
        if (isDeal && finalSolution.dDealId != "0") { // deal 加特殊参数w
            bidValue["dealid"] = finalSolution.dDealId;
            showUrl.add(URL_DEAL_ID, finalSolution.dDealId);
        }
        showUrl.add(URL_IMP_OF, "3");
        showUrl.addMacro(URL_EXCHANGE_PRICE, AD_YOUKU_PRICE);
        bidValue["nurl"] = std::string(isIOS ? SNIPPET_SHOW_URL_HTTPS : SNIPPET_SHOW_URL) + "?" + showUrl.cipherParam();
        std::string crid = std::to_string(adInfo.bannerId);
        bidValue["crid"] = crid;
        std::string landingUrl;
        if (banner.bannerType == BANNER_TYPE_PRIMITIVE) {
            std::string nativeTemplateId = "0";
            const cppcms::json::array & assets = adzInfo.find("native.assets").array();
            const adservice::utility::AdSizeMap & adSizeMap = adservice::utility::AdSizeMap::getInstance();
            for (uint32_t i = 0; i < assets.size(); i++) {
                const cppcms::json::value & asset = assets[i];
                int w = asset.get("image_url.w", 0);
                int h = asset.get("image_url.h", 0);
                auto sizePair = adSizeMap.get({ w, h });
                if (sizePair.first == banner.width && sizePair.second == banner.height) {
                    nativeTemplateId = asset.get("native_template_id", "0");
                    break;
                }
            }
            cppcms::json::value nativeObj;
            nativeObj["native_template_id"] = nativeTemplateId;
            bidValue["native"] = nativeObj;
            landingUrl = mtlsArray[0]["p9"].str();
            replace(landingUrl, "{{click}}", "");
        } else {
            std::string materialUrl = mtlsArray[0]["p0"].str();
            bidValue["adm"] = materialUrl;
            landingUrl = mtlsArray[0]["p1"].str();
        }
        url::URLHelper clickUrlParam;
        getClickPara(clickUrlParam, requestId, "", landingUrl);
        extValue["ldp"]
            = std::string(isIOS ? SNIPPET_CLICK_URL_HTTPS : SNIPPET_CLICK_URL) + "?" + clickUrlParam.cipherParam();
        extValue["pm"] = cppcms::json::array();
        if (!tview.empty()) {
            cppcms::json::array & extPmArray = extValue["pm"].array();
            extPmArray.push_back(tview);
        }
        if (!cookieMappingUrl.empty()) {
            extValue["pm"].array().push_back(cookieMappingUrl);
        }
        int maxCpmPrice = result.bidPrice;
        bidValue["price"] = maxCpmPrice;
        bidValue["ext"] = extValue;
        bidArrays.push_back(std::move(bidValue));
    }

    void YoukuBiddingHandler::match(adservice::utility::HttpResponse & response)
    {
        std::string result = toJson(bidResponse);
        if (result.empty()) {
            LOG_ERROR << "YoukuBiddingHandler::match failed to parse obj to json";
            reject(response);
        } else {
            response.status(200);
            response.set_content_header("application/json; charset=utf-8");
            response.set_body(result);
        }
    }

    void YoukuBiddingHandler::reject(adservice::utility::HttpResponse & response)
    {
        response.status(204);
        response.set_content_header("application/json; charset=utf-8");
    }
}
}
