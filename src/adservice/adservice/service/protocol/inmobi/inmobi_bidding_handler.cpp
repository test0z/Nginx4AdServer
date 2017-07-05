
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
                    pcOs = SOLUTION_OS_ANDROID;
                } else if (!strcasecmp(osType.data(), INMOBI_OS_IOS)) {
                    mobileDev = SOLUTION_DEVICE_IPHONE;
                    pcOs = SOLUTION_OS_IOS;
                } else
                    mobileDev = SOLUTION_DEVICE_OTHER;
            } else if (devType == INMOBI_DEV_TABLET || devType == INMOBI_DEV_MOBILEORPAD) {
                flowType = SOLUTION_FLOWTYPE_MOBILE;
                if (!strcasecmp(osType.data(), INMOBI_OS_ANDROID)) {
                    mobileDev = SOLUTION_DEVICE_ANDROIDPAD;
                    pcOs = SOLUTION_OS_ANDROID;
                } else if (!strcasecmp(osType.data(), INMOBI_OS_IOS)) {
                    mobileDev = SOLUTION_DEVICE_IPAD;
                    pcOs = SOLUTION_OS_IOS;
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

        const int FLOW_BANNER = 0;
        const int FLOW_NATIVE = 1;
        const int FLOW_VIDEO = 2;
    }
    bool InmobiBiddingHandler::parseRequestData(const std::string & data)
    {
        bidResponse.undefined();
        return parseJson(data.c_str(), bidRequest);
    }

    bool InmobiBiddingHandler::fillSpecificLog(const AdSelectCondition & selectCondition,
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
        queryCondition.basePrice = (int)std::ceil(priceFloor * 100);
        PreSetAdplaceInfo adplaceInfo;
        int reqType = FLOW_BANNER;
        cppcms::json::value adTypeObj = adzinfo.find("banner");
        if (adTypeObj.is_undefined()) {
            adTypeObj = adzinfo.find("video");
            if (adTypeObj.is_undefined()) {
                adTypeObj = adzinfo.find("native");
                reqType = FLOW_NATIVE;
            } else {
                reqType = FLOW_VIDEO;
            }
        }
        if (reqType == FLOW_BANNER) {
            queryCondition.width = adTypeObj.get("w", 0);
            queryCondition.height = adTypeObj.get("h", 0);
        } else if (reqType == FLOW_NATIVE) {
            const adservice::utility::AdSizeMap & adSizeMap = adservice::utility::AdSizeMap::getInstance();
            queryCondition.bannerType = BANNER_TYPE_PRIMITIVE;
            const cppcms::json::array & assets = adTypeObj.find("requestobj.assets").array();
            if (assets.size() > 0) {
                for (uint32_t i = 0; i < assets.size(); i++) {
                    const cppcms::json::value & asset = assets[i];
                    int type = asset.get("img.type", 500);
                    int w = asset.get("img.wmin", 0);
                    int h = asset.get("img.hmin", 0);
                    if (w != 0 && h != 0 && type == 3) {
                        auto sizePair = adSizeMap.get({ w, h });
                        for (auto & sizeIter : sizePair) {
                            queryCondition.width = sizeIter.first;
                            queryCondition.height = sizeIter.second;
                            adplaceInfo.sizeArray.push_back({ queryCondition.width, queryCondition.height });
                        }
                    }
                }
                queryCondition.pAdplaceInfo = &adplaceInfo;
            } else {
                return false;
            }
        } else if (reqType == FLOW_VIDEO) {
            queryCondition.width = adTypeObj.get("w", 0);
            queryCondition.height = adTypeObj.get("h", 0);
            queryCondition.bannerType = BANNER_TYPE_VIDEO;
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
            queryCondition.deviceBrand = adservice::utility::userclient::getDeviceBrandFromUA(ua);
            queryCondition.geo = { device.get("lon", 0.0), device.get("lat", 0.0) };
            queryCondition.mobileModel = device.get("model", "");
            queryCondition.deviceMaker = device.get("make", "");
            queryCondition.mobileNetwork = getInmobiNetwork(device.get("connectiontype", 0));
            queryCondition.idfa = stringtool::toupper(device.get("ext.idfa", ""));
            queryCondition.imei = stringtool::toupper(device.get("didmd5", ""));
            queryCondition.androidId = stringtool::toupper(device.get("dpidmd5", ""));
            cookieMappingKeyMobile(md5_encode(queryCondition.idfa),
                                   queryCondition.imei,
                                   queryCondition.androidId,
                                   md5_encode(queryCondition.mac),
                                   queryCondition,
                                   queryCondition.adxid,
                                   bidRequest.get("user.id", ""));
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
        if (bidResponse.is_undefined()) {
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
        cppcms::json::value bannerJson = bannerJson2HttpsIOS(isIOS, strBannerJson, banner.bannerType);
        const cppcms::json::array & mtlsArray = bannerJson["mtls"].array();
        url::URLHelper showUrl;
        getShowPara(showUrl, requestId);
        if (!queryCondition.dealId.empty() && finalSolution.dDealId != "0") { // deal 加特殊参数w
            bidValue["dealid"] = finalSolution.dDealId;
            showUrl.add(URL_DEAL_ID, finalSolution.dDealId);
        }
        showUrl.add(URL_IMP_OF, "3");
        showUrl.addMacro(URL_EXCHANGE_PRICE, INMOBI_PRICE_MACRO);
        bidValue["nurl"] = getShowBaseUrl(isIOS) + "?" + showUrl.cipherParam();
        std::string crid = std::to_string(adInfo.bannerId);
        bidValue["crid"] = crid;
        if (banner.bannerType == BANNER_TYPE_VIDEO) { //视频创意
            std::string landingUrl = mtlsArray[0]["p1"].str();
            url::URLHelper clickUrlParam;
            getClickPara(clickUrlParam, requestId, "", landingUrl);
            std::string clickUrl = getClickBaseUrl(isIOS) + "?" + clickUrlParam.cipherParam();
            bidValue["adm"] = prepareVast(banner, mtlsArray[0].get("p0", ""), "", clickUrl,
                                          stringtool::safeconvert(stringtool::stol, mtlsArray[0].get("p7", "15")));
        } else if (banner.bannerType != BANNER_TYPE_PRIMITIVE) { //普通图片创意或HTML创意
            int w = banner.width;
            int h = banner.height;
            std::string html = generateHtmlSnippet(requestId, w, h, "of=2&", "", isIOS);
            bidValue["adm"] = html;
        } else { //原生创意设置admobject
            const cppcms::json::array & assets = adzInfo.find("native.requestobj.assets").array();
            cppcms::json::value admObject;
            cppcms::json::value nativeObject;
            cppcms::json::array assetsArray;
            // const adservice::utility::AdSizeMap & adSizeMap = adservice::utility::AdSizeMap::getInstance();
            for (uint32_t i = 0; i < assets.size(); i++) {
                const cppcms::json::value & asset = assets[i];
                int w = asset.get("img.wmin", 0);
                int h = asset.get("img.hmin", 0);
                int imageType = asset.get("img.type", 500);
                cppcms::json::value assetObj;
                assetObj["id"] = asset.get("id", 1);
                if (w != 0 && h != 0) {                     // image
                    if (imageType == 1 || imageType == 2) { // logo or icon
                        assetObj["img"] = cppcms::json::value();
                        assetObj["img"]["url"] = mtlsArray[0].get("p15", "");
                    } else if (imageType == 3) { // main
                        assetObj["img"] = cppcms::json::value();
                        assetObj["img"]["w"] = w;
                        assetObj["img"]["h"] = h;
                        assetObj["img"]["url"] = mtlsArray[0].get("p6", "");
                    }
                } else if (!asset.find("data").is_undefined() && asset.get("data.type", 0) == 2) { // description
                    assetObj["data"] = cppcms::json::value();
                    assetObj["data"]["value"] = mtlsArray[0].get("p5", "");
                } else if (!asset.find("title").is_undefined()) { // title
                    assetObj["title"] = cppcms::json::value();
                    assetObj["title"]["text"] = mtlsArray[0].get("p0", "");
                } else {
                    assetObj["data"] = cppcms::json::value();
                    assetObj["data"]["value"] = "";
                }
                assetsArray.push_back(assetObj);
            }
            std::string landingUrl = mtlsArray[0]["p9"].str();
            nativeObject["assets"] = assetsArray;
            nativeObject["link"] = cppcms::json::value();
            url::URLHelper clickUrlParam;
            getClickPara(clickUrlParam, requestId, "", landingUrl);
            std::string clickUrl = getClickBaseUrl(isIOS) + "?" + clickUrlParam.cipherParam();
            nativeObject["link"]["url"] = clickUrl;
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
