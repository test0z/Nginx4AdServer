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
    using namespace adservice::utility::serialize;
    using namespace adservice::utility::json;
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

    static int getNetwork(const std::string & network)
    {
        if (network == "wifi") {
            return SOLUTION_NETWORK_WIFI;
        } else
            return SOLUTION_NETWORK_ALL;
    }

    bool NetEaseBiddingHandler::parseRequestData(const std::string & data)
    {
        return parseJson(data.c_str(), bidRequest);
    }

    bool NetEaseBiddingHandler::fillLogItem(protocol::log::LogItem & logItem)
    {
        logItem.logType = protocol::log::LogPhaseType::BID;
        logItem.reqStatus = 200;
        const cppcms::json::value & deviceInfo = bidRequest["device"];
        logItem.adInfo.adxid = ADX_NETEASE_MOBILE;
        logItem.adInfo.adxpid = adInfo.adxpid;
        if (isBidAccepted) {
            cppcms::json::value device;
            device["deviceinfo"] = deviceInfo;
            logItem.deviceInfo = toJson(device);
            logItem.adInfo.sid = adInfo.sid;
            logItem.adInfo.advId = adInfo.advId;
            logItem.adInfo.adxid = adInfo.adxid;
            logItem.adInfo.pid = adInfo.pid;
            logItem.adInfo.adxpid = adInfo.adxpid;
            logItem.adInfo.adxuid = adInfo.adxuid;
            logItem.adInfo.bannerId = adInfo.bannerId;
            logItem.adInfo.cid = adInfo.cid;
            logItem.adInfo.mid = adInfo.mid;
            logItem.adInfo.cpid = adInfo.cpid;
            logItem.adInfo.offerPrice = adInfo.offerPrice;
            logItem.adInfo.cost = 1500;
            logItem.adInfo.bidPrice = adInfo.offerPrice;
            logItem.adInfo.priceType = adInfo.priceType;
            logItem.adInfo.ppid = adInfo.ppid;
            url::extractAreaInfo(adInfo.areaId.data(), logItem.geoInfo.country, logItem.geoInfo.province,
                                 logItem.geoInfo.city);
            logItem.adInfo.bidSize = adInfo.bidSize;
            logItem.adInfo.orderId = adInfo.orderId;
            logItem.adInfo.ppids = adInfo.ppids;
        } else {
            logItem.adInfo.pid = adInfo.pid;
            logItem.adInfo.bidSize = adInfo.bidSize;
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
        AdSelectCondition queryCondition;
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
            std::string deviceId = queryCondition.mobileDevice == SOLUTION_DEVICE_ANDROID ? device.get("imei", "")
                                                                                          : device.get("idfa", "");
            strncpy(biddingFlowInfo.deviceIdBuf, deviceId.data(), sizeof(biddingFlowInfo.deviceIdBuf) - 1);
            const cppcms::json::value & networkStatus = device.find("network_status");
            if (!networkStatus.is_undefined()) {
                queryCondition.mobileNetwork = getNetwork(networkStatus.str());
            }
        }
        PreSetAdplaceInfo adplaceInfo;
        adplaceInfo.flowType = SOLUTION_FLOWTYPE_MOBILE;
        auto & sizeStyleMap = adplaceStyleMap.getSizeStyleMap();
        for (auto iter : sizeStyleMap) {
            adplaceInfo.sizeArray.push_back(std::make_pair(iter.first.first, iter.first.second));
            queryCondition.width = iter.first.first;
            queryCondition.height = iter.first.second;
        }
        queryCondition.pAdplaceInfo = &adplaceInfo;

        if (!filterCb(this, queryCondition)) {
            adInfo.pid = std::to_string(queryCondition.mttyPid);
            adInfo.adxpid = queryCondition.adxpid;
            adInfo.adxid = queryCondition.adxid;
            adInfo.bidSize = makeBidSize(queryCondition.width, queryCondition.height);
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

    static bool replace(std::string & str, const std::string & from, const std::string & to)
    {
        size_t start_pos = str.find(from);
        if (start_pos == std::string::npos)
            return false;
        str.replace(start_pos, from.length(), to);
        return true;
    }

    void NetEaseBiddingHandler::buildBidResult(const AdSelectCondition & queryCondition,
                                               const MT::common::SelectResult & result)
    {
        const MT::common::ADPlace & adplace = result.adplace;
        const MT::common::Banner & banner = result.banner;
        if (queryCondition.adxid != adplace.adxId || queryCondition.adxpid != adplace.adxPId) {
            LOG_ERROR << "in NetEaseBiddingHandler,query adxid:" << queryCondition.adxid
                      << ",result adxid:" << adplace.adxId << ",query adxpid:" << queryCondition.adxpid
                      << ",result adxpid:" << adplace.adxPId;
            isBidAccepted = false;
            return;
        }
        if (!parseJson(BIDRESPONSE_TEMPLATE, bidResponse)) {
            LOG_ERROR << "in NetEaseBiddingHandler::buildBidResult parseJson failed";
            isBidAccepted = false;
            return;
        }

        //缓存最终广告结果
        fillAdInfo(queryCondition, result, "");

        // html snippet相关
        std::string requestId = bidRequest["id"].str();
        char clickParam[2048];
        char showParam[2048];
        char buffer[2048];

        int bannerType = banner.bannerType;
        char pjson[2048] = { '\0' };
        std::string strBannerJson = banner.json;
        strncat(pjson, strBannerJson.data(), sizeof(pjson));
        // tripslash2(pjson);
        cppcms::json::value bannerJson;
        parseJson(pjson, bannerJson);
        std::string mainTitle;
        std::string landingUrl;
        std::string downloadUrl;
        getShowPara(requestId, showParam, sizeof(showParam));
        snprintf(buffer, sizeof(buffer), AD_NETEASE_SHOW_URL, showParam);
        bidResponse["showMonitorUrl"] = buffer;
        std::vector<std::string> materialUrls;
        cppcms::json::array & mtlsArray = bannerJson["mtls"].array();
        if (bannerType == BANNER_TYPE_PRIMITIVE) {
            materialUrls.push_back(mtlsArray[0]["p6"].str());
            materialUrls.push_back(mtlsArray[0]["p7"].str());
            materialUrls.push_back(mtlsArray[0]["p8"].str());
            landingUrl = mtlsArray[0]["p9"].str();
            replace(landingUrl, "{{click}}", "");
            if (queryCondition.mobileDevice == SOLUTION_DEVICE_IPHONE) {
                downloadUrl = mtlsArray[0]["p10"].str();
            } else if (queryCondition.mobileDevice == SOLUTION_DEVICE_ANDROID) {
                downloadUrl = mtlsArray[0]["p11"].str();
            }
            int style = 0;
            auto & sizeStyleMap = adplaceStyleMap.getSizeStyleMap();
            for (auto iter = sizeStyleMap.find(std::make_pair(banner.width, banner.height));
                 iter != sizeStyleMap.end() && (style = iter->second) && false;)
                ;

            if (style == 3) {
                mainTitle = mtlsArray[0]["p1"].str();
                std::string bakMainTitle = mtlsArray[0]["p0"].str();
                if (mainTitle.length() < bakMainTitle.length()) {
                    mainTitle = bakMainTitle;
                }
            } else {
                mainTitle = mtlsArray[0]["p0"].str();
            }
            bidResponse["style"] = std::to_string(style);
        } else {
            std::string materialUrl = mtlsArray[0]["p0"].str();
            materialUrls.push_back(materialUrl);
            landingUrl = mtlsArray[0]["p1"].str();
        }
        bidResponse["mainTitle"] = mainTitle;
        bidResponse["valid_time"] = 86400000;
        getClickPara(requestId, clickParam, sizeof(clickParam), "", landingUrl);
        snprintf(buffer, sizeof(buffer), AD_NETEASE_CLICK_URL, clickParam);
        bidResponse["linkUrl"] = buffer;
        cppcms::json::array & resArray = bidResponse["resource_url"].array();
        for (auto & murl : materialUrls) {
            if (!murl.empty())
                resArray.push_back(cppcms::json::value(murl));
        }
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
