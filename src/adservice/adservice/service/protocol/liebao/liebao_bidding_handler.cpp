//
// Created by guoze.lin on 16/6/27.
//

#include "liebao_bidding_handler.h"
#include "core/core_ad_sizemap.h"
#include "core/core_ip_manager.h"
#include "logging.h"
#include "utility/utility.h"

namespace protocol {
namespace bidding {

    using namespace adservice::utility;
    using namespace adservice::utility::url;
    using namespace adservice::utility::serialize;
    using namespace adservice::utility::json;
    using namespace adservice::utility::cypher;
    using namespace adservice::server;

#define AD_LIEBAO_SHOW_URL "http://show.mtty.com/v?of=3&p=1500&%s"
#define AD_LIEBAO_CLICK_URL "http://click.mtty.com/c?%s"
#define LIEBAO_NETWORK_WIFI 2

    static int getLiebaoDeviceType(const std::string & platform)
    {
        if (!strncasecmp(platform.data(), "android", platform.length())) {
            return SOLUTION_DEVICE_ANDROID;
        } else if (!strncasecmp(platform.data(), "ios", platform.length())) {
            return SOLUTION_DEVICE_IPHONE;
        } else
            return SOLUTION_DEVICE_OTHER;
    }

    static int getLiebaoOs(const std::string & platform)
    {
        if (!strncasecmp(platform.data(), "android", platform.length())) {
            return SOLUTION_OS_ANDROID;
        } else if (!strncasecmp(platform.data(), "ios", platform.length())) {
            return SOLUTION_OS_IOS;
        } else
            return SOLUTION_OS_OTHER;
    }

    static int getNetwork(int network)
    {
        if (network == LIEBAO_NETWORK_WIFI) {
            return SOLUTION_NETWORK_WIFI;
        } else
            return SOLUTION_NETWORK_ALL;
    }

    bool LieBaoBiddingHandler::parseRequestData(const std::string & data)
    {
        bidResponse.undefined();
        return parseJson(data.c_str(), bidRequest);
    }

    bool LieBaoBiddingHandler::fillSpecificLog(const AdSelectCondition & selectCondition,
                                               protocol::log::LogItem & logItem, bool isAccepted)
    {
        logItem.ipInfo.proxy = selectCondition.ip;
        logItem.userAgent = bidRequest.get("device.ua", "");
        if (isAccepted) {
            const cppcms::json::value & deviceInfo = bidRequest["device"];
            cppcms::json::value device;
            device["deviceinfo"] = deviceInfo;
            logItem.deviceInfo = toJson(device);
            logItem.adInfo.cost = 1500;
        }
        return true;
    }

