//
// Created by guoze.lin on 16/5/3.
//
#include <sstream>

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

    bool YoukuBiddingHandler::fillLogItem(protocol::log::LogItem & logItem)
    {
        logItem.reqStatus = 200;
        cppcms::json::value & deviceInfo = bidRequest["device"];
        logItem.userAgent = deviceInfo.get("ua", "");
        logItem.ipInfo.proxy = deviceInfo.get("ip", "");
        logItem.adInfo.adxid = adInfo.adxid;
        logItem.adInfo.adxpid = adInfo.adxpid;
        if (isBidAccepted) {
            cppcms::json::value device;
            device["deviceInfo"] = deviceInfo;
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
            logItem.adInfo.priceType = adInfo.priceType;
            logItem.adInfo.ppid = adInfo.ppid;
            url::extractAreaInfo(
                adInfo.areaId.data(), logItem.geoInfo.country, logItem.geoInfo.province, logItem.geoInfo.city);
            logItem.adInfo.bidSize = adInfo.bidSize;
            logItem.referer = bidRequest.get("site.ref", "");
            logItem.adInfo.orderId = adInfo.orderId;
        } else {
            logItem.adInfo.pid = adInfo.pid;
            logItem.adInfo.bidSize = adInfo.bidSize;
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

        const cppcms::json::value & adzinfo = impressions[0];
        std::string pid = adzinfo.get<std::string>("tagid");
        AdSelectCondition queryCondition;
        queryCondition.adxid = ADX_YOUKU;
        queryCondition.adxpid = pid;
        queryCondition.basePrice = adzinfo.get<int>("bidfloor", 0);
        adInfo.adxid = ADX_YOUKU;
        PreSetAdplaceInfo adplaceInfo;
        bool isNative = false;
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
            queryCondition.bannerType = BANNER_TYPE_PRIMITIVE;
            const cppcms::json::array & assets = adObject.find("assets").array();
            if (assets.size() > 0) {
                for (uint32_t i = 0; i < assets.size(); i++) {
                    const cppcms::json::value & asset = assets[i];
                    queryCondition.width = asset.get("image_url.w", 750);
                    queryCondition.height = asset.get("image_url.h", 350);
                    adplaceInfo.sizeArray.push_back({ queryCondition.width, queryCondition.height });
                }
                queryCondition.pAdplaceInfo = &adplaceInfo;
            } else {
                return false;
            }
        }
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
            if (queryCondition.mobileDevice == SOLUTION_DEVICE_ANDROID
                || queryCondition.mobileDevice == SOLUTION_DEVICE_ANDROIDPAD) {
                std::string deviceId = device.get<std::string>("androidid", "");
                strncpy(biddingFlowInfo.deviceIdBuf, deviceId.data(), sizeof(biddingFlowInfo.deviceIdBuf) - 1);
            } else if (queryCondition.mobileDevice == SOLUTION_DEVICE_IPAD
                       || queryCondition.mobileDevice == SOLUTION_DEVICE_IPHONE) {
                std::string deviceId = device.get<std::string>("idfa", "");
                strncpy(biddingFlowInfo.deviceIdBuf, deviceId.data(), sizeof(biddingFlowInfo.deviceIdBuf) - 1);
            } else {
                biddingFlowInfo.deviceIdBuf[0] = '\0';
            }
            if (queryCondition.flowType == SOLUTION_FLOWTYPE_MOBILE) {
                queryCondition.adxid = ADX_YOUKU_MOBILE;
                adInfo.adxid = ADX_YOUKU_MOBILE;
            }

            queryCondition.mobileNetwork = getNetwork(device.get("connectiontype", 0));
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
	"id":"",
	"bidid":"",
	"seatbid":[
		{
			"bid":[
				{
					"adm":"",
					"id":"",
					"impid":"",
					"nurl":"",
					"price":"",
					"crid":"",
					"ext":{
						"ldp":"",
						"pm":[],
						"cm":[],
						"type":""
					}
				}
			]
		}
	]
})";

    void YoukuBiddingHandler::buildBidResult(const AdSelectCondition & queryCondition,
                                             const MT::common::SelectResult & result)
    {
        const cppcms::json::value & adzInfo = bidRequest["imp"].array()[0];
        const MT::common::Solution & finalSolution = result.solution;
        const MT::common::Banner & banner = result.banner;
        if (!parseJson(BIDRESPONSE_TEMPLATE, bidResponse)) {
            LOG_ERROR << "in YoukuBiddingHandler::buildBidResult parseJson failed";
            isBidAccepted = false;
            return;
        }
        std::string requestId = bidRequest["id"].str();
        bidResponse["id"] = requestId;
        bidResponse["bidid"] = "1";
        cppcms::json::value & bidValue = bidResponse["seatbid"][0]["bid"][0];
        bidValue["id"] = "1";
        std::string impId = adzInfo["id"].str();
        bidValue["impid"] = impId;

        //缓存最终广告结果
        fillAdInfo(queryCondition, result, bidRequest.get("user.id", ""));

        // html snippet相关
        char pjson[2048] = { '\0' };
        std::string strBannerJson = banner.json;
        strncat(pjson, strBannerJson.data(), sizeof(pjson));
        // tripslash2(pjson);
        cppcms::json::value bannerJson;
        parseJson(pjson, bannerJson);
        cppcms::json::array & mtlsArray = bannerJson["mtls"].array();
        std::string tview = bannerJson["tview"].str();

        cppcms::json::value & extValue = bidValue["ext"];
        char showParam[2048];
        char clickParam[2048];
        getShowPara(requestId, showParam, sizeof(showParam));
        if (isDeal && finalSolution.dDealId != "0") { // deal 加特殊参数w
            char dealParam[256];
            int dealParamLen
                = snprintf(dealParam, sizeof(dealParam), "&" URL_DEAL_ID "=%s", finalSolution.dDealId.data());
            strncat(showParam, dealParam, dealParamLen);
            bidValue["dealid"] = finalSolution.dDealId;
        }
        char buffer[2048];
        snprintf(buffer, sizeof(buffer), AD_YOUKU_FEED, AD_YOUKU_PRICE, showParam); //包含of=3
        bidValue["nurl"] = buffer;
        std::string crid = std::to_string(adInfo.bannerId);
        bidValue["crid"] = crid;
        std::string landingUrl;
        if (banner.bannerType == BANNER_TYPE_PRIMITIVE) {
            int nativeTemplateId = 0;
            const cppcms::json::array & assets = adzInfo.find("native.assets").array();
            std::string title = mtlsArray[0]["p0"].str();
            for (uint32_t i = 0; i < assets.size(); i++) {
                const cppcms::json::value & asset = assets[i];
                int w = asset.get("image_url.w", 0);
                int h = asset.get("image_url.h", 0);
                uint32_t titleLen = asset.get("title.len", 0);
                if (w == banner.width && h == banner.height && title.length() <= titleLen) {
                    nativeTemplateId = asset.get("native_template_id", 0);
                    break;
                }
            }
            bidValue["native.native_template_id"] = nativeTemplateId;
            landingUrl = mtlsArray[0]["p9"].str();
            replace(landingUrl, "{{click}}", "");
        } else {
            std::string materialUrl = mtlsArray[0]["p0"].str();
            bidValue["adm"] = materialUrl;
            landingUrl = mtlsArray[0]["p1"].str();
        }
        getClickPara(requestId, clickParam, sizeof(clickParam), "", landingUrl);
        snprintf(buffer, sizeof(buffer), AD_YOUKU_CLICK, clickParam);
        std::string cm = buffer;
        extValue["ldp"] = cm;
        if (!tview.empty()) {
            cppcms::json::array & extPmArray = extValue["pm"].array();
            extPmArray.push_back(cppcms::json::value(tview));
        }
        int maxCpmPrice = result.bidPrice;
        bidValue["price"] = maxCpmPrice;
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
