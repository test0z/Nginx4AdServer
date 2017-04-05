//
// Created by guoze.lin on 16/6/27.
//

#include "sohu_bidding_handler.h"
#include "core/core_ad_sizemap.h"
#include "core/core_ip_manager.h"
#include "logging.h"
#include "utility/utility.h"

namespace protocol {
namespace bidding {

    using namespace protocol::sohuadx;
    using namespace adservice::utility;
    using namespace adservice::utility::serialize;
    using namespace adservice::utility::json;
    using namespace adservice::utility::userclient;
    using namespace adservice::utility::cypher;
    using namespace adservice::server;

    namespace {

        int getSohuDeviceType(const std::string & mobile)
        {
            if (!strcasecmp(mobile.data(), "iPhone")) {
                return SOLUTION_DEVICE_IPHONE;
            } else if (!strcasecmp(mobile.data(), "AndroidPhone")) {
                return SOLUTION_DEVICE_ANDROID;
            } else if (!strcasecmp(mobile.data(), "iPad")) {
                return SOLUTION_DEVICE_IPAD;
            } else if (!strcasecmp(mobile.data(), "AndroidPad")) {
                return SOLUTION_DEVICE_ANDROIDPAD;
            } else
                return SOLUTION_DEVICE_OTHER;
        }

        int getSohuNeworkType(const std::string & netType)
        {
            if (!strcasecmp(netType.data(), "WIFI")) {
                return SOLUTION_NETWORK_WIFI;
            } else if (!strcasecmp(netType.data(), "4G")) {
                return SOLUTION_NETWORK_4G;
            } else if (!strcasecmp(netType.data(), "3G")) {
                return SOLUTION_NETWORK_3G;
            } else if (!strcasecmp(netType.data(), "2G")) {
                return SOLUTION_NETWORK_2G;
            } else
                return SOLUTION_NETWORK_OTHER;
        }
    }

    bool SohuBiddingHandler::parseRequestData(const std::string & data)
    {
        bidRequest.Clear();
        bidResponse.Clear();
        return getProtoBufObject(bidRequest, data);
    }

    bool SohuBiddingHandler::fillSpecificLog(const AdSelectCondition & selectCondition,
                                             protocol::log::LogItem & logItem, bool isAccepted)
    {
        logItem.userAgent = bidRequest.device().ua();
        logItem.ipInfo.proxy = selectCondition.ip;
        logItem.referer = bidRequest.has_site() && bidRequest.site().has_page() ? bidRequest.site().page() : "";
        if (isAccepted) {
            if (bidRequest.has_device()) {
                const Request_Device & device = bidRequest.device();
                if (!(device.type() == "PC")) {
                    logItem.deviceInfo = device.DebugString();
                    adInfo.adxid = logItem.adInfo.adxid = ADX_SOHU_MOBILE;
                }
            }
        }
        return true;
    }