    bool LieBaoBiddingHandler::filter(const BiddingFilterCallback & filterCb)
    {
        if (!bidRequest.find("test").is_undefined() && bidRequest.get("test", 0) == 1) {
            return false;
        }
        const cppcms::json::value & imp = bidRequest.find("imp");
        if (imp.is_undefined()) {
            return false;
        }
        const cppcms::json::array & impArray = imp.array();
        if (impArray.empty()) {
            return false;
        }
        std::vector<AdSelectCondition> queryConditions(impArray.size());
        std::vector<PreSetAdplaceInfo> adplaceInfos(impArray.size());
        for (uint32_t i = 0; i < impArray.size(); i++) {
            const cppcms::json::value & adzInfo = impArray[i];
            AdSelectCondition & queryCondition = queryConditions[i];
            queryCondition.adxid = ADX_LIEBAO_MOBILE;
            queryCondition.adxpid = adzInfo.get("tagid", "");
            queryCondition.flowType = SOLUTION_FLOWTYPE_MOBILE;
            queryCondition.basePrice = 1500;
            if (i == 0) {
                const cppcms::json::value & device = bidRequest.find("device");
                if (!device.is_undefined()) {
                    queryCondition.mobileDevice = getLiebaoDeviceType(device.get("os", ""));
                    if (queryCondition.mobileDevice == SOLUTION_DEVICE_OTHER) {
                        queryCondition.mobileDevice = userclient::getMobileTypeFromUA(device.get("ua", ""));
                    }
                    queryCondition.pcOS = getLiebaoOs(device.get("os", ""));
                    queryCondition.mobileModel = device.get("model", "");
                    queryCondition.deviceMaker = device.get("make", "");
                    queryCondition.idfa = queryCondition.androidId = stringtool::toupper(device.get("dpidmd5", ""));
                    queryCondition.imei = stringtool::toupper(device.get("imei", ""));
                    queryCondition.mobileNetwork = getNetwork(device.get("connnectiontype", 0));
                    queryCondition.ip = device.get("ip", "");
                    cookieMappingKeyMobile(md5_encode(queryCondition.idfa),
                                           md5_encode(queryCondition.imei),
                                           md5_encode(queryCondition.androidId),
                                           md5_encode(queryCondition.mac));
                    queryCookieMapping(cmInfo.queryKV, queryCondition);
                }
                const cppcms::json::value & site = bidRequest.find("site");
                if (!site.is_undefined()) {
                    queryCondition.keywords.push_back(site.get("keywords", ""));
                }
                const cppcms::json::value & app = bidRequest.find("app");
                if (!app.is_undefined()) {
                    queryCondition.keywords.push_back(app.get("keywords", ""));
                }
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
            PreSetAdplaceInfo & adplaceInfo = adplaceInfos[i];
            adplaceInfo.flowType = SOLUTION_FLOWTYPE_MOBILE;

            const cppcms::json::value & native = adzInfo.find("native");
            if (!native.is_undefined()) {
                std::string request = native.get("request", "{\"native\":{\"assets\":[]}}");
                cppcms::json::value nativeReq;
                parseJson(request.c_str(), nativeReq);
                const cppcms::json::array & assetArray = nativeReq.find("native.assets").array();
                const AdSizeMap & adSizeMap = AdSizeMap::getInstance();
                for (size_t j = 0; j < assetArray.size(); j++) {
                    int w = assetArray[j].get("img.w", 0);
                    int h = assetArray[j].get("img.h", 0);
                    const auto & sizePair = adSizeMap.get({ w, h });
                    queryCondition.width = sizePair.first;
                    queryCondition.height = sizePair.second;
                    adplaceInfo.sizeArray.push_back(sizePair);
                }
                queryCondition.bannerType = BANNER_TYPE_PRIMITIVE;
            }

            if (adplaceInfo.sizeArray.empty()) {
                return isBidAccepted = false;
            }
            queryCondition.pAdplaceInfo = &adplaceInfo;
            const cppcms::json::value & pmp = adzInfo.find("pmp");
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
            "seat":"1",
            "bid":[
            ]
        }
    ]
})";

