//
// Created by guoze.lin on 16/5/3.
//
#include <sstream>

#include "adx2345_bidding_handler.h"
#include "core/core_ad_sizemap.h"
#include "core/core_ip_manager.h"
#include "core/core_typetable.h"
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

#define AD_2345_PRICE "${WINPRICE}"

    static void osFlowType(const std::string & osType, const std::string & ua, int & flowType, int & devType,
                           int & pcOs, std::string & pcBrowser)
    {
        flowType = SOLUTION_FLOWTYPE_PC;
        if (strcasecmp(osType.c_str(), "windows") == 0) {
            pcOs = SOLUTION_OS_WINDOWS;
        } else if (strcasecmp(osType.c_str(), "mac") == 0) {
            pcOs = SOLUTION_OS_MAC;
        } else if (strcasecmp(osType.c_str(), "android") == 0) {
            pcOs = SOLUTION_OS_ANDROID;
            flowType = SOLUTION_FLOWTYPE_MOBILE;
            devType = SOLUTION_DEVICE_ANDROID;
        } else if (strcasecmp(osType.c_str(), "ios") == 0) {
            pcOs = SOLUTION_OS_IOS;
            flowType = SOLUTION_FLOWTYPE_MOBILE;
            devType = SOLUTION_DEVICE_IPHONE;
        }
        if (pcOs == SOLUTION_OS_ALL || pcOs == SOLUTION_OS_OTHER) {
            pcOs = getOSTypeFromUA(ua);
            if (pcOs == SOLUTION_OS_ANDROID) {
                flowType = SOLUTION_FLOWTYPE_MOBILE;
                devType = SOLUTION_DEVICE_ANDROID;
            }
            if (pcOs == SOLUTION_OS_IOS) {
                flowType = SOLUTION_FLOWTYPE_MOBILE;
                devType = SOLUTION_DEVICE_IPHONE;
            }
        }
        pcBrowser = getBrowserTypeFromUA(ua);
    }

    bool Adx2345BiddingHandler::parseRequestData(const std::string & data)
    {
        bidResponse.undefined();
        return parseJson(data.c_str(), bidRequest);
    }

    bool Adx2345BiddingHandler::fillSpecificLog(const AdSelectCondition & selectCondition,
                                                protocol::log::LogItem & logItem, bool isAccepted)
    {
        cppcms::json::value & userInfo = bidRequest["userinfo"];
        logItem.userAgent = userInfo.get("ua", "");
        logItem.ipInfo.proxy = selectCondition.ip;
        if (isAccepted) {
            cppcms::json::value device;
            device["deviceInfo"] = userInfo;
            logItem.deviceInfo = toJson(device);
        }
        return true;
    }

    bool Adx2345BiddingHandler::filter(const BiddingFilterCallback & filterCb)
    {
        const cppcms::json::value & imp = bidRequest.find("adinfo");
        if (imp.is_undefined()) {
            return false;
        }
        const cppcms::json::array & impressions = imp.array();
        if (impressions.empty()) {
            return false;
        }
        std::vector<AdSelectCondition> queryConditions(impressions.size());
        std::vector<PreSetAdplaceInfo> adplaceInfos(impressions.size());
        const AdSizeMap & adSizeMap = AdSizeMap::getInstance();
        for (uint32_t i = 0; i < impressions.size(); i++) {
            const cppcms::json::value & adzinfo = impressions[i];
            std::string pid = std::to_string(adzinfo.get<int>("adsenseid"));

            AdSelectCondition & queryCondition = queryConditions[i];
            queryCondition.adxid = ADX_2345;
            queryCondition.adxpid = pid;
            queryCondition.basePrice = adzinfo.get("bidfloor", 0);
            adInfo.adxid = ADX_2345;
            queryCondition.width = adzinfo.get("width", 0);
            queryCondition.height = adzinfo.get("height", 0);
            PreSetAdplaceInfo & adplaceInfo = adplaceInfos[i];
            auto sizePair = adSizeMap.get({ queryCondition.width, queryCondition.height });
            for (auto & sizeIter : sizePair) {
                queryCondition.width = sizeIter.first;
                queryCondition.height = sizeIter.second;
                adplaceInfo.sizeArray.push_back({ queryCondition.width, queryCondition.height });
            }
            queryCondition.pAdplaceInfo = &adplaceInfo;
            if (i == 0) {
                const cppcms::json::value & userInfo = bidRequest.find("userinfo");
                if (!userInfo.is_undefined()) {
                    std::string ip = userInfo.get("ip", "");
                    queryCondition.ip = ip;
                    std::string osType = userInfo.get("os", "");
                    std::string ua = userInfo.get("ua", "");
                    osFlowType(osType, ua, queryCondition.flowType, queryCondition.mobileDevice, queryCondition.pcOS,
                               queryCondition.pcBrowserStr);
                    queryCondition.deviceBrand = adservice::utility::userclient::getDeviceBrandFromUA(ua);
                }
            } else {
                AdSelectCondition & firstQueryCondition = queryConditions[0];
                queryCondition.ip = firstQueryCondition.ip;
                queryCondition.flowType = firstQueryCondition.flowType;
                queryCondition.mobileDevice = firstQueryCondition.mobileDevice;
                queryCondition.pcOS = firstQueryCondition.pcOS;
                queryCondition.pcBrowserStr = firstQueryCondition.pcBrowserStr;
                queryCondition.adxid = firstQueryCondition.adxid;
                queryCondition.deviceBrand = firstQueryCondition.deviceBrand;
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
    "bidlist":[
    ]
})";

    void Adx2345BiddingHandler::buildBidResult(const AdSelectCondition & queryCondition,
                                               const MT::common::SelectResult & result, int seq)
    {
        std::string requestId = bidRequest["id"].str();
        if (bidResponse.is_undefined()) {
            if (!parseJson(BIDRESPONSE_TEMPLATE, bidResponse)) {
                LOG_ERROR << "in Adx2345BiddingHandler::buildBidResult parseJson failed";
                isBidAccepted = false;
                return;
            }
            bidResponse["id"] = requestId;
            bidResponse["bid"] = 1;
            bidResponse["bidid"] = "2345" + requestId;
        }
        const cppcms::json::value & adzInfo = bidRequest["adinfo"].array()[seq];
        // const MT::common::Solution & finalSolution = result.solution;
        const MT::common::Banner & banner = result.banner;
        cppcms::json::array & bidArrays = bidResponse["bidlist"].array();
        cppcms::json::value bidValue;
        bidValue["id"] = "1";
        std::string impId = std::to_string(adzInfo.get("adsenseid", 0));
        bidValue["adsenseid"] = impId;

        //缓存最终广告结果
        fillAdInfo(queryCondition, result, "");

        bool isIOS = queryCondition.mobileDevice == SOLUTION_DEVICE_IPHONE
                     || queryCondition.mobileDevice == SOLUTION_DEVICE_IPAD;

        std::string strBannerJson = banner.json;
        cppcms::json::value bannerJson = bannerJson2HttpsIOS(isIOS, strBannerJson, banner.bannerType);
        cppcms::json::array & mtlsArray = bannerJson["mtls"].array();
        std::string tview = bannerJson.get("tview", "");
        std::string crid = std::to_string(adInfo.bannerId);
        bidValue["adid"] = crid;
        cppcms::json::array pvArray;
        url::URLHelper showUrl;
        getShowPara(showUrl, requestId);
        showUrl.add(URL_IMP_OF, "3");
        // showUrl.add(URL_EXCHANGE_PRICE, std::to_string(finalSolution.offerPrice));
        showUrl.addMacro(URL_EXCHANGE_PRICE, AD_2345_PRICE);
        pvArray.push_back(getShowBaseUrl(isIOS) + "?" + showUrl.cipherParam());
        std::string landingUrl = mtlsArray[0].get("p1", "");
        url::URLHelper clickUrlParam;
        getClickPara(clickUrlParam, requestId, "", landingUrl);
        clickUrlParam.addMacro(URL_EXCHANGE_PRICE, AD_2345_PRICE);
        cppcms::json::array clickArray;
        clickArray.push_back(getClickBaseUrl(isIOS) + "?" + clickUrlParam.cipherParam());
        bidValue["click"] = clickArray;
        if (!tview.empty()) {
            pvArray.push_back(tview);
        }
        bidValue["pv"] = pvArray;
        int maxCpmPrice = result.bidPrice;
        bidValue["price"] = maxCpmPrice;
        bidArrays.push_back(std::move(bidValue));
    }

    void Adx2345BiddingHandler::match(adservice::utility::HttpResponse & response)
    {
        std::string result = toJson(bidResponse);
        if (result.empty()) {
            LOG_ERROR << "Adx2345BiddingHandler::match failed to parse obj to json";
            reject(response);
        } else {
            response.status(200);
            response.set_content_header("application/json; charset=utf-8");
            response.set_body(result);
        }
    }

    void Adx2345BiddingHandler::reject(adservice::utility::HttpResponse & response)
    {
        response.status(204);
        response.set_content_header("application/json; charset=utf-8");
    }
}
}
