
#include <sstream>

#include "core/core_ad_sizemap.h"
#include "core/core_ip_manager.h"
#include "core/core_typetable.h"
#include "inmobi_bidding_handler.h"
#include "logging.h"
#include "utility/utility.h"

namespace protocol {
namespace bidding {

    using namespace adservice::utility;
    using namespace adservice::utility::serialize;
    using namespace adservice::utility::json;
    using namespace adservice::utility::userclient;
    using namespace adservice::utility::cypher;
    using namespace adservice::server;

#define INMOBI_DEV_MOBILEORPAD 1
#define INMOBI_DEV_PC 2
#define INMOBI_DEV_TV 3
#define INMOBI_DEV_PHONE 4
#define INMOBI_DEV_TABLET 5
#define INMOBI_DEV_CONNECTEDDEVICE 6
#define INMOBI_DEV_SETTOPBOX 7

#define INMOBI_NETWORK_UNKNOWN 0
#define INMOBI_NETWORK_ETHERNET 1
#define INMOBI_NETWORK_WIFI 2
#define INMOBI_NETWORK_CELLULAR_UNKNOWN 3
#define INMOBI_NETWORK_CELLULAR_2G 4
#define INMOBI_NETWORK_CELLULAR_3G 5
#define INMOBI_NETWORK_CELLULAR_4G 6

#define INMOBI_OS_ANDROID "android"
#define INMOBI_OS_IOS "ios"
#define INMOBI_OS_WINDOWS "windows"
#define INMOBI_OS_MAC "mac"

#define INMOBI_PRIVATE_AUCTION 1

#define INMOBI_PRICE_MACRO "${AUCTION_PRICE}"
#define AD_INMOBI_FEED "http://show.mtty.com/v?of=3&p=%s&%s"

    namespace {
        void fromInmobiDevTypeOsType(int devType,
                                     const std::string & osType,
                                     const std::string & ua,
                                     int & flowType,
                                     int & mobileDev,
                                     int & pcOs,
                                     std::string & pcBrowser)
        {
            if (devType == INMOBI_DEV_PHONE) {
                flowType = SOLUTION_FLOWTYPE_MOBILE;
                if (!strcasecmp(osType.data(), INMOBI_OS_ANDROID)) {
                    mobileDev = SOLUTION_DEVICE_ANDROID;
                } else if (!strcasecmp(osType.data(), INMOBI_OS_IOS)) {
                    mobileDev = SOLUTION_DEVICE_IPHONE;
                } else
                    mobileDev = SOLUTION_DEVICE_OTHER;
            } else if (devType == INMOBI_DEV_TABLET || devType == INMOBI_DEV_MOBILEORPAD) {
                flowType = SOLUTION_FLOWTYPE_MOBILE;
                if (!strcasecmp(osType.data(), INMOBI_OS_ANDROID)) {
                    mobileDev = SOLUTION_DEVICE_ANDROIDPAD;
                } else if (!strcasecmp(osType.data(), INMOBI_OS_IOS)) {
                    mobileDev = SOLUTION_DEVICE_IPAD;
                } else
                    mobileDev = SOLUTION_DEVICE_OTHER;
            } else if (devType == INMOBI_DEV_PC) {
                flowType = SOLUTION_FLOWTYPE_PC;
                if (osType.empty()) {
                    pcOs = getOSTypeFromUA(ua);
                } else {
                    if (!strcasecmp(osType.data(), INMOBI_OS_WINDOWS)) {
                        pcOs = SOLUTION_OS_WINDOWS;
                    } else if (!strcasecmp(osType.data(), INMOBI_OS_MAC)) {
                        pcOs = SOLUTION_OS_MAC;
                    } else
                        pcOs = SOLUTION_OS_OTHER;
                }
                pcBrowser = getBrowserTypeFromUA(ua);
            } else { // connected device or set-top box or tv
                flowType = SOLUTION_FLOWTYPE_MOBILE;
                if (devType == INMOBI_DEV_TV) {
                    mobileDev = SOLUTION_DEVICE_TV;
                } else {
                    mobileDev = SOLUTION_DEVICE_OTHER;
                }
            }
        }

        int getInmobiNetwork(int network)
        {
            switch (network) {
            case INMOBI_NETWORK_WIFI:
                return SOLUTION_NETWORK_WIFI;
            case INMOBI_NETWORK_CELLULAR_2G:
                return SOLUTION_NETWORK_2G;
            case INMOBI_NETWORK_CELLULAR_3G:
                return SOLUTION_NETWORK_3G;
            case INMOBI_NETWORK_CELLULAR_4G:
                return SOLUTION_NETWORK_4G;
            default:
                return SOLUTION_NETWORK_ALL;
            }
        }

