#include <sstream>

#include "core/core_ad_sizemap.h"
#include "core/core_ip_manager.h"
#include "core/core_typetable.h"
#include "logging.h"
#include "netease_bidding_handlerv2.h"
#include "nex_bidding_handler.h"
#include "utility/utility.h"

namespace protocol {
namespace bidding {

    using namespace adservice::utility;
    using namespace adservice::utility::serialize;
    using namespace adservice::utility::json;
    using namespace adservice::utility::userclient;
    using namespace adservice::utility::cypher;
    using namespace adservice::server;

    static NetEaseAdplaceStyleMap adplaceStyleMap;

#define AD_NEX_CM_URL ""
#define AD_NEX_FEED "http://show.mtty.com/v?of=3&p=%s&%s"
#define AD_NEX_CLICK "http://click.mtty.com/c?%s"
#define AD_NEX_PRICE "${AUCTION_PRICE}"
#define NEX_OS_WINDOWS "Windows"
#define NEX_OS_ANDROID "Android"
#define NEX_OS_iPhone "ios"
#define NEX_OS_MAC "Mac"
#define NEX_CONN_WIFI "wifi"
#define NEX_CONN_2G "2g"
#define NEX_CONN_3G "3g"
#define NEX_CONN_4G "4g"

    static void fromNexDevTypeOsType(const std::string & osType,
                                     const std::string & ua,
                                     int & flowType,
                                     int & mobileDev,
                                     int & pcOs,
                                     std::string & pcBrowser)
    {
        flowType = SOLUTION_FLOWTYPE_MOBILE;
        if (!strcasecmp(osType.data(), NEX_OS_ANDROID)) {
            mobileDev = SOLUTION_DEVICE_ANDROID;
        } else if (!strcasecmp(osType.data(), NEX_OS_iPhone)) {
            mobileDev = SOLUTION_DEVICE_IPHONE;
        } else {
            flowType = SOLUTION_FLOWTYPE_PC;
            if (osType.empty()) {
                pcOs = getOSTypeFromUA(ua);
            } else {
                if (!strcasecmp(osType.data(), NEX_OS_WINDOWS)) {
                    pcOs = SOLUTION_OS_WINDOWS;
                } else if (!strcasecmp(osType.data(), NEX_OS_MAC)) {
                    pcOs = SOLUTION_OS_MAC;
                } else
                    pcOs = SOLUTION_OS_OTHER;
            }
            pcBrowser = getBrowserTypeFromUA(ua);
        }
    }

