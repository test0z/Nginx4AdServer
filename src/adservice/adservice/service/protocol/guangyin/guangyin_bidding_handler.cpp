#include "guangyin_bidding_handler.h"

#include "core/core_ip_manager.h"
#include "logging.h"
#include "utility/utility.h"
#include <boost/algorithm/string.hpp>

namespace protocol {
namespace bidding {

    namespace {

        // 广告投放后的追踪url，用来统计展现量，SSP在广告展示时访问，需要支持302
        const std::string IURL_PLACEHOLDER{ "##IURL##" }; // show
        // 竞价成功后通知url，用来计费，SSP在广告展示时访问，需要支持302
        const std::string NURL_PLACEHOLDER{ "##NURL##" };
        // 广告点击url，用来统计点击数量，SSP在广告展示时访问，需要支持302
        const std::string CURL_PLACEHOLDER{ "##CURL##" }; // click

        int getDeviceType(const Device & device)
        {
            if (boost::iequals(device.os(), "iOS")) {
                if (boost::iequals(device.model(), "iPhone")) {
                    return SOLUTION_DEVICE_IPHONE;
                } else if (boost::iequals(device.model(), "iPad")) {
                    return SOLUTION_DEVICE_IPAD;
                }
            } else if (boost::iequals(device.os(), "Android")) {
                return SOLUTION_DEVICE_ANDROID;
            }

            return SOLUTION_DEVICE_OTHER;
        }

        int getNetWork(int network)
        {
            switch (network) {
            case ConnectionType::UNKNOWN_TYPE:
                return SOLUTION_NETWORK_UNKNOWN;
            case ConnectionType::ETHERNET:
                return SOLUTION_NETWORK_OTHER;
            case ConnectionType::WIFI:
                return SOLUTION_NETWORK_WIFI;
            case ConnectionType::CELLULAR_NETWORK_UNKNOWN_GENERATION:
                return SOLUTION_NETWORK_UNKNOWN;
            case ConnectionType::CELLULAR_NETWORK_2G:
                return SOLUTION_NETWORK_2G;
            case ConnectionType::CELLULAR_NETWORK_3G:
                return SOLUTION_NETWORK_3G;
            case ConnectionType::CELLULAR_NETWORK_4G:
                return SOLUTION_NETWORK_4G;
            }

            return SOLUTION_NETWORK_UNKNOWN;
        }

        bool replace(std::string & str, const std::string & from, const std::string & to)
        {
            size_t start_pos = str.find(from);
            if (start_pos == std::string::npos)
                return false;
            str.replace(start_pos, from.length(), to);
            return true;
        }

		void nativeiframe(const MT::common::Banner & banner, protocol::log::AdInfo & adInfo, std::string & htmlTemplate)
        {

            std::map<std::string, std::string> replaces;
            //{ "{advid}", "{bid}", "{xid}", "{geoid}", "{width}", "{height}" }
            replaces["{advid}"] = std::to_string(adInfo.advId);
            replaces["{bid}"] = std::to_string(adInfo.bannerId);
            replaces["{xid}"] = std::to_string(adInfo.adxid);
            replaces["{geoid}"] = adInfo.areaId;
            replaces["{width}"] = std::to_string(banner.width);
            replaces["{height}"] = std::to_string(banner.height);
            for (auto & k : replaces) {
                replace(htmlTemplate, k.first, k.second);
            }
        }

    } // namespace anonyous

    bool GuangyinBiddingHandler::parseRequestData(const std::string & data)
    {
        bidRequest_.Clear();
        return adservice::utility::serialize::getProtoBufObject(bidRequest_, data);
    }

