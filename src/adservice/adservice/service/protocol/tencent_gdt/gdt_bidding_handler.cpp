//
// Created by guoze.lin on 16/5/11.
//
#include <string>

#include "core/core_ad_sizemap.h"
#include "core/core_ip_manager.h"
#include "gdt_bidding_handler.h"
#include "logging.h"
#include "utility/utility.h"

namespace protocol {
namespace bidding {

    using namespace protocol::gdt::adx;
    using namespace adservice::utility;
    using namespace adservice::utility::serialize;
    using namespace adservice::utility::userclient;
    using namespace adservice::server;

    static GdtAdplaceMap gdtAdplaceMap;

    inline int max(const int & a, const int & b)
    {
        return a > b ? a : b;
    }

    int getGdtOsType(BidRequest_OperatingSystem os)
    {
        switch (os) {
        case BidRequest_OperatingSystem::BidRequest_OperatingSystem_kOSWindows:
            return SOLUTION_OS_WINDOWS;
        default:
            return SOLUTION_OS_OTHER;
        }
    }

    int getGdtMobileDeviceType(BidRequest_OperatingSystem os)
    {
        switch (os) {
        case BidRequest_OperatingSystem::BidRequest_OperatingSystem_kOSWindows:
            return SOLUTION_DEVICE_WINDOWSPHONE;
        case BidRequest_OperatingSystem::BidRequest_OperatingSystem_kOSAndroid:
            return SOLUTION_DEVICE_ANDROID;
        case BidRequest_OperatingSystem::BidRequest_OperatingSystem_kOSIOS:
            return SOLUTION_DEVICE_IPHONE;
        default:
            return SOLUTION_DEVICE_OTHER;
        }
    }

    bool GdtBiddingHandler::parseRequestData(const std::string & data)
    {
        bidRequest.Clear();
        return getProtoBufObject(bidRequest, data);
    }

    bool GdtBiddingHandler::fillSpecificLog(const AdSelectCondition & selectCondition, protocol::log::LogItem & logItem,
                                            bool isAccepted)
    {
        logItem.ipInfo.proxy = selectCondition.ip;
        if (isAccepted) {
            if (bidRequest.has_device()) {
                const BidRequest_Device & device = bidRequest.device();
                logItem.deviceInfo = device.DebugString();
            }
        }
        return true;
    }

    bool GdtBiddingHandler::filter(const BiddingFilterCallback & filterCb)
    {
        if (bidRequest.is_ping() || bidRequest.is_test()) {
            return bidFailedReturn();
        }
        //从BID Request中获取请求的广告位信息,目前只取第一个
        const BidRequest_Impression & adzInfo = bidRequest.impressions(0);
        std::vector<AdSelectCondition> queryConditions{ AdSelectCondition() };
        AdSelectCondition & queryCondition = queryConditions[0];
        queryCondition.adxid = ADX_TENCENT_GDT;
        queryCondition.ip = bidRequest.ip();
        queryCondition.basePrice = adzInfo.has_bid_floor() ? adzInfo.bid_floor() : 0;
        IpManager & ipManager = IpManager::getInstance();
        queryCondition.dGeo = ipManager.getAreaByIp(queryCondition.ip.data());
        PreSetAdplaceInfo adplaceInfo;
        const adservice::utility::AdSizeMap & adSizeMap = adservice::utility::AdSizeMap::getInstance();
        for (int i = 0; i < adzInfo.creative_specs_size(); i++) {
            int createspecs = adzInfo.creative_specs(i);
            if (gdtAdplaceMap.find(createspecs)) {
                GdtAdplace & gdtAdplace = gdtAdplaceMap.get(createspecs);
                const std::pair<int, int> & sizePair
                    = adSizeMap.get(std::make_pair(gdtAdplace.width, gdtAdplace.height));
                adplaceInfo.sizeArray.push_back(std::make_pair(sizePair.first, sizePair.second));
                adplaceInfo.flowType = gdtAdplace.flowType;
                queryCondition.width = sizePair.first;
                queryCondition.height = sizePair.second;
                queryCondition.flowType = gdtAdplace.flowType;
            }
        }
        if (adplaceInfo.sizeArray.size() == 0) {
            return bidFailedReturn();
        }
        if (bidRequest.has_device()) { // device
            const BidRequest_Device & device = bidRequest.device();
            BidRequest_DeviceType devType = device.device_type();
            if (devType == BidRequest_DeviceType::BidRequest_DeviceType_kDeviceTypePC) {
                queryCondition.pcOS = getGdtOsType(device.os());
                queryCondition.pcBrowserStr = getBrowserTypeFromUA(device.user_agent());
                if (queryCondition.pcOS == SOLUTION_OS_OTHER) {
                    queryCondition.pcOS = getOSTypeFromUA(device.user_agent());
                }
                queryCondition.mac = device.id();
            } else if (devType == BidRequest_DeviceType::BidRequest_DeviceType_kDeviceTypeMobile) {
                adplaceInfo.flowType = SOLUTION_FLOWTYPE_MOBILE;
                queryCondition.flowType = SOLUTION_FLOWTYPE_MOBILE;
                queryCondition.adxid = ADX_GDT_MOBILE;
                queryCondition.mobileDevice = getGdtMobileDeviceType(device.os());
                if (queryCondition.mobileDevice == SOLUTION_DEVICE_IPHONE) {
                    queryCondition.idfa = device.id();
                } else {
                    queryCondition.imei = device.id();
                }
            } else if (devType == BidRequest_DeviceType::BidRequest_DeviceType_kDeviceTypePad) {
                adplaceInfo.flowType = SOLUTION_FLOWTYPE_MOBILE;
                queryCondition.flowType = SOLUTION_FLOWTYPE_MOBILE;
                queryCondition.adxid = ADX_GDT_MOBILE;
                queryCondition.mobileDevice = device.os() == BidRequest_OperatingSystem_kOSIOS
                                                  ? SOLUTION_DEVICE_IPAD
                                                  : SOLUTION_DEVICE_ANDROIDPAD;
                if (queryCondition.mobileDevice == SOLUTION_DEVICE_IPAD) {
                    queryCondition.idfa = device.id();
                } else {
                    queryCondition.imei = device.id();
                }
            } else {
                queryCondition.mobileDevice = SOLUTION_DEVICE_OTHER;
                queryCondition.pcOS = SOLUTION_OS_OTHER;
            }
            cookieMappingKeyMobile(md5_encode(queryCondition.idfa),
                                   md5_encode(queryCondition.imei),
                                   md5_encode(queryCondition.androidId),
                                   md5_encode(queryCondition.mac));
            queryCookieMapping(cmInfo.queryKV, queryCondition);
        }
        if (queryCondition.flowType == SOLUTION_FLOWTYPE_MOBILE && bidRequest.has_app()) {
            const BidRequest_App & app = bidRequest.app();
            if (app.has_app_bundle_id()) {
                queryCondition.adxpid = app.app_bundle_id();
            }
        } else if (adzInfo.has_placement_id()) {
            queryCondition.adxpid = adzInfo.placement_id();
        }
        queryCondition.pAdplaceInfo = &adplaceInfo;
        if (!filterCb(this, queryConditions)) {
            return bidFailedReturn();
        }

        return isBidAccepted = true;
    }

