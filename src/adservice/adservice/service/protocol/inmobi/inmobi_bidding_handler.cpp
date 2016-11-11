
#include <sstream>

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
    using namespace adservice::server;

#define INMOBI_DEV_MOBILE 1
#define INMOBI_DEV_PC 2
#define INMOBI_DEV_TV 3
#define INMOBI_DEV_PHONE 4
#define INMOBI_DEV_TABLET 5
#define INMOBI_DEV_CONNECTEDDEVICE 6
#define INMOBI_DEV_SETTOPBOX 7

    bool InmobiBiddingHandler::parseRequestData(const std::string & data)
    {
        return parseJson(data.c_str(), bidRequest);
    }

    bool InmobiBiddingHandler::fillLogItem(protocol::log::LogItem & logItem)
    {
        logItem.reqStatus = 200;
        cppcms::json::value & deviceInfo = bidRequest["device"];
        logItem.userAgent = deviceInfo["ua"].str();
        logItem.ipInfo.proxy = deviceInfo["ip"].str();
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
            logItem.adInfo.ppids = adInfo.ppids;
        } else {
            logItem.adInfo.pid = adInfo.pid;
            logItem.adInfo.bidSize = adInfo.bidSize;
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
        std::string pid = adzinfo.get<std::string>("id");
        AdSelectCondition queryCondition;
        queryCondition.adxid = ADX_INMOBI;
        queryCondition.adxpid = pid;
        double priceFloor = adzinfo.get<double>("bidfloor", 0.0);
        queryCondition.basePrice = (int)std::ceil(priceFloor);
        boolean isNative = false;
        cppcms::json::value adTypeObj = adzinfo.find("banner");
        if (adTypeObj.is_undefined()) {
            adTypeObj = adzinfo.find("video");
            if (adTypeObj.is_undefined()) {
                adTypeObj = adzinfo.find("native");
                isNative = true;
            }
        }
        if (isNative) {
            queryCondition.width = adTypeObj.get("w", 0);
            queryCondition.height = adTypeObj.get("h", 0);
        } else {
            cppcms::json::value nativeAssets = adTypeObj.find("assets");
            if (!nativeAssets.is_undefined()) {
                const cppcms::json::array & assetsArray = nativeAssets.array();
                if (assetsArray.size() > 0) {
                    queryCondition.width = assetsArray[0].get("img.w", 0);
                    queryCondition.height = assetsArray[0].get("img.h", 0);
                }
            }
        }
        cppcms::json::value & device = bidRequest.find("device");
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

    void InmobiBiddingHandler::buildBidResult(const AdSelectCondition & queryCondition,
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
        std::string materialUrl = mtlsArray[0]["p0"].str();
        std::string landingUrl = mtlsArray[0]["p1"].str();
        std::string tview = bannerJson["tview"].str();

        cppcms::json::value & extValue = bidValue["ext"];
        char showParam[2048];
        char clickParam[2048];
        getShowPara(requestId, showParam, sizeof(showParam));
        if (isDeal && finalSolution.dDealId != "0") { // deal 加特殊参数w
            char dealParam[256];
            int dealParamLen
                = snprintf(dealParam, sizeof(dealParam), "&" URL_YOUKU_DEAL "=%s", finalSolution.dDealId.data());
            strncat(showParam, dealParam, dealParamLen);
            bidValue["dealid"] = finalSolution.dDealId;
        }
        char buffer[2048];
        snprintf(buffer, sizeof(buffer), AD_YOUKU_FEED, AD_YOUKU_PRICE, showParam); //包含of=3
        bidValue["nurl"] = buffer;
        std::string crid = std::to_string(adInfo.bannerId);
        bidValue["crid"] = crid;
        if (!adzInfo.find("video").is_undefined()
            || queryCondition.flowType == SOLUTION_FLOWTYPE_MOBILE) { //视频流量 adm为素材地址 ldp为点击链
            bidValue["adm"] = materialUrl;
            getClickPara(requestId, clickParam, sizeof(clickParam), "", landingUrl);
            snprintf(buffer, sizeof(buffer), AD_YOUKU_CLICK, clickParam);
            std::string cm = buffer;
            extValue["ldp"] = cm;
            if (!tview.empty()) {
                cppcms::json::array & extPmArray = extValue["pm"].array();
                extPmArray.push_back(cppcms::json::value(tview));
            }
        } else { //动态创意流量 adm为iframe 设置type
            int w = banner.width;
            int h = banner.height;
            std::string html = generateHtmlSnippet(requestId, w, h, "of=2&", "");
            bidValue["adm"] = html;
            extValue["type"] = "c";
        }
        int maxCpmPrice = result.bidPrice;
        bidValue["price"] = maxCpmPrice;
    }

    void InmobiBiddingHandler::match(adservice::utility::HttpResponse & response)
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

    void InmobiBiddingHandler::reject(adservice::utility::HttpResponse & response)
    {
        response.status(204);
        response.set_content_header("application/json; charset=utf-8");
    }
}
}
