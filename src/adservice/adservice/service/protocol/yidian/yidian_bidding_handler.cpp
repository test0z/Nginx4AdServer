#include "yidian_bidding_handler.h"
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

    std::vector<std::pair<int, int>> YidianSizeMap{ { 1, 0 }, { 2, 12 }, { 3, 0 } };
    std::unordered_map<int, std::pair<int, int>> splashMap{ { 56, { 640, 1136 } }, { 66, { 640, 960 } } };

    void GetDevice(int & os, std::string & os_type)
    {
        boost::algorithm::to_upper(os_type);
        if (os_type == "IOS") {
            os = SOLUTION_DEVICE_IPHONE;
        } else if (os_type == "ANDROID") {
            os = SOLUTION_DEVICE_ANDROID;
        } else {
            os = SOLUTION_DEVICE_OTHER;
        }
    }
    void GetAdFlag(AdSelectCondition & select, const std::string & idfa, std::string & imei)
    {
        if (select.mobileDevice == SOLUTION_DEVICE_ANDROID) {
            select.imei = stringtool::toupper(imei);
            select.idfa = "";
        } else if (select.mobileDevice == SOLUTION_DEVICE_IPHONE) {
            select.idfa = stringtool::toupper(idfa);
            select.imei = "";
        } else {
            select.idfa = "";
            select.imei = "";
        }
        select.mac = "";
        select.androidId = "";
    }
    void getPriceType(OUT int & pricetype, const int & mtty_type)
    {
        switch (mtty_type) {
        case PRICETYPE_CPM:
        case PRICETYPE_RRTB_CPM:
            pricetype = 1;
            return;
        case PRICETYPE_CPC:
        case PRICETYPE_RRTB_CPC:
        case PRICETYPE_RCPC:
            pricetype = 2;
            return;
        default:
            pricetype = 5;
            return;
        }
    }
    bool YidianBidingHandler::parseRequestData(const std::string & data)
    {
        bidRequest.undefined();
        bidResponse.undefined();
        return parseJson(data.c_str(), bidRequest);
    }
    bool YidianBidingHandler::fillSpecificLog(const AdSelectCondition & selectCondition,
                                              protocol::log::LogItem & logItem, bool isAccepted)
    {

        logItem.ipInfo.proxy = selectCondition.ip;
        if (isAccepted) {
            const cppcms::json::value & deviceInfo = bidRequest["device"];
            cppcms::json::value device;
            device["deviceinfo"] = deviceInfo;
            logItem.deviceInfo = toJson(device);
        }
        return true;
    }
    bool YidianBidingHandler::filter(const BiddingFilterCallback & filterCb)
    {
        std::vector<PreSetAdplaceInfo> adplaceInfos{ PreSetAdplaceInfo() };
        std::vector<AdSelectCondition> queryConditions{ AdSelectCondition() };
        AdSelectCondition & queryCondition = queryConditions[0];
        PreSetAdplaceInfo & adplaceInfo = adplaceInfos[0];
        queryCondition.isFromSSP = true;
        queryCondition.pAdplaceInfo = &adplaceInfo;
        queryCondition.adxid = ADX_YIDIAN;
        queryCondition.basePrice = 1;
        queryCondition.flowType = SOLUTION_FLOWTYPE_MOBILE;
        adplaceInfo.flowType = SOLUTION_FLOWTYPE_MOBILE;
        std::string dealId = bidRequest.get("deal_id", "");
        if (!dealId.empty()) {
            std::ostringstream deal_os;
            deal_os << ',' << dealId << ',';
            queryCondition.dealId = deal_os.str();
        }
        const cppcms::json::value & device_object = bidRequest.find("device");
        /*获取设备信息*/
        queryCondition.ip = device_object.get("ip", "");
        std::string os_type = device_object.get("os", "");
        std::string idfa = device_object.get("ifa", "");
        std::string imei = device_object.get("imeiplain", "");
        GetDevice(queryCondition.mobileDevice, os_type);
        GetAdFlag(queryCondition, idfa, imei);
        std::string pid = device_object.get("bundle", "");
        queryCondition.adxpid = pid;
        cookieMappingKeyMobile(md5_encode(queryCondition.idfa), md5_encode(queryCondition.imei),
                               md5_encode(queryCondition.androidId), md5_encode(queryCondition.mac), queryCondition);
        int ad_type = boost::lexical_cast<int>(bidRequest.get("tagid", ""));
        int width = 0;
        int height = 0;
        // const AdSizeMap & adSizeMap = AdSizeMap::getInstance();
        switch (ad_type) {
        case 0: {
            const cppcms::json::value & Osplash = bidRequest.find("splash");
            width = Osplash.get("w", 0);
            height = Osplash.get("h", 0);
            int tempdata = ((float)width / height) * 100;
            LOG_DEBUG << tempdata;
            auto it = splashMap.find(tempdata);
            if (it != splashMap.end()) {
                queryCondition.width = it->second.first;
                queryCondition.height = it->second.second;
                LOG_DEBUG << queryCondition.width << queryCondition.height;
            } else {
                queryCondition.width = 640;
                queryCondition.height = 1136;
            }
            queryCondition.adxpid = "10000";
            adplaceInfo.sizeArray.push_back({ queryCondition.width, queryCondition.height });
            Adtype = 0;
        } break;
        case 1: {
            const cppcms::json::value & Olist = bidRequest.find("list");
            // width = 360;
            // height = 234;
            maxduration = Olist.get("maxduration", 0);
            queryCondition.adxpid = "10001";
            Adtype = 1;
        } break;
        case 2: {
            // const cppcms::json::value & Ocontent = bidRequest.find("content");
            // width = 900;
            // height = 450;
            queryCondition.adxpid = "10002";
            Adtype = 2;
        } break;

        default:
            return bidFailedReturn();
        }
        if (Adtype) {
            queryCondition.bannerType = BANNER_TYPE_PRIMITIVE;
            for (auto sizeIter : YidianSizeMap) {
                queryCondition.width = sizeIter.first;
                queryCondition.height = sizeIter.second;
                adplaceInfo.sizeArray.push_back({ queryCondition.width, queryCondition.height });
            }
        }
        queryCookieMapping(cmInfo.queryKV, queryCondition);
        if (!filterCb(this, queryConditions)) {
            return bidFailedReturn();
        }
        return isBidAccepted = true;
    }

    static const char * BIDRESPONSE_TEMPLATE = R"(
{
	"id":"",
	"tagid":"",
	"bid_type":0,
     "price":"",
     "clickmonitor_urls":[],
      "viewmonitor_urls":[]
})";
    static const char * SPLASH_TEMPLATE = R"(
{
    "id":0,
    "image_url":"",
    "click_type":0,
    "click_url":"",
    "full_screen":0
})";
    static const char * LIST_TEMPLATE = R"(
{
 "id":0,
  "title":"",
  "source":"",
   "image_url":[],
   "click_url":"", 
})";

    static const char * CONTENT_TEMPLATE = R"({
  "id":0,
  "title":"",
  "source":"",
   "image_url":[],
   "click_url":""
})";
    void YidianBidingHandler::buildBidResult(IN const AdSelectCondition & queryCondition,
                                             IN const MT::common::SelectResult & result, int seq)
    {
        std::string requestId = bidRequest["id"].str();
        std::string tagId = bidRequest["tagid"].str();
        if (bidResponse.is_undefined()) {
            if (!parseJson(BIDRESPONSE_TEMPLATE, bidResponse)) {
                LOG_ERROR << "in YidianBiddingHandler::buildBidResult parseJson failed";
                isBidAccepted = false;
                return;
            }
        }
        bidResponse["id"] = requestId;
        bidResponse["tagid"] = tagId;
        int priceType = 0;
        const MT::common::Solution & finalSolution = result.solution;
        const MT::common::Banner & banner = result.banner;
        bool isIOS = queryCondition.mobileDevice == SOLUTION_DEVICE_IPHONE
                     || queryCondition.mobileDevice == SOLUTION_DEVICE_IPAD;
        fillAdInfo(queryCondition, result, bidRequest.get<std::string>("id"));
        getPriceType(priceType, finalSolution.priceType);
        bidResponse["bid_type"] = priceType;
        std::ostringstream os;
        os << result.bidPrice;
        bidResponse["price"] = os.str();
        std::string strBannerJson = banner.json;
        cppcms::json::value bannerJson = bannerJson2HttpsIOS(isIOS, strBannerJson, banner.bannerType);
        std::string landingurl = "";
        std::string clickurl = "";
        std::string monitor_url = "";
        const cppcms::json::array & mtlsArray = bannerJson["mtls"].array();
        cppcms::json::value adObject;
        switch (Adtype) {
        case 0:
            json::parseJson(SPLASH_TEMPLATE, adObject);
            break;
        case 1:
            json::parseJson(LIST_TEMPLATE, adObject);
            break;
        case 2:
            json::parseJson(CONTENT_TEMPLATE, adObject);
            break;
        default:
            isBidAccepted = false;
            return;
        }
        /*原生处理*/
        if (banner.bannerType == BANNER_TYPE_PRIMITIVE) {
            cppcms::json::array & imgUrls = adObject["image_url"].array();
            landingurl = mtlsArray[0].get("p9", "");
            // std::string download_url = ;
            int img_total = 0;
            int adtypeId = 0;
            int type = boost::lexical_cast<int>(mtlsArray[0].get("p2", ""));
            /*if (type == 3) { //组图
                img_total = 3;
                adtypeId = 2;
            } else if (type < 3) { //大图和小图
                img_total = 1;
                if (type == 2)
                    adtypeId = 0; //大图
                else
                    adtypeId = 1; //小图
            } else if (type == 4 && Adtype == 1) {
                img_total = 1;
                adtypeId = 7;
            }*/
            // adtypeId response中标注广告类型
            switch (type) {
            case 1: //小图
                img_total = 1;
                adtypeId = 1;
                break;
            case 2: //大图
                img_total = 1;
                adtypeId = 0;
                break;
            case 3: //组图
                img_total = 3;
                adtypeId = 2;
                break;
            case 4: //视频流
                img_total = 1;
                adtypeId = 7;
                break;
            case 5:
                img_total = 0;
                break;
            default:
                isBidAccepted = false;
                return;
            }
            adObject["id"] = adtypeId;
            adObject["title"] = mtlsArray[0].get("p0", "");
            adObject["source"] = mtlsArray[0].get("p18", "");
            /*if(adtypeId == 7)
            {
                adObject["video_duration"] = mtlsArray[0].get("","");
                adObject["video_url"] = mtlsArray[0].get("p16","");
            }*/
            for (int i = 0; i < img_total; i++) {
                std::ostringstream out_str;
                out_str << 'p' << 6 + i;
                std::string img_url = mtlsArray[0].get(out_str.str(), "");
                /*if (isIOS)
                    adservice::utility::url::url_replace(img_url, "http://", "https://");
                else
                    adservice::utility::url::url_replace(img_url, "https://", "http://");*/
                imgUrls.push_back(cppcms::json::value(img_url));
            }

        } else {
            landingurl = mtlsArray[0].get("p1", "");
            std::string img_url = mtlsArray[0].get("p0", "");
            /*if (isIOS)
                adservice::utility::url::url_replace(img_url, "http://", "https://");
            else
                adservice::utility::url::url_replace(img_url, "https://", "http://");*/
            adObject["image_url"] = img_url;
            adObject["click_type"] = 0;
            adObject["id"] = 1;
        }
        adObject["click_url"] = landingurl;
        {

            cppcms::json::array & click_urls = bidResponse["clickmonitor_urls"].array();
            cppcms::json::array & view_urls = bidResponse["viewmonitor_urls"].array();

            url::URLHelper clickUrlParam;
            getClickPara(clickUrlParam, requestId, "", landingurl);
            clickurl = getClickBaseUrl(isIOS) + "?" + clickUrlParam.cipherParam();
            click_urls.push_back(clickurl);

            url::URLHelper showUrlParam;
            getShowPara(showUrlParam, requestId);
            showUrlParam.add(URL_IMP_OF, "3");
            showUrlParam.add(URL_EXCHANGE_PRICE, os.str());
            monitor_url = getShowBaseUrl(isIOS) + "?" + showUrlParam.cipherParam();
            view_urls.push_back(monitor_url);
        }
        // std::string objStr = toJson(adObject);
        switch (Adtype) {
        case 0:
            bidResponse["splash"] = adObject;
            break;
        case 1:
            bidResponse["list"] = adObject;
            break;
        case 2:
            bidResponse["content"] = adObject;
            break;
        }
        redoCookieMapping(queryCondition.adxid, "");
    }

    void YidianBidingHandler::match(INOUT adservice::utility::HttpResponse & response)
    {
        std::string result = toJson(bidResponse);
        if (result.empty()) {
            LOG_ERROR << "YidianBiddingHandler::match failed to parse obj to json";
            reject(response);
        } else {
            response.status(200);
            response.set_content_header("application/json; charset=utf-8");
            response.set_body(result);
        }
    }

    void YidianBidingHandler::reject(INOUT adservice::utility::HttpResponse & response)
    {
        response.status(204);
        response.set_content_header("application/json; charset=utf-8");
    }
}
}