    void GdtBiddingHandler::buildBidResult(const AdSelectCondition & queryCondition,
                                           const MT::common::SelectResult & result, int seq)
    {
        if (seq == 0) {
            bidResponse.Clear();
            bidResponse.set_request_id(bidRequest.id());
            bidResponse.clear_seat_bids();
        }
        redoCookieMapping(ADX_GDT_MOBILE, "");
        BidResponse_SeatBid * seatBid = bidResponse.add_seat_bids();
        const MT::common::Banner & banner = result.banner;
        const BidRequest_Impression & adzInfo = bidRequest.impressions(seq);
        seatBid->set_impression_id(adzInfo.id());
        BidResponse_Bid * adResult = seatBid->add_bids();
        int maxCpmPrice = result.bidPrice;
        adResult->set_bid_price(maxCpmPrice);
        adResult->set_creative_id(std::to_string(banner.bId));
        //缓存最终广告结果
        fillAdInfo(queryCondition, result, bidRequest.has_user() ? bidRequest.user().id() : "");

        // html snippet相关
        url::URLHelper showUrlParam;
        getShowPara(showUrlParam, bidRequest.id());
        showUrlParam.add(URL_IMP_OF, "3");
        adResult->set_impression_param(showUrlParam.cipherParam());
        url::URLHelper clickUrlParam;
        getClickPara(clickUrlParam, bidRequest.id(), "", "");
        adResult->set_click_param(clickUrlParam.cipherParam());
    }

    void GdtBiddingHandler::match(adservice::utility::HttpResponse & response)
    {
        std::string result;
        if (!writeProtoBufObject(bidResponse, &result)) {
            LOG_ERROR << "failed to write protobuf object in GdtBiddingHandler::match";
            reject(response);
            return;
        }
        response.set_content_header("application/x-protobuf");
        response.status(200);
        response.set_body(result);
    }

    void GdtBiddingHandler::reject(adservice::utility::HttpResponse & response)
    {
        bidResponse.Clear();
        bidResponse.set_request_id(bidRequest.id());
        std::string result;
        if (!writeProtoBufObject(bidResponse, &result)) {
            LOG_ERROR << "failed to write protobuf object in GdtBiddingHandler::reject";
            return;
        }
        response.set_content_header("application/x-protobuf");
        response.status(200);
        response.set_body(result);
    }
}
}