    bool SohuBiddingHandler::filter(const BiddingFilterCallback & filterCb)
    {
        if (bidRequest.istest() != 0) {
            return bidFailedReturn();
        }
        //从BID Request中获取请求的广告位信息,目前只取第一个
        if (bidRequest.impression_size() <= 0)
            return bidFailedReturn();
        const Request_Impression & adzInfo = bidRequest.impression(0);
        const std::string & pid = adzInfo.pid();
        std::vector<AdSelectCondition> queryConditions{ AdSelectCondition() };
        std::vector<PreSetAdplaceInfo> adplaceInfos{ PreSetAdplaceInfo() };
        AdSelectCondition & queryCondition = queryConditions[0];
        PreSetAdplaceInfo & pAdplaceInfo = adplaceInfos[0];
        queryCondition.pAdplaceInfo = &pAdplaceInfo;
        queryCondition.adxid = ADX_SOHU_PC;
        queryCondition.adxpid = pid;
        queryCondition.basePrice = adzInfo.has_bidfloor() ? adzInfo.bidfloor() : 0;
        if (bidRequest.has_device()) {
            auto & device = bidRequest.device();
            queryCondition.ip = device.has_ip() ? device.ip() : "";
            queryCondition.mobileDevice = getMobileTypeFromUA(device.ua());
            queryCondition.pcOS = getOSTypeFromUA(device.ua());
            if (strcasecmp(device.type().data(), "Mobile") == 0) { // mobile
                queryCondition.mobileDevice = getSohuDeviceType(device.mobiletype());
                queryCondition.flowType = SOLUTION_FLOWTYPE_MOBILE;
                queryCondition.adxid = ADX_SOHU_MOBILE;
                queryCondition.mobileNetwork = getSohuNeworkType(device.nettype());
                queryCondition.idfa = device.has_idfa() ? stringtool::toupper(device.idfa()) : "";
                queryCondition.imei = device.has_imei() ? stringtool::toupper(device.imei()) : "";
                queryCondition.androidId = device.has_androidid() ? stringtool::toupper(device.androidid()) : "";
                queryCondition.mac = device.has_mac() ? stringtool::toupper(device.mac()) : "";
                cookieMappingKeyMobile(md5_encode(queryCondition.idfa),
                                       md5_encode(queryCondition.imei),
                                       md5_encode(queryCondition.androidId),
                                       md5_encode(queryCondition.mac),
                                       queryCondition,
                                       queryCondition.adxid,
                                       bidRequest.has_user() && bidRequest.user().has_suid() ? bidRequest.user().suid()
                                                                                             : "");
            } else if (queryCondition.mobileDevice != SOLUTION_DEVICE_OTHER
                       || strcasecmp(device.type().data(), "Wap") == 0) { // wap
                queryCondition.mobileDevice = queryCondition.mobileDevice == SOLUTION_DEVICE_OTHER
                                                  ? getSohuDeviceType(device.mobiletype())
                                                  : queryCondition.mobileDevice;
                queryCondition.flowType = SOLUTION_FLOWTYPE_MOBILE;
                queryCondition.adxid = ADX_SOHU_MOBILE;
                queryCondition.mobileNetwork = getSohuNeworkType(device.nettype());
                cookieMappingKeyWap(ADX_SOHU_MOBILE, bidRequest.has_user() && bidRequest.user().has_suid()
                                                         ? bidRequest.user().suid()
                                                         : "");
            } else { // pc
                queryCondition.pcOS = getOSTypeFromUA(device.ua());
                queryCondition.flowType = SOLUTION_FLOWTYPE_PC;
                queryCondition.mac = device.has_mac() ? device.mac() : "";
                cookieMappingKeyPC(
                    ADX_SOHU_PC, bidRequest.has_user() && bidRequest.user().has_suid() ? bidRequest.user().suid() : "");
            }
        } else {
            cookieMappingKeyPC(ADX_SOHU_PC,
                               bidRequest.has_user() && bidRequest.user().has_suid() ? bidRequest.user().suid() : "");
        }
        queryCookieMapping(cmInfo.queryKV, queryCondition);
        if (adzInfo.has_banner()) {
            const Request_Impression_Banner & banner = adzInfo.banner();
            queryCondition.width = banner.width();
            queryCondition.height = banner.height();
            if (queryCondition.flowType == SOLUTION_FLOWTYPE_PC) {
                queryCondition.bannerType = BANNER_TYPE_PIC;
            } else {
                queryCondition.bannerType = BANNER_TYPE_MOBILE;
            }
        } else if (adzInfo.has_video()) {
            const Request_Impression_Video & video = adzInfo.video();
            queryCondition.width = video.width();
            queryCondition.height = video.height();
            queryCondition.bannerType = BANNER_TYPE_VIDEO;
        }
        if (queryCondition.flowType == SOLUTION_FLOWTYPE_MOBILE) {
            const adservice::utility::AdSizeMap & sizeMap = adservice::utility::AdSizeMap::getInstance();
            auto size = sizeMap.get({ queryCondition.width, queryCondition.height });
            for (auto sizeIter : size) {
                queryCondition.width = sizeIter.first;
                queryCondition.height = sizeIter.second;
                pAdplaceInfo.sizeArray.push_back({ queryCondition.width, queryCondition.height });
            }
        }
        if (adzInfo.has_ispreferreddeals() && adzInfo.ispreferreddeals()) {
            const std::string & dealId = adzInfo.campaignid();
            queryCondition.dealId = std::string(",") + dealId + ",";
        }
        if (!filterCb(this, queryConditions)) {
            return bidFailedReturn();
        }
        return isBidAccepted = true;
    }