    static const char * NATIVE_ADM_TEMPLATE = R"({"native":{
                                                "imptrackers":[],
                                                "link":{},
                                                "assets":[]
                                             }
                                             })";

    void LieBaoBiddingHandler::buildBidResult(const AdSelectCondition & queryCondition,
                                              const MT::common::SelectResult & result, int seq)
    {
        std::string requestId = bidRequest["id"].str();
        if (bidResponse.is_undefined()) {
            if (!parseJson(BIDRESPONSE_TEMPLATE, bidResponse)) {
                LOG_ERROR << "in LieBaoBiddingHandler::buildBidResult parseJson failed";
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

        //        std::string cookieMappingUrl = ""; // redoCookieMapping(ADX_YOUKU, YOUKU_COOKIEMAPPING_URL);

        bool isIOS = queryCondition.mobileDevice == SOLUTION_DEVICE_IPHONE
                     || queryCondition.mobileDevice == SOLUTION_DEVICE_IPAD;
        // html snippet相关
        std::string strBannerJson = banner.json;
        urlHttp2HttpsIOS(isIOS, strBannerJson);
        cppcms::json::value bannerJson;
        parseJson(strBannerJson.c_str(), bannerJson);
        cppcms::json::array & mtlsArray = bannerJson["mtls"].array();

        url::URLHelper showUrl;
        getShowPara(showUrl, requestId);
        if (!queryCondition.dealId.empty() && finalSolution.dDealId != "0") { // deal 加特殊参数w
            bidValue["dealid"] = finalSolution.dDealId;
            showUrl.add(URL_DEAL_ID, finalSolution.dDealId);
        }
        showUrl.add(URL_IMP_OF, "3");
        bidValue["nurl"] = std::string(isIOS ? SNIPPET_SHOW_URL_HTTPS : SNIPPET_SHOW_URL) + "?" + showUrl.cipherParam();
        std::string crid = std::to_string(adInfo.bannerId);
        bidValue["crid"] = crid;
        std::string landingUrl;
        if (banner.bannerType == BANNER_TYPE_PRIMITIVE) {
            std::string request = adzInfo.get("native.request", "{\"native\":{\"assets\":[]}}");
            cppcms::json::value nativeReq;
            json::parseJson(request.c_str(), nativeReq);
            const cppcms::json::array & assets = nativeReq.find("native.assets").array();
            const adservice::utility::AdSizeMap & adSizeMap = adservice::utility::AdSizeMap::getInstance();
            cppcms::json::value admObj;
            json::parseJson(NATIVE_ADM_TEMPLATE, admObj);
            cppcms::json::array & admAssetsArray = admObj["native"]["assets"].array();
            cppcms::json::value outputAsset;
            outputAsset["img"] = cppcms::json::value();
            for (uint32_t i = 0; i < assets.size(); i++) {
                const cppcms::json::value & asset = assets[i];
                int w = asset.get("img.w", 0);
                int h = asset.get("img.h", 0);
                auto sizePair = adSizeMap.get({ w, h });
                if (sizePair.first == banner.width && sizePair.second == banner.height) {
                    outputAsset["id"] = asset.get("id", 0);
                    outputAsset["img"]["w"] = w;
                    outputAsset["img"]["h"] = h;
                    bidValue["w"] = w;
                    bidValue["h"] = h;
                    outputAsset["img"]["url"] = mtlsArray[0]["p6"].str();
                    break;
                }
            }

            landingUrl = mtlsArray[0]["p9"].str();
            url_replace(landingUrl, "{{click}}", "");
            adservice::utility::url::url_replace(landingUrl, "https://", "http://");
            admAssetsArray.push_back(outputAsset);
            admObj["link"]["url"] = landingUrl;
            admObj["link"]["clicktrackers"] = cppcms::json::array();
            url::URLHelper clickUrlParam;
            getClickPara(clickUrlParam, requestId, "", landingUrl);
            admObj["link"]["clicktrackers"].array().push_back(
                std::string(isIOS ? SNIPPET_CLICK_URL_HTTPS : SNIPPET_CLICK_URL) + "?" + clickUrlParam.cipherParam());
            std::string admObjectStr = json::toJson(admObj);
            bidValue["adm"] = admObjectStr;
        }

        int maxCpmPrice = result.bidPrice;
        bidValue["price"] = maxCpmPrice;
        bidValue["bundle"] = "com.liebao.hell";
        bidArrays.push_back(std::move(bidValue));
        redoCookieMapping(queryCondition.adxid, "");
    }

    void LieBaoBiddingHandler::match(adservice::utility::HttpResponse & response)
    {
        std::string result = toJson(bidResponse);
        if (result.empty()) {
            LOG_ERROR << "LiebaoBiddingHandler::match failed to parse obj to json";
            reject(response);
        } else {
            response.status(200);
            response.set_content_header("application/json; charset=utf-8");
            response.set_body(result);
        }
    }

    void LieBaoBiddingHandler::reject(adservice::utility::HttpResponse & response)
    {
        response.status(200);
        response.set_content_header("application/json; charset=utf-8");
    }
}
}