    static int getNetwork(const std::string & network)
    {
        if (!strcasecmp(network.c_str(), NEX_CONN_WIFI)) {
            return SOLUTION_NETWORK_WIFI;
        } else if (!strcasecmp(network.c_str(), NEX_CONN_2G)) {
            return SOLUTION_NETWORK_2G;
        } else if (!strcasecmp(network.c_str(), NEX_CONN_3G)) {
            return SOLUTION_NETWORK_3G;
        } else if (!strcasecmp(network.c_str(), NEX_CONN_4G)) {
            return SOLUTION_NETWORK_4G;
        } else {
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

    bool NexBiddingHandler::parseRequestData(const std::string & data)
    {
        bidResponse.undefined();
        return parseJson(data.c_str(), bidRequest);
    }

    bool NexBiddingHandler::fillSpecificLog(const AdSelectCondition & selectCondition, protocol::log::LogItem & logItem,
                                            bool isAccepted)
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

    bool NexBiddingHandler::filter(const BiddingFilterCallback & filterCb)
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
            queryCondition.adxid = ADX_NEX_PC;
            queryCondition.adxpid = pid;
            queryCondition.basePrice = adzinfo.get<int>("bidfloor", 0);
            adInfo.adxid = ADX_NEX_PC;
            PreSetAdplaceInfo & adplaceInfo = adplaceInfos[i];
            const cppcms::json::array & allowstyles = adzinfo.find("allowstyle").array();
            if (allowstyles.empty()) {
                continue;
            }
            for (auto & allowStyle : allowstyles) {
                int64_t style = allowStyle.number();
                if (adplaceStyleMap.find(style)) {
                    const auto & sizeStyle = adplaceStyleMap.get(style);
                    for (auto size : sizeStyle.sizes) {
                        adplaceInfo.sizeArray.push_back(std::make_pair(size.first, size.second));
                        queryCondition.width = size.first;
                        queryCondition.height = size.second;
                    }
                }
            }
            queryCondition.pAdplaceInfo = &adplaceInfo;
            if (i == 0) {
                cppcms::json::value & device = bidRequest["device"];
                if (!device.is_undefined()) {
                    std::string ip = device.get("ip", "");
                    queryCondition.ip = ip;
                    std::string osType = device.get("os", "");
                    std::string ua = device.get("ua", "");
                    fromNexDevTypeOsType(osType,
                                         ua,
                                         queryCondition.flowType,
                                         queryCondition.mobileDevice,
                                         queryCondition.pcOS,
                                         queryCondition.pcBrowserStr);
                    if (queryCondition.flowType == SOLUTION_FLOWTYPE_MOBILE) {
                        queryCondition.adxid = ADX_NEX_MOBILE;
                        adInfo.adxid = ADX_NEX_MOBILE;
                    }
                    queryCondition.mobileNetwork = getNetwork(device.get("connectiontype", ""));
                    queryCondition.idfa = device.get("idfa", "");
                    queryCondition.mac = device.get("mac", "");
                    queryCondition.imei = device.get("imei", "");
                    cookieMappingKeyMobile(md5_encode(queryCondition.idfa),
                                           md5_encode(queryCondition.imei),
                                           md5_encode(queryCondition.androidId),
                                           md5_encode(queryCondition.mac));
                    queryCookieMapping(cmInfo.queryKV, queryCondition);
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
                        int64_t bidfloor = deals[i].get("bidfloor", 0);
                        if (bidfloor > queryCondition.basePrice) {
                            queryCondition.basePrice = bidfloor;
                        }
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

    void NexBiddingHandler::buildBidResult(const AdSelectCondition & queryCondition,
                                           const MT::common::SelectResult & result, int seq)
    {
        std::string requestId = bidRequest["id"].str();
        if (bidResponse.is_undefined()) {
            if (!parseJson(BIDRESPONSE_TEMPLATE, bidResponse)) {
                LOG_ERROR << "in NexBiddingHandler::buildBidResult parseJson failed";
                isBidAccepted = false;
                return;
            }
            bidResponse["id"] = requestId;
            bidResponse["bidid"] = "1";
            bidResponse["cur"] = "CNY";
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
        showUrl.addMacro(URL_EXCHANGE_PRICE, AD_NEX_PRICE);
        bidValue["nurl"] = "http://mtty-cdn.mtty.com/1x1.gif";
        bidValue["pvm"] = cppcms::json::array();
        bidValue["pvm"].array().push_back(std::string(isIOS ? SNIPPET_SHOW_URL_HTTPS : SNIPPET_SHOW_URL) + "?"
                                          + showUrl.cipherParam());
        cppcms::json::value bannerFeedbackJson;
        if (!banner.feedback.empty()) {
            parseJson(banner.feedback.c_str(), bannerFeedbackJson);
        }
        std::string crid = bannerFeedbackJson.get(std::to_string(queryCondition.adxid) + ".adx_cid",
                                                  bannerFeedbackJson.get("adx_cid", std::to_string(adInfo.bannerId)));
        bidValue["crid"] = crid;
        std::string landingUrl;
        std::string mainTitle;
        if (banner.bannerType == BANNER_TYPE_PRIMITIVE) {
            landingUrl = mtlsArray[0]["p9"].str();
            replace(landingUrl, "{{click}}", "");
            adservice::utility::url::url_replace(landingUrl, "https://", "http://");
            extValue["linkUrl"] = landingUrl;
            int style = 0;
            auto & sizeStyleMap = adplaceStyleMap.getSizeStyleMap();
            for (auto iter = sizeStyleMap.find(std::make_pair(banner.width, banner.height));
                 iter != sizeStyleMap.end() && (style = iter->second) && false;)
                ;

            std::vector<std::string> materialUrls;
            materialUrls.push_back(mtlsArray[0]["p6"].str());
            if (style == 11) {
                materialUrls.push_back(mtlsArray[0]["p7"].str());
                materialUrls.push_back(mtlsArray[0]["p8"].str());
            }
            cppcms::json::array admArray;
            for (auto & murl : materialUrls) {
                cppcms::json::value adm;
                adm["url"] = murl;
                adm["type"] = 0;
                admArray.push_back(adm);
            }
            extValue["adm"] = admArray;
            if (style == 3) {
                mainTitle = mtlsArray[0]["p1"].str();
                std::string bakMainTitle = mtlsArray[0]["p0"].str();
                if (mainTitle.length() < bakMainTitle.length()) {
                    mainTitle = bakMainTitle;
                }
            } else {
                mainTitle = mtlsArray[0]["p0"].str();
            }
            extValue["title"] = mainTitle;
            extValue["style"] = style;
        }
        url::URLHelper clickUrlParam;
        getClickPara(clickUrlParam, requestId, "", landingUrl);
        std::string cm
            = std::string(isIOS ? SNIPPET_CLICK_URL_HTTPS : SNIPPET_CLICK_URL) + "?" + clickUrlParam.cipherParam();
        cppcms::json::array clickm = cppcms::json::array();
        clickm.push_back(cm);
        bidValue["clickm"] = clickm;
        if (!tview.empty()) {
            cppcms::json::array & extPmArray = bidValue["pvm"].array();
            extPmArray.push_back(cppcms::json::value(tview));
        } else {
            cppcms::json::array & extPmArray = bidValue["pvm"].array();
            extPmArray.push_back("https://mtty-cdn.mtty.com/1x1.gif");
        }
        int maxCpmPrice = result.bidPrice;
        bidValue["price"] = maxCpmPrice;
        bidValue["ext"] = extValue;
        bidArrays.push_back(std::move(bidValue));
        redoCookieMapping(queryCondition.adxid, "");
    }

    void NexBiddingHandler::match(adservice::utility::HttpResponse & response)
    {
        std::string result = toJson(bidResponse);
        if (result.empty()) {
            LOG_ERROR << "NexBiddingHandler::match failed to parse obj to json";
            reject(response);
        } else {
            response.status(200);
            response.set_content_header("application/json; charset=utf-8");
            response.set_body(result);
        }
    }

    void NexBiddingHandler::reject(adservice::utility::HttpResponse & response)
    {
        response.status(204);
        response.set_content_header("application/json; charset=utf-8");
    }
}
}