        bool replace(std::string & str, const std::string & from, const std::string & to)
        {
            size_t start_pos = str.find(from);
            if (start_pos == std::string::npos)
                return false;
            str.replace(start_pos, from.length(), to);
            return true;
        }
    }
    bool InmobiBiddingHandler::parseRequestData(const std::string & data)
    {
        return parseJson(data.c_str(), bidRequest);
    }

    bool InmobiBiddingHandler::fillSpecificLog(const AdSelectCondition & selectCondition,
                                               protocol::log::LogItem & logItem, bool isAccepted)
    {
        cppcms::json::value & deviceInfo = bidRequest["device"];
        logItem.userAgent = deviceInfo.get("ua", "");
        logItem.ipInfo.proxy = selectCondition.ip;
        if (isAccepted) {
            cppcms::json::value device;
            device["deviceInfo"] = deviceInfo;
            logItem.deviceInfo = toJson(device);
            logItem.referer = bidRequest.get("site.ref", "");
        }
        return true;
    }

    bool InmobiBiddingHandler::filter(const BiddingFilterCallback & filterCb)
    {
        const cppcms::json::value & imp = bidRequest.find("imp");
        if (imp.is_undefined()) {
            return false;
        }
        const cppcms::json::array & impressions = imp.array();
        if (impressions.empty()) {
            return false;
        }

        const cppcms::json::value & adzinfo = impressions[0];
        std::vector<AdSelectCondition> queryConditions{ AdSelectCondition() };
        AdSelectCondition & queryCondition = queryConditions[0];
        queryCondition.adxid = ADX_INMOBI;
        double priceFloor = adzinfo.get<double>("bidfloor", 0.0);
        queryCondition.basePrice = (int)std::ceil(priceFloor);
        PreSetAdplaceInfo adplaceInfo;
        bool isNative = false;
        cppcms::json::value adTypeObj = adzinfo.find("banner");
        if (adTypeObj.is_undefined()) {
            adTypeObj = adzinfo.find("video");
            if (adTypeObj.is_undefined()) {
                adTypeObj = adzinfo.find("native");
                isNative = true;
            }
        }
        if (!isNative) {
            queryCondition.width = adTypeObj.get("w", 0);
            queryCondition.height = adTypeObj.get("h", 0);
        } else {
            const adservice::utility::AdSizeMap & adSizeMap = adservice::utility::AdSizeMap::getInstance();
            queryCondition.bannerType = BANNER_TYPE_PRIMITIVE;
            const cppcms::json::array & assets = adTypeObj.find("requestobj.assets").array();
            if (assets.size() > 0) {
                for (uint32_t i = 0; i < assets.size(); i++) {
                    const cppcms::json::value & asset = assets[i];
                    int w = asset.get("img.wmin", 0);
                    int h = asset.get("img.hmin", 0);
                    auto sizePair = adSizeMap.get({ w, h });
                    queryCondition.width = sizePair.first;
                    queryCondition.height = sizePair.second;
                    adplaceInfo.sizeArray.push_back({ queryCondition.width, queryCondition.height });
                }
                queryCondition.pAdplaceInfo = &adplaceInfo;
            } else {
                return false;
            }
        }
        const cppcms::json::value & device = bidRequest.find("device");
        if (!device.is_undefined()) {
            std::string ip = device.get("ip", "");
            queryCondition.ip = ip;
            int devType = device.get("devicetype", INMOBI_DEV_MOBILEORPAD);
            std::string osType = device.get("os", "");
            std::string ua = device.get("ua", "");
            fromInmobiDevTypeOsType(devType,
                                    osType,
                                    ua,
                                    queryCondition.flowType,
                                    queryCondition.mobileDevice,
                                    queryCondition.pcOS,
                                    queryCondition.pcBrowserStr);
            queryCondition.mobileNetwork = getInmobiNetwork(device.get("carrier", 0));
            queryCondition.idfa = device.get("idfa", "");
            queryCondition.imei = device.get("didmd5", "");
            queryCondition.androidId = device.get("dpidmd5", "");
            cookieMappingKeyMobile(md5_encode(queryCondition.idfa),
                                   md5_encode(queryCondition.imei),
                                   md5_encode(queryCondition.androidId),
                                   md5_encode(queryCondition.mac));
            queryCookieMapping(cmInfo.queryKV, queryCondition);
        }
        const cppcms::json::value & siteContent = bidRequest.find("site");
        const cppcms::json::value & appContent = bidRequest.find("app");
        const cppcms::json::value & contentExt = siteContent.is_undefined() ? appContent : siteContent;
        if (!contentExt.is_undefined()) {
            queryCondition.adxpid = contentExt.get("id", "");
            const cppcms::json::array & cats = contentExt.find("cat").array();
            if (!cats.empty()) {
                TypeTableManager & typeTableManager = TypeTableManager::getInstance();
                queryCondition.mttyContentType = typeTableManager.getContentType(ADX_INMOBI, cats[0].str());
            }
        }
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
          "seat":"56abf687dd6c419886c841240815ceb9",
          "bid":[]
        }
    ],
    "cur":"CNY"
})";

    void InmobiBiddingHandler::buildBidResult(const AdSelectCondition & queryCondition,
                                              const MT::common::SelectResult & result, int seq)
    {
        std::string requestId = bidRequest["id"].str();
        if (seq == 0) {
            if (!parseJson(BIDRESPONSE_TEMPLATE, bidResponse)) {
                LOG_ERROR << "in InmobiBiddingHandler::buildBidResult parseJson failed";
                isBidAccepted = false;
                return;
            }
            std::string requestId = bidRequest["id"].str();
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

        bool isIOS = queryCondition.mobileDevice == SOLUTION_DEVICE_IPAD
                     || queryCondition.mobileDevice == SOLUTION_DEVICE_IPHONE;

        // html snippet相关
        std::string strBannerJson = banner.json;
        urlHttp2HttpsIOS(isIOS, strBannerJson);
        // tripslash2(pjson);
        cppcms::json::value bannerJson;
        parseJson(strBannerJson.c_str(), bannerJson);

        url::URLHelper showUrl;
        getShowPara(showUrl, requestId);
        if (!queryCondition.dealId.empty() && finalSolution.dDealId != "0") { // deal 加特殊参数w
            bidValue["dealid"] = finalSolution.dDealId;
            showUrl.add(URL_DEAL_ID, finalSolution.dDealId);
        }
        showUrl.add(URL_IMP_OF, "3");
        showUrl.addMacro(URL_EXCHANGE_PRICE, INMOBI_PRICE_MACRO);
        bidValue["nurl"] = std::string(isIOS ? SNIPPET_SHOW_URL_HTTPS : SNIPPET_SHOW_URL) + "?" + showUrl.cipherParam();
        std::string crid = std::to_string(adInfo.bannerId);
        bidValue["crid"] = crid;
        if (banner.bannerType != BANNER_TYPE_PRIMITIVE) { //非原生创意
            int w = banner.width;
            int h = banner.height;
            std::string html = generateHtmlSnippet(requestId, w, h, "of=2&", "", isIOS);
            bidValue["adm"] = html;
        } else { //设置admobject
            const cppcms::json::array & mtlsArray = bannerJson["mtls"].array();
            const cppcms::json::array & assets = adzInfo.find("native.assets").array();
            std::string title = mtlsArray[0]["p0"].str();
            cppcms::json::value admObject;
            cppcms::json::value nativeObject;
            cppcms::json::array assetsArray;
            for (uint32_t i = 0; i < assets.size(); i++) {
                const cppcms::json::value & asset = assets[i];
                int w = asset.get("img.w", 0);
                int h = asset.get("img.h", 0);
                uint32_t titleLen = asset.get("title.len", 0);
                if (w == banner.width && h == banner.height && title.length() <= titleLen) {
                    cppcms::json::value assetObj;
                    assetObj["id"] = 1;
                    assetObj["img.w"] = banner.width;
                    assetObj["img.h"] = banner.height;
                    assetsArray.push_back(assetObj);
                    break;
                }
            }
            std::string landingUrl = mtlsArray[0]["p9"].str();
            replace(landingUrl, "{{click}}", "");
            nativeObject["assets"] = assetsArray;
            nativeObject["link"] = cppcms::json::value();
            nativeObject["link"]["url"] = landingUrl;
            admObject["native"] = nativeObject;
            bidValue["admobject"] = admObject;
        }
        bidValue["adomain"] = cppcms::json::array();
        bidValue["adomain"].array().push_back("http://mtty.com");
        bidValue["iurl"] = "";
        cppcms::json::array attrArray;
        attrArray.push_back(3);
        bidValue["attr"] = attrArray;
        int maxCpmPrice = result.bidPrice;
        bidValue["price"] = maxCpmPrice / 100.0;
        bidArrays.push_back(std::move(bidValue));
        redoCookieMapping(queryCondition.adxid, "");
    }

    void InmobiBiddingHandler::match(adservice::utility::HttpResponse & response)
    {
        std::string result = toJson(bidResponse);
        if (result.empty()) {
            LOG_ERROR << "InmobiBiddingHandler::match failed to parse obj to json";
            reject(response);
        } else {
            response.status(200);
            response.set_content_header("application/json; charset=utf-8");
            response.set_body(result);
        }
    }

    void InmobiBiddingHandler::reject(adservice::utility::HttpResponse & response)
    {
        response.status(204);
        response.set_content_header("application/json; charset=utf-8");
    }
}
}