    bool GuangyinBiddingHandler::fillLogItem(protocol::log::LogItem & logItem)
    {
        logItem.reqStatus = 200;
        logItem.userAgent = bidRequest_.device().ua();
        logItem.ipInfo.proxy = bidRequest_.device().ip();
        logItem.adInfo.adxid = adInfo.adxid;
        logItem.adInfo.adxpid = adInfo.adxpid;
        if (isBidAccepted) {
            if (bidRequest_.device().devicetype() == DeviceType::MOBILE) {
                logItem.deviceInfo = bidRequest_.device().DebugString();
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
            adservice::utility::url::extractAreaInfo(adInfo.areaId.data(), logItem.geoInfo.country,
                                                     logItem.geoInfo.province, logItem.geoInfo.city);
            logItem.adInfo.bidSize = adInfo.bidSize;
            logItem.adInfo.priceType = adInfo.priceType;
            logItem.adInfo.ppid = adInfo.ppid;
            logItem.referer = bidRequest_.has_site() ? bidRequest_.site().page() : "";
        } else {
            logItem.adInfo.pid = adInfo.pid;
            logItem.adInfo.bidSize = adInfo.bidSize;
        }

        return true;
    }

    bool GuangyinBiddingHandler::filter(const BiddingFilterCallback & filterCb)
    {
        if (bidRequest_.test() == 1) {
            return bidFailedReturn();
        }

        //从BID Request中获取请求的广告位信息,目前只取第一个
        if (bidRequest_.imp().size() == 0) {
            return bidFailedReturn();
        }
        Device device = bidRequest_.device();
        Imp imp = bidRequest_.imp(0);
        const Banner & banner = imp.banner();

        AdSelectCondition queryCondition;
        queryCondition.adxid = ADX_GUANGYIN;
        queryCondition.adxpid = imp.id();
        queryCondition.ip = device.ip();
        queryCondition.basePrice = imp.bidfloor();
        queryCondition.width = banner.w();
        queryCondition.height = banner.h();

        if (device.devicetype() == DeviceType::MOBILE) {
            queryCondition.mobileDevice = getDeviceType(device);
            queryCondition.flowType = SOLUTION_FLOWTYPE_MOBILE;
            queryCondition.adxid = ADX_GUANGYIN_MOBILE;
            const std::string & deviceId = device.idfa().empty() ? device.androidid() : device.idfa();
            strncpy(biddingFlowInfo.deviceIdBuf, deviceId.c_str(), deviceId.size());

            queryCondition.mobileNetwork = getNetWork(device.connectiontype());
            if (bidRequest_.has_app()) {
                const App & app = bidRequest_.app();
				if (app.has_name()) {
                    queryCondition.adxpid = app.name();
				} else if (app.has_publisher()) {
                    queryCondition.adxpid = app.publisher().slot();
                }
            }
        } else {
            queryCondition.pcOS = adservice::utility::userclient::getOSTypeFromUA(device.ua());
            queryCondition.pcBrowserStr = adservice::utility::userclient::getBrowserTypeFromUA(device.ua());
            queryCondition.flowType = SOLUTION_FLOWTYPE_PC;
            if (bidRequest_.has_site()) {
                const Site & site = bidRequest_.site();
                if (site.has_publisher()) {
                    queryCondition.adxpid = site.publisher().slot();
                }
            }
        }

        if (!filterCb(this, queryCondition)) {
            adInfo.adxpid = queryCondition.adxpid;
            adInfo.adxid = queryCondition.adxid;
            adInfo.bidSize = makeBidSize(queryCondition.width, queryCondition.height);
            return bidFailedReturn();
        }

        return isBidAccepted = true;
    }

	void GuangyinBiddingHandler::buildBidResult(const AdSelectCondition & queryCondition,
												const MT::common::SelectResult & result)
    {
        bidResponse_.Clear();
        bidResponse_.set_id(bidRequest_.id());
        bidResponse_.set_bidid(adservice::utility::cypher::randomId(3));
        bidResponse_.clear_seatbid();

		const MT::common::Solution & finalSolution = result.solution;
		const MT::common::ADPlace & adplace = result.adplace;
		const MT::common::Banner & banner = result.banner;

        //缓存最终广告结果
		adInfo.pid = std::to_string(adplace.pId);
		adInfo.adxpid = queryCondition.adxpid;
		adInfo.sid = finalSolution.sId;

		auto advId = finalSolution.advId;
        adInfo.advId = advId;
        adInfo.adxid = queryCondition.adxid;
        adInfo.adxuid = bidRequest_.user().id();
		adInfo.bannerId = banner.bId;
		adInfo.cid = adplace.cId;
		adInfo.mid = adplace.mId;
		adInfo.cpid = adInfo.advId;
		adInfo.bidSize = makeBidSize(banner.width, banner.height);
        adInfo.priceType = finalSolution.priceType;
        adInfo.ppid = result.ppid;

        const Imp & imp = bidRequest_.imp(0);
		float maxCpmPrice = std::max((float)result.bidPrice, imp.bidfloor());
        if (adInfo.priceType != PRICETYPE_RRTB_CPC) {
            adInfo.offerPrice = maxCpmPrice;
        } else {
            adInfo.offerPrice = result.bidPrice;
        }

        const std::string & userIp = bidRequest_.device().ip();
        adInfo.areaId = adservice::server::IpManager::getInstance().getAreaCodeStrByIp(userIp.c_str());

        SeatBid * seatBid = bidResponse_.add_seatbid();
		seatBid->set_seat("5761460d3ada7515003d3589");
        Bid * adResult = seatBid->add_bid();

        adResult->set_id(bidRequest_.id());
        adResult->set_impid(imp.id());
        adResult->set_price(maxCpmPrice);

        const Banner & reqBanner = imp.banner();
        adResult->set_adomain("show.mtty.com");

        char buffer[2048];
        getShowPara(bidRequest_.id(), buffer, sizeof(buffer));
        adResult->set_iurl(std::string(SNIPPET_SHOW_URL) + "?" + buffer + "&of=3&p=%%AUCTION_PRICE%%");

        cppcms::json::value bannerJson;
        std::stringstream ss;
        ss << boost::algorithm::erase_all_copy(banner.json, "\\");
        ss >> bannerJson;
        const cppcms::json::array & mtlsArray = bannerJson["mtls"].array();
        if (banner.bannerType == 5) {
            std::string base64html = mtlsArray[0].get("p0", "");
            std::string html;
            adservice::utility::cypher::urlsafe_base64decode(base64html, html);
            nativeiframe(banner, adInfo, html);
            adResult->set_adm(html);
        } else {
            adResult->set_adm(std::string("<iframe width=\"" + std::to_string(reqBanner.w()) + "\" height=\""
                                          + std::to_string(reqBanner.h())
                                          + "\" frameborder=\"0\" scrolling=\"no\" src=\""
                                          + std::string(SNIPPET_SHOW_URL)
                                          + "?"
                                          + buffer
                                          + "&of=2\"></iframe><!--<img src=\"&\"/>-->"));
        }
        std::string landingUrl = mtlsArray[0].get("p1", "");
        getClickPara(bidRequest_.id(), buffer, sizeof(buffer), "", landingUrl);
        adResult->set_curl(std::string("http://click.mtty.com/c?") + buffer);

        adResult->set_w(banner.width);
        adResult->set_h(banner.height);

        adResult->set_type(AdType::IFRAME);
        adResult->set_admtype(AdmType::HTML);
    }

    void GuangyinBiddingHandler::match(adservice::utility::HttpResponse & response)
    {
        std::string result;
        if (!adservice::utility::serialize::writeProtoBufObject(bidResponse_, &result)) {
			LOG_ERROR << "failed to write protobuf object in GuangyinBiddingHandler::match";
            reject(response);
            return;
        }
        response.status(200);
        response.set_body(result);
    }

    void GuangyinBiddingHandler::reject(adservice::utility::HttpResponse & response)
    {
        bidResponse_.Clear();
        bidResponse_.set_id(bidRequest_.id());
        bidResponse_.set_bidid(adservice::utility::cypher::randomId(3));
        bidResponse_.set_nbr(NoBidReasonCodes::TECHNICAL_ERROR);
        std::string result;
        if (!adservice::utility::serialize::writeProtoBufObject(bidResponse_, &result)) {
			LOG_ERROR << "failed to write protobuf object in GuangyinBiddingHandler::reject";
            return;
        }
        response.status(200);
        response.set_body(result);
    }

} // namespace protocol
} // namespace adservice
