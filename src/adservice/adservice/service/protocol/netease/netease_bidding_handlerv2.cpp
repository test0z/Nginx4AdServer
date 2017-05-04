//
// Created by guoze.lin on 16/6/27.
//

#include "netease_bidding_handlerv2.h"
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

#define AD_NETEASE_SHOW_URL "http://show.mtty.com/v?of=3&p=1500&%s"
#define AD_NETEASE_CLICK_URL "http://click.mtty.com/c?%s"

    static NetEaseAdplaceStyleMap adplaceStyleMap;

    int getNetEaseDeviceType(const std::string & platform)
    {
        if (!strncasecmp(platform.data(), "android", platform.length())) {
            return SOLUTION_DEVICE_ANDROID;
        } else if (!strncasecmp(platform.data(), "ios", platform.length())) {
            return SOLUTION_DEVICE_IPHONE;
        } else
            return SOLUTION_DEVICE_OTHER;
    }

    int getNetEaseOsType(const std::string & platform)
    {
        if (!strncasecmp(platform.data(), "android", platform.length())) {
            return SOLUTION_OS_ANDROID;
        } else if (!strncasecmp(platform.data(), "ios", platform.length())) {
            return SOLUTION_OS_IOS;
        } else
            return SOLUTION_OS_OTHER;
    }

    static int getNetwork(const std::string & network)
    {
        if (network == "wifi") {
            return SOLUTION_NETWORK_WIFI;
        } else
            return SOLUTION_NETWORK_ALL;
    }

    bool NetEaseBiddingHandler::parseRequestData(const std::string & data)
    {
        bidResponse.undefined();
        return parseJson(data.c_str(), bidRequest);
    }

    bool NetEaseBiddingHandler::fillSpecificLog(const AdSelectCondition & selectCondition,
                                                protocol::log::LogItem & logItem, bool isAccepted)
    {
        logItem.ipInfo.proxy = selectCondition.ip;
        if (isAccepted) {
            const cppcms::json::value & deviceInfo = bidRequest["device"];
            cppcms::json::value device;
            device["deviceinfo"] = deviceInfo;
            logItem.deviceInfo = toJson(device);
            logItem.adInfo.cost = 1500;
        }
        return true;
    }

    bool NetEaseBiddingHandler::filter(const BiddingFilterCallback & filterCb)
    {
        if (!bidRequest.find("is_test").is_undefined() && bidRequest["is_test"].boolean()) {
            return false;
        }
        cppcms::json::value & adzinfo = bidRequest["adunit"];
        std::string pid = adzinfo.get("space_id", "0");
        std::vector<AdSelectCondition> queryConditions{ AdSelectCondition() };
        AdSelectCondition & queryCondition = queryConditions[0];
        queryCondition.isFromSSP = true;
        queryCondition.adxid = ADX_NETEASE_MOBILE;
        queryCondition.adxpid = pid;
        const cppcms::json::value & geo = bidRequest.find("geo");
        if (!geo.is_undefined()) { //"ip"
            const cppcms::json::value & geoIp = geo.find("ip");
            if (!geoIp.is_undefined()) {
                queryCondition.ip = geoIp.str();
            }
        } else { //网易接口并没有给我们传ip
            queryCondition.ip = "";
        }
        queryCondition.flowType = SOLUTION_FLOWTYPE_MOBILE;
        queryCondition.basePrice = 1500;
        std::string platform = adzinfo.get("platform", "");
        queryCondition.mobileDevice = getNetEaseDeviceType(platform);
        const cppcms::json::value & device = bidRequest.find("device");
        if (!device.is_undefined()) {
            queryCondition.idfa = stringtool::toupper(device.get("idfa", ""));
            queryCondition.imei = stringtool::toupper(device.get("imei", ""));
            queryCondition.mac = stringtool::toupper(device.get("mac", ""));
            queryCondition.pcOS = getNetEaseOsType(device.get("os", ""));
            const cppcms::json::value & networkStatus = device.find("network_status");
            if (!networkStatus.is_undefined()) {
                queryCondition.mobileNetwork = getNetwork(networkStatus.str());
            }
            cookieMappingKeyMobile(md5_encode(queryCondition.idfa),
                                   md5_encode(queryCondition.imei),
                                   md5_encode(queryCondition.androidId),
                                   md5_encode(queryCondition.mac),
                                   queryCondition);
            queryCookieMapping(cmInfo.queryKV, queryCondition);
        }
        PreSetAdplaceInfo adplaceInfo;
        adplaceInfo.flowType = SOLUTION_FLOWTYPE_MOBILE;
        const std::string & strStyles = adzinfo.get("style", "3,10,11,13");
        std::vector<int64_t> styles;
        MT::common::string2vecint(strStyles, styles);
        if (styles.empty()) {
            return isBidAccepted = false;
        }
        for (int64_t style : styles) {
            if (adplaceStyleMap.find(style)) {
                const auto & sizeStyle = adplaceStyleMap.get(style);
                for (auto size : sizeStyle.sizes) {
                    adplaceInfo.sizeArray.push_back(std::make_pair(size.first, size.second));
                    queryCondition.width = size.first;
                    queryCondition.height = size.second;
                }
            }
        }
        if (adplaceInfo.sizeArray.empty()) {
            return isBidAccepted = false;
        }
        queryCondition.pAdplaceInfo = &adplaceInfo;

        if (!filterCb(this, queryConditions)) {
            return bidFailedReturn();
        }
        return isBidAccepted = true;
    }

    static const char * BIDRESPONSE_TEMPLATE = R"(
{
	"mainTitle":"",
	"subTitle":"",
	"monitor":"",
	"showMonitorUrl":"",
	"clickMonitorUrl":"",
	"valid_time":0,
	"content":"",
	"resource_url":[],
	"linkUrl":""
})";

    void NetEaseBiddingHandler::buildBidResult(const AdSelectCondition & queryCondition,
                                               const MT::common::SelectResult & result, int seq)
    {
        if (bidResponse.is_undefined()) {
            if (!parseJson(BIDRESPONSE_TEMPLATE, bidResponse)) {
                LOG_ERROR << "in NetEaseBiddingHandler::buildBidResult parseJson failed";
                isBidAccepted = false;
                return;
            }
        }

        const MT::common::Banner & banner = result.banner;
        //缓存最终广告结果
        fillAdInfo(queryCondition, result, "");

        bool isIOS = queryCondition.mobileDevice == SOLUTION_DEVICE_IPHONE
                     || queryCondition.mobileDevice == SOLUTION_DEVICE_IPAD;
        // html snippet相关
        std::string requestId = bidRequest["id"].str();

        int bannerType = banner.bannerType;
        std::string strBannerJson = banner.json;
        cppcms::json::value bannerJson = bannerJson2HttpsIOS(isIOS, strBannerJson, banner.bannerType);
        const cppcms::json::array & mtlsArray = bannerJson["mtls"].array();
        std::string mainTitle;
        std::string landingUrl;
        std::string downloadUrl;
        URLHelper showUrlParam;
        getShowPara(showUrlParam, requestId);
        showUrlParam.add(URL_IMP_OF, "3");
        showUrlParam.add(URL_EXCHANGE_PRICE, "1500");
        bidResponse["showMonitorUrl"] = getShowBaseUrl(isIOS) + "?" + showUrlParam.cipherParam();
        std::vector<std::string> materialUrls;
        if (bannerType == BANNER_TYPE_PRIMITIVE) {
            materialUrls.push_back(mtlsArray[0].get("p6", ""));
            materialUrls.push_back(mtlsArray[0].get("p7", ""));
            materialUrls.push_back(mtlsArray[0].get("p8", ""));
            landingUrl = mtlsArray[0].get("p9", "");
            if (queryCondition.mobileDevice == SOLUTION_DEVICE_IPHONE) {
                downloadUrl = mtlsArray[0].get("p10", "");
            } else if (queryCondition.mobileDevice == SOLUTION_DEVICE_ANDROID) {
                downloadUrl = mtlsArray[0].get("p11", "");
            }
            int style = 0;
            auto & sizeStyleMap = adplaceStyleMap.getSizeStyleMap();
            for (auto iter = sizeStyleMap.find(std::make_pair(banner.width, banner.height));
                 iter != sizeStyleMap.end() && (style = iter->second) && false;)
                ;

            if (style == 3) {
                mainTitle = mtlsArray[0].get("p1", "");
                std::string bakMainTitle = mtlsArray[0].get("p0", "");
                if (mainTitle.length() < bakMainTitle.length()) {
                    mainTitle = bakMainTitle;
                }
            } else {
                mainTitle = mtlsArray[0].get("p0", "");
            }
            bidResponse["style"] = std::to_string(style);
            if (style == 13) {
                bidResponse["videoUrl"] = mtlsArray[0].get("p16", "");
            }
        } else {
            std::string materialUrl = mtlsArray[0].get("p0", "");
            materialUrls.push_back(materialUrl);
            landingUrl = mtlsArray[0].get("p1", "");
        }
        bidResponse["mainTitle"] = mainTitle;
        bidResponse["valid_time"] = 86400000;
        url::URLHelper clickUrlParam;
        getClickPara(clickUrlParam, requestId, "", landingUrl);
        bidResponse["linkUrl"] = getClickBaseUrl(isIOS) + "?" + clickUrlParam.cipherParam();
        cppcms::json::array & resArray = bidResponse["resource_url"].array();
        for (auto & murl : materialUrls) {
            if (!murl.empty())
                resArray.push_back(cppcms::json::value(murl));
        }
        redoCookieMapping(queryCondition.adxid, "");
    }

    void NetEaseBiddingHandler::match(adservice::utility::HttpResponse & response)
    {
        std::string result = toJson(bidResponse);
        if (result.empty()) {
            LOG_ERROR << "NetEaseBiddingHandler::match failed to parse obj to json";
            reject(response);
        } else {
            response.status(200);
            response.set_content_header("application/json; charset=utf-8");
            response.set_body(result);
        }
    }

    void NetEaseBiddingHandler::reject(adservice::utility::HttpResponse & response)
    {
        response.status(200);
        response.set_content_header("application/json; charset=utf-8");
    }
}
}
