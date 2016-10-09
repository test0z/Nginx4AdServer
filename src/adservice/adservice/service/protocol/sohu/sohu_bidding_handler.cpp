//
// Created by guoze.lin on 16/6/27.
//

#include "sohu_bidding_handler.h"
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
	using namespace adservice::server;

    int getSohuDeviceType(const std::string & mobile)
    {
        if (mobile.find("iPhone") != std::string::npos) {
            return SOLUTION_DEVICE_IPHONE;
        } else if (mobile.find("AndroidPhone") != std::string::npos) {
            return SOLUTION_DEVICE_ANDROID;
        } else if (mobile.find("iPad") != std::string::npos) {
            return SOLUTION_DEVICE_IPAD;
        } else if (mobile.find("AndroidPad") != std::string::npos) {
            return SOLUTION_DEVICE_ANDROIDPAD;
        } else
            return SOLUTION_DEVICE_OTHER;
    }

    std::string SohuBiddingHandler::getDisplayPara()
    {
        char showBuf[1024];
        getShowPara(bidRequest.bidid(), showBuf, sizeof(showBuf));
        const char * extShowBuf = "of=0&&p=%%WINPRICE%%";
        strncat(showBuf, extShowBuf, strlen(extShowBuf));
        return std::string(showBuf);
    }

    std::string SohuBiddingHandler::getSohuClickPara(const std::string & url)
    {
        char clickBuf[1024];
        std::string ref = bidRequest.has_site() ? bidRequest.site().page() : "";
        getClickPara(bidRequest.bidid(), clickBuf, sizeof(clickBuf), ref, url);
        return std::string(clickBuf);
    }

    bool SohuBiddingHandler::parseRequestData(const std::string & data)
    {
        bidRequest.Clear();
        return getProtoBufObject(bidRequest, data);
    }

    bool SohuBiddingHandler::fillLogItem(protocol::log::LogItem & logItem)
    {
        logItem.reqStatus = 200;
        logItem.userAgent = bidRequest.device().ua();
        logItem.ipInfo.proxy = bidRequest.device().ip();
        logItem.adInfo.adxid = adInfo.adxid;
        logItem.adInfo.adxpid = adInfo.adxpid;
        if (isBidAccepted) {
            if (bidRequest.has_device()) {
                const Request_Device & device = bidRequest.device();
                if (!(device.type() == "PC")) {
                    logItem.deviceInfo = device.DebugString();
                    adInfo.adxid = logItem.adInfo.adxid = ADX_SOHU_MOBILE;
                }
            }
            logItem.adInfo.sid = adInfo.sid;
            logItem.adInfo.advId = adInfo.advId;
            logItem.adInfo.pid = adInfo.pid;
            logItem.adInfo.adxpid = adInfo.adxpid;
            logItem.adInfo.adxuid = adInfo.adxuid;
            logItem.adInfo.bannerId = adInfo.bannerId;
            logItem.adInfo.cid = adInfo.cid;
            logItem.adInfo.mid = adInfo.mid;
            logItem.adInfo.cpid = adInfo.cpid;
            logItem.adInfo.offerPrice = adInfo.offerPrice;
            logItem.adInfo.areaId = adInfo.areaId;
            logItem.adInfo.priceType = adInfo.priceType;
            logItem.adInfo.ppid = adInfo.ppid;
            url::extractAreaInfo(adInfo.areaId.data(), logItem.geoInfo.country, logItem.geoInfo.province,
                                 logItem.geoInfo.city);
            logItem.adInfo.bidSize = adInfo.bidSize;
            logItem.referer
                = bidRequest.has_site() ? (bidRequest.site().has_page() ? bidRequest.site().page() : "") : "";
		} else {
            logItem.adInfo.pid = adInfo.pid;
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
        AdSelectCondition queryCondition;
        queryCondition.adxid = ADX_SOHU_PC;
        queryCondition.adxpid = pid;
        queryCondition.ip = bidRequest.device().ip();
        if (bidRequest.device().type() == "PC") {
            queryCondition.pcOS = getOSTypeFromUA(bidRequest.device().ua());
            queryCondition.flowType = SOLUTION_FLOWTYPE_PC;
        } else {
            queryCondition.mobileDevice = getSohuDeviceType(bidRequest.device().mobiletype());
            queryCondition.flowType = SOLUTION_FLOWTYPE_MOBILE;
            queryCondition.adxid = ADX_SOHU_MOBILE;
        }
        if (adzInfo.has_banner()) {
            const Request_Impression_Banner & banner = adzInfo.banner();
            queryCondition.width = banner.width();
            queryCondition.height = banner.height();
        } else if (adzInfo.has_video()) {
            const Request_Impression_Video & video = adzInfo.video();
            queryCondition.width = video.width();
            queryCondition.height = video.height();
        }
        if (!filterCb(this, queryCondition)) {
            return bidFailedReturn();
        }
        return isBidAccepted = true;
    }

	void SohuBiddingHandler::buildBidResult(const AdSelectCondition & queryCondition,
											const MT::common::SelectResult & result)
    {
        bidResponse.Clear();
        bidResponse.set_version(bidRequest.version());
        bidResponse.set_bidid(bidRequest.bidid());
        bidResponse.clear_seatbid();
        Response_SeatBid * seatBid = bidResponse.add_seatbid();
        seatBid->set_idx(0);
        Response_Bid * adResult = seatBid->add_bid();
		const MT::common::Solution & finalSolution = result.solution;
		const MT::common::ADPlace & adplace = result.adplace;
		const MT::common::Banner & banner = result.banner;
        int advId = finalSolution.advId;
        // const Request_Impression& adzInfo = bidRequest.impression(0);
        int maxCpmPrice = result.bidPrice;
        adResult->set_price(maxCpmPrice);

        //缓存最终广告结果
        adInfo.pid = queryCondition.mttyPid;
        adInfo.adxpid = queryCondition.adxpid;
		adInfo.sid = finalSolution.sId;
        adInfo.advId = advId;
        adInfo.adxid = queryCondition.adxid;
        adInfo.adxuid = bidRequest.has_user() ? bidRequest.user().suid() : "";
		adInfo.bannerId = banner.bId;
		adInfo.cid = adplace.cId;
		adInfo.mid = adplace.mId;
        adInfo.cpid = adInfo.advId;
        adInfo.offerPrice = maxCpmPrice;
        adInfo.priceType = finalSolution.priceType;
        adInfo.ppid = result.ppid;
        adInfo.bidSize = makeBidSize(banner.width, banner.height);
        const std::string & userIp = bidRequest.device().ip();
        IpManager & ipManager = IpManager::getInstance();
        adInfo.areaId = ipManager.getAreaCodeStrByIp(userIp.data());

        char pjson[2048] = { '\0' };
        std::string strBannerJson = banner.json;
        strncat(pjson, strBannerJson.data(), sizeof(pjson));
        tripslash2(pjson);
        cppcms::json::value bannerJson;
        parseJson(pjson, bannerJson);
        const cppcms::json::array & mtlsArray = bannerJson["mtls"].array();
        std::string materialUrl = mtlsArray[0].get("p0", "");
        std::string landingUrl = mtlsArray[0].get("p1", "");
        adResult->set_adurl(materialUrl);
        //            adResult->set_adpara()
        //            adResult->set_ext()
        adResult->set_displaypara(getDisplayPara());
        adResult->set_clickpara(getSohuClickPara(landingUrl));
        //            adResult->set_adm_url()
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