    void SohuBiddingHandler::buildBidResult(const AdSelectCondition & queryCondition,
                                            const MT::common::SelectResult & result, int seq)
    {
        if (!bidResponse.has_bidid()) {
            bidResponse.Clear();
            bidResponse.set_version(bidRequest.version());
            bidResponse.set_bidid(bidRequest.bidid());
            bidResponse.clear_seatbid();
        }
        Response_SeatBid * seatBid = bidResponse.add_seatbid();
        seatBid->set_idx(seq);
        Response_Bid * adResult = seatBid->add_bid();
        const MT::common::Banner & banner = result.banner;
        const Request_Impression & adzInfo = bidRequest.impression(seq);
        int maxCpmPrice = result.bidPrice;
        adResult->set_price(maxCpmPrice);

        //缓存最终广告结果
        fillAdInfo(queryCondition, result, bidRequest.has_user() ? bidRequest.user().suid() : "");

        bool isIOS = queryCondition.mobileDevice == SOLUTION_DEVICE_IPHONE
                     || queryCondition.mobileDevice == SOLUTION_DEVICE_IPAD;
        std::string strBannerJson = banner.json;
        cppcms::json::value bannerJson = bannerJson2HttpsIOS(isIOS, strBannerJson, banner.bannerType);
        const cppcms::json::array & mtlsArray = bannerJson["mtls"].array();
        std::string materialUrl;
        std::string landingUrl;
        if (banner.bannerType == BANNER_TYPE_PRIMITIVE) {
            materialUrl = mtlsArray[0].get("p6", "");
            landingUrl = mtlsArray[0].get("p9", "");
        } else {
            materialUrl = mtlsArray[0].get("p0", "");
            landingUrl = mtlsArray[0].get("p1", "");
        }
        adResult->set_adurl(materialUrl);

        url::URLHelper showUrlParam;
        getShowPara(showUrlParam, bidRequest.bidid());
        showUrlParam.add(URL_IMP_OF, "3");
        url::URLHelper clickUrlParam;
        getClickPara(clickUrlParam, bidRequest.bidid(), "", landingUrl);
        if (!queryCondition.dealId.empty()) {
            showUrlParam.add(URL_DEAL_ID, adzInfo.campaignid());
            clickUrlParam.add(URL_DEAL_ID, adzInfo.campaignid());
        }
        adResult->set_displaypara(showUrlParam.cipherParam());
        adResult->set_clickpara(clickUrlParam.cipherParam());
        redoCookieMapping(queryCondition.adxid, "http://t.go.sohu.com/cm.gif?ver=1&mid=10123");
    }

    void SohuBiddingHandler::match(adservice::utility::HttpResponse & response)
    {
        std::string result;
        if (!writeProtoBufObject(bidResponse, &result)) {
            LOG_ERROR << "failed to write protobuf object in TanxBiddingHandler::match";
            reject(response);
            return;
        }
        response.status(200);
        response.set_content_header("application/x-protobuf");
        response.set_body(result);
    }

    void SohuBiddingHandler::reject(adservice::utility::HttpResponse & response)
    {
        bidResponse.Clear();
        bidResponse.set_version(bidRequest.version());
        bidResponse.set_bidid(bidRequest.bidid());
        std::string result;
        if (!writeProtoBufObject(bidResponse, &result)) {
            LOG_ERROR << "failed to write protobuf object in TanxBiddingHandler::reject";
            return;
        }
        response.status(200);
        response.set_content_header("application/x-protobuf");
        response.set_body(result);
    }
}
}
