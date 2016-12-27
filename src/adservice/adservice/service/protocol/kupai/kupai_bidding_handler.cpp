#include "kupai_bidding_handler.h"

#include "core/core_ad_sizemap.h"
#include "core/core_ip_manager.h"
#include "logging.h"
#include "utility/utility.h"
#include <boost/algorithm/string.hpp>

namespace protocol {
namespace bidding {

    using namespace com::google::openrtb;
    using namespace adservice::utility;
    using namespace adservice::utility::url;

    static KupaiAdplaceStyleMap adplaceStyleMap;

    namespace {

        int getDeviceType(const BidRequest_Device_DeviceType & device)
        {
            if (device != BidRequest_Device_DeviceType_TABLET) {
                return SOLUTION_DEVICE_ANDROID;
            } else {
                return SOLUTION_DEVICE_ANDROIDPAD;
            }
        }

        int getNetWork(const BidRequest_Device_ConnectionType & network)
        {
            switch (network) {
            case BidRequest_Device_ConnectionType_CONNECTION_UNKNOWN:
                return SOLUTION_NETWORK_UNKNOWN;
            case BidRequest_Device_ConnectionType_ETHERNET:
                return SOLUTION_NETWORK_OTHER;
            case BidRequest_Device_ConnectionType_WIFI:
                return SOLUTION_NETWORK_WIFI;
            case BidRequest_Device_ConnectionType_CELL_UNKNOWN:
                return SOLUTION_NETWORK_UNKNOWN;
            case BidRequest_Device_ConnectionType_CELL_2G:
                return SOLUTION_NETWORK_2G;
            case BidRequest_Device_ConnectionType_CELL_3G:
                return SOLUTION_NETWORK_3G;
            case BidRequest_Device_ConnectionType_CELL_4G:
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

    } // namespace anonyous

    bool KupaiBiddingHandler::parseRequestData(const std::string & data)
    {
        bidRequest_.Clear();
        return adservice::utility::serialize::getProtoBufObject(bidRequest_, data);
    }

    bool KupaiBiddingHandler::fillSpecificLog(const AdSelectCondition & selectCondition,
                                              protocol::log::LogItem & logItem, bool isAccepted)
    {
        logItem.ipInfo.proxy = selectCondition.ip;
        if (isAccepted) {
            if (bidRequest_.device().devicetype() != BidRequest_Device_DeviceType_PERSONAL_COMPUTER) {
                logItem.deviceInfo = bidRequest_.device().DebugString();
            }
        }
        return true;
    }

    bool KupaiBiddingHandler::filter(const BiddingFilterCallback & filterCb)
    {
        if (bidRequest_.test()) {
            return bidFailedReturn();
        }

        //从BID Request中获取请求的广告位信息,目前只取第一个
        if (bidRequest_.imp().size() == 0) {
            return bidFailedReturn();
        }
        const BidRequest_Device & device = bidRequest_.device();
        const BidRequest_Imp & imp = bidRequest_.imp(0);

        std::vector<AdSelectCondition> queryConditions{ AdSelectCondition() };
        std::vector<PreSetAdplaceInfo> adplaceInfos{ PreSetAdplaceInfo() };
        AdSelectCondition & queryCondition = queryConditions[0];
        PreSetAdplaceInfo & adplaceInfo = adplaceInfos[0];
        queryCondition.adxid = ADX_KUPAI_MOBILE;
        queryCondition.adxpid = imp.tagid();
        queryCondition.ip = device.has_ip() ? device.ip() : "";
        queryCondition.basePrice = (int)imp.bidfloor();
        if (imp.has_banner()) {
            const BidRequest_Imp_Banner & banner = imp.banner();
            queryCondition.width = banner.w();
            queryCondition.height = banner.h();
        } else if (imp.has_native()) {
            const BidRequest_Imp_Native & native = imp.native();
            std::string strNativeRequest = native.request();
            cppcms::json::value nativeRequest;
            json::parseJson(strNativeRequest.c_str(), nativeRequest);
            cppcms::json::array & assets = nativeRequest["reqAssets"].array();
            queryCondition.bannerType = BANNER_TYPE_PRIMITIVE;
            if (assets.size() > 0) {
                for (uint32_t i = 0; i < assets.size(); i++) {
                    const cppcms::json::value & asset = assets[i];
                    int w = asset.get("reqImg.w", 0);
                    int h = asset.get("reqImg.h", 0);
                    int t = asset.get("type", 0);
                    auto sizePair = adplaceStyleMap.getMttySize(t, w, h);
                    queryCondition.width = sizePair.first;
                    queryCondition.height = sizePair.second;
                    adplaceInfo.sizeArray.push_back({ queryCondition.width, queryCondition.height });
                }
                queryCondition.pAdplaceInfo = &adplaceInfo;
            }
        }
        if (device.devicetype() != BidRequest_Device_DeviceType_PERSONAL_COMPUTER) {
            queryCondition.mobileDevice = getDeviceType(device.devicetype());
            queryCondition.flowType = SOLUTION_FLOWTYPE_MOBILE;
            queryCondition.adxid = ADX_KUPAI_MOBILE;
            queryCondition.mobileNetwork = getNetWork(device.connectiontype());
            queryCondition.imei = device.has_didmd5() ? device.didmd5() : "";
            queryCondition.androidId = device.has_dpidmd5() ? device.dpidmd5() : "";
            queryCondition.mac = device.has_macmd5() ? device.macmd5() : "";
        } else {
            queryCondition.pcOS = SOLUTION_OS_OTHER;
            queryCondition.flowType = SOLUTION_FLOWTYPE_PC;
            queryCondition.mac = device.has_macmd5() ? device.macmd5() : "";
        }

        if (!filterCb(this, queryConditions)) {
            adInfo.pid = std::to_string(queryCondition.mttyPid);
            adInfo.adxpid = queryCondition.adxpid;
            adInfo.adxid = queryCondition.adxid;
            adInfo.bidSize = makeBidSize(queryCondition.width, queryCondition.height);
            return bidFailedReturn();
        }

        return isBidAccepted = true;
    }

    void KupaiBiddingHandler::buildBidResult(const AdSelectCondition & queryCondition,
                                             const MT::common::SelectResult & result, int seq)
    {
        if (seq == 0) {
            bidResponse_.Clear();
            bidResponse_.set_id(bidRequest_.id());
            bidResponse_.set_bidid(adservice::utility::cypher::randomId(3));
            bidResponse_.clear_seatbid();
        }
        const MT::common::Banner & banner = result.banner;

        //缓存最终广告结果
        fillAdInfo(queryCondition, result, "");

        const BidRequest_Imp & imp = bidRequest_.imp(seq);
        float maxCpmPrice = (float)result.bidPrice;

        BidResponse_SeatBid * seatBid = bidResponse_.add_seatbid();
        BidResponse_SeatBid_Bid * adResult = seatBid->add_bid();

        adResult->set_id(bidRequest_.id());
        adResult->set_impid(imp.id());
        adResult->set_price(maxCpmPrice);
        adResult->set_crid(std::to_string(banner.bId));
        adResult->SetExtension(com::wk::adx::rtb::tagid, imp.has_tagid() ? imp.tagid() : "");
        adResult->SetExtension(com::wk::adx::rtb::creative_id, std::to_string(banner.bId));

        URLHelper showUrlParam;
        getShowPara(showUrlParam, bidRequest_.id());
        showUrlParam.add("of", "3");
        showUrlParam.addMacro("p", "%%AUCTION_PRICE%%");
        adResult->set_nurl(std::string(SNIPPET_SHOW_URL) + "?" + showUrlParam.cipherParam());

        cppcms::json::value bannerJson;
        std::stringstream ss;
        ss << banner.json; // boost::algorithm::erase_all_copy(banner.json, "\\");
        ss >> bannerJson;
        const cppcms::json::array & mtlsArray = bannerJson["mtls"].array();
        std::string landingUrl;
        cppcms::json::value admObject;
        if (banner.bannerType == BANNER_TYPE_PRIMITIVE) {
            landingUrl = mtlsArray[0]["p9"].str();
            replace(landingUrl, "{{click}}", "");
            cppcms::json::array imageUrls;
            imageUrls.push_back(mtlsArray[0]["p6"].str());
            imageUrls.push_back(mtlsArray[0]["p7"].str());
            imageUrls.push_back(mtlsArray[0]["p8"].str());
            admObject["imgurl"] = imageUrls;
            admObject["landingurl"] = landingUrl;
        } else if (banner.bannerType != BANNER_TYPE_HTML) {
            landingUrl = mtlsArray[0]["p1"].str();
            cppcms::json::array imageUrls;
            imageUrls.push_back(mtlsArray[0]["p0"].str());
            admObject["imgurl"] = imageUrls;
            admObject["landingurl"] = landingUrl;
        }
        int style = 0;
        auto & sizeStyleMap = adplaceStyleMap.getSizeStyleMap();
        auto sizeStyleListIter = sizeStyleMap.find(std::make_pair(banner.width, banner.height));
        if (sizeStyleListIter != sizeStyleMap.end() && sizeStyleListIter->second.size() > 0) {
            admObject["itemType"] = std::get<0>(sizeStyleListIter->second[0]);
        } else {
            admObject["itemType"] = style;
        }
        std::string admJson = json::toJson(admObject);
        adResult->set_adm(admJson);

        url::URLHelper clickUrlParam;
        getClickPara(clickUrlParam, bidRequest_.id(), "", landingUrl);
        adResult->AddExtension(com::wk::adx::rtb::clktrackers,
                               std::string(SNIPPET_CLICK_URL) + "?" + clickUrlParam.cipherParam());

        adResult->set_w(banner.width);
        adResult->set_h(banner.height);
    }

    void KupaiBiddingHandler::match(adservice::utility::HttpResponse & response)
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

    void KupaiBiddingHandler::reject(adservice::utility::HttpResponse & response)
    {
        bidResponse_.Clear();
        bidResponse_.set_id(bidRequest_.id());
        bidResponse_.set_bidid(adservice::utility::cypher::randomId(3));
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
