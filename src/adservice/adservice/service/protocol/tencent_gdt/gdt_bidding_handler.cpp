//
// Created by guoze.lin on 16/5/11.
//
#include <string>

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
    static GdtSizeMap gdtAdplaceSizeMap;

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

    bool GdtBiddingHandler::fillLogItem(protocol::log::LogItem & logItem)
    {
        logItem.reqStatus = 200;
        logItem.ipInfo.proxy = bidRequest.ip();
        logItem.adInfo.adxid = adInfo.adxid;
        logItem.adInfo.adxpid = adInfo.adxpid;
        if (isBidAccepted) {
            if (bidRequest.has_device()) {
                const BidRequest_Device & device = bidRequest.device();
                logItem.deviceInfo = device.DebugString();
            }
            logItem.adInfo.sid = adInfo.sid;
            logItem.adInfo.advId = adInfo.advId;
            logItem.adInfo.adxid = adInfo.adxid;
            logItem.adInfo.adxpid = adInfo.adxpid;
            logItem.adInfo.adxuid = adInfo.adxuid;
            logItem.adInfo.bannerId = adInfo.bannerId;
            logItem.adInfo.cid = adInfo.cid;
            logItem.adInfo.mid = adInfo.mid;
            logItem.adInfo.cpid = adInfo.cpid;
            logItem.adInfo.offerPrice = adInfo.offerPrice;
            logItem.adInfo.priceType = adInfo.priceType;
            logItem.adInfo.ppid = adInfo.ppid;
            url::extractAreaInfo(adInfo.areaId.data(), logItem.geoInfo.country, logItem.geoInfo.province,
                                 logItem.geoInfo.city);
            logItem.adInfo.bidSize = adInfo.bidSize;
		} else {
			logItem.adInfo.pid = adInfo.pid;
            logItem.adInfo.bidSize = adInfo.bidSize;
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
        long pid = adzInfo.placement_id();
        AdSelectCondition queryCondition;
        queryCondition.adxid = ADX_TENCENT_GDT;
        queryCondition.adxpid = std::to_string(pid);
        queryCondition.ip = bidRequest.ip();
        IpManager & ipManager = IpManager::getInstance();
        queryCondition.dGeo = ipManager.getAreaByIp(queryCondition.ip.data());
        PreSetAdplaceInfo adplaceInfo;
        for (int i = 0; i < adzInfo.creative_specs_size(); i++) {
            int createspecs = adzInfo.creative_specs(i);
            if (gdtAdplaceMap.find(createspecs)) {
                GdtAdplace & gdtAdplace = gdtAdplaceMap.get(createspecs);
                const std::pair<int, int> & sizePair
					= gdtAdplaceSizeMap.get(std::make_pair(gdtAdplace.width, gdtAdplace.height));
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
            } else if (devType == BidRequest_DeviceType::BidRequest_DeviceType_kDeviceTypeMobile) {
                adplaceInfo.flowType = SOLUTION_FLOWTYPE_MOBILE;
                queryCondition.flowType = SOLUTION_FLOWTYPE_MOBILE;
                queryCondition.adxid = ADX_GDT_MOBILE;
                queryCondition.mobileDevice = getGdtMobileDeviceType(device.os());
            } else if (devType == BidRequest_DeviceType::BidRequest_DeviceType_kDeviceTypePad) {
                queryCondition.mobileDevice = device.os() == BidRequest_OperatingSystem_kOSIOS
                                                  ? SOLUTION_DEVICE_IPAD
                                                  : SOLUTION_DEVICE_ANDROIDPAD;
            } else {
                queryCondition.mobileDevice = SOLUTION_DEVICE_OTHER;
                queryCondition.pcOS = SOLUTION_OS_OTHER;
            }
        }
        queryCondition.pAdplaceInfo = &adplaceInfo;
        if (!filterCb(this, queryCondition)) {
            adInfo.adxid = queryCondition.adxid;
            adInfo.bidSize = makeBidSize(queryCondition.width, queryCondition.height);
            return bidFailedReturn();
        }

        return isBidAccepted = true;
    }

	void GdtBiddingHandler::buildBidResult(const AdSelectCondition & queryCondition,
										   const MT::common::SelectResult & result)
    {
        bidResponse.Clear();
        bidResponse.set_request_id(bidRequest.id());
        bidResponse.clear_seat_bids();
        BidResponse_SeatBid * seatBid = bidResponse.add_seat_bids();
		const MT::common::Solution & finalSolution = result.solution;
		const MT::common::ADPlace & adplace = result.adplace;
		const MT::common::Banner & banner = result.banner;
        // int advId = finalSolution.advId;
        const BidRequest_Impression & adzInfo = bidRequest.impressions(0);
        seatBid->set_impression_id(adzInfo.id());
        BidResponse_Bid * adResult = seatBid->add_bids();
        int maxCpmPrice = max(result.bidPrice, adzInfo.bid_floor());
        adResult->set_bid_price(maxCpmPrice);
		adResult->set_creative_id(std::to_string(banner.bId));
        //缓存最终广告结果
		adInfo.pid = std::to_string(adplace.pId);
        adInfo.adxpid = queryCondition.adxpid;
        adInfo.advId = finalSolution.advId;
		adInfo.sid = finalSolution.sId;
        adInfo.adxid = queryCondition.adxid;
        adInfo.adxuid = bidRequest.user().id();
		adInfo.bannerId = banner.bId;
        adInfo.cpid = adInfo.advId;
        adInfo.offerPrice = maxCpmPrice;
        adInfo.priceType = finalSolution.priceType;
        adInfo.ppid = result.ppid;
        adInfo.bidSize = makeBidSize(banner.width, banner.height);

        const std::string & userIp = bidRequest.ip();
        IpManager & ipManager = IpManager::getInstance();
        adInfo.areaId = ipManager.getAreaCodeStrByIp(userIp.data());

        // html snippet相关
        char showParam[2048];
        getShowPara(bidRequest.id(), showParam, sizeof(showParam));
        strncat(showParam, "&of=3", 5);
        adResult->set_click_param(showParam);
        adResult->set_impression_param(showParam);
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
