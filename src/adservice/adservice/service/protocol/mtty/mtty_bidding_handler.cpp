//
// Created by guoze.lin on 16/5/3.
//
#include <sstream>

#include "core/core_ad_sizemap.h"
#include "core/core_ip_manager.h"
#include "core/core_typetable.h"
#include "logging.h"
#include "mtty_bidding_handler.h"
#include "utility/utility.h"

namespace protocol {
namespace bidding {

    using namespace adservice::utility;
    using namespace adservice::utility::serialize;
    using namespace adservice::utility::json;
    using namespace adservice::utility::userclient;
    using namespace adservice::utility::cypher;
    using namespace adservice::server;

    bool MttyBiddingHandler::parseRequestData(const std::string & data)
    {
        bidRequest_.Clear();
        bidResponse_.Clear();
        return getProtoBufObject(bidRequest_, data);
    }

    bool MttyBiddingHandler::fillSpecificLog(const AdSelectCondition & selectCondition,
                                             protocol::log::LogItem & logItem, bool isAccepted)
    {
        return true;
    }

    bool MttyBiddingHandler::filter(const BiddingFilterCallback & filterCb)
    {
        if (bidRequest_.imp_size() == 0) {
            return isBidAccepted = false;
        }
        std::vector<AdSelectCondition> queryConditions(bidRequest_.imp_size());
        std::vector<PreSetAdplaceInfo> adplaceInfos(bidRequest_.imp_size());
        for (uint32_t i = 0; i < bidRequest_.imp_size(); i++) {
            auto & imp = bidRequest_.imp(0);
            AdSelectCondition & queryCondition = queryConditions[i];
            PreSetAdplaceInfo & presetAdplace = adplaceInfos[i];
            queryCondition.pAdplaceInfo = &presetAdplace;
            queryCondition.mttyPid = std::stoi(imp.tagid());
            queryCondition.adxid = 0;
            queryCondition.ip = bidRequest_.ip();
            queryCondition.basePrice = imp.bidfloor();
            if (imp.has_banner()) { // banner
                auto & banner = imp.banner();
                queryCondition.width = banner.width();
                queryCondition.height = banner.height();
                presetAdplace.sizeArray.push_back({ banner.width(), banner.height() });
            } else { //原生
                auto & native = imp.native();
                AdSizeMap & adsizeMap = AdSizeMap::getInstance();
                for (uint32_t ni = 0; ni < native.assets_size(); ni++) {
                    auto & asset = native.assets(ni);
                    if (!asset.has_img()) {
                        continue;
                    }
                    auto & img = asset.mutable_img();
                    auto sizes = adsizeMap.get({ img->w(), img->h() });
                    for (auto & sizeIter : sizes) {
                        presetAdplace.sizeArray.push_back({ sizeIter.first, sizeIter.second });
                        queryCondition.width = sizeIter.first;
                        queryCondition.height = sizeIter.second;
                    }
                }
            }
            if (imp.has_pmp() && imp.pmp().deals_size() > 0) {
                auto & pmp = imp.pmp();
                std::stringstream ss;
                ss << ",";
                for (uint32_t k = 0; k < pmp.deals_size(); k++) {
                    auto & deal = pmp.deals(k);
                    if (deal.bidfloor() < queryCondition.basePrice) {
                        queryCondition.basePrice = deal.bidfloor();
                    }
                    ss << deal.dealid() << ",";
                }
                queryCondition.dealId = ss.str();
            }
            if (bidRequest_.has_user()) {
                queryCondition.mtUserId = bidRequest_.user().muid();
            }
            if (i == 0) {
                // device
                if (bidRequest_.has_device()) {
                    auto & device = bidRequest_.device();
                    queryCondition.mobileDevice = userclient::getMobileTypeFromUA(device.ua());
                    queryCondition.pcOS = userclient::getOSTypeFromUA(device.ua());
                    if (queryCondition.mobileDevice == SOLUTION_DEVICE_OTHER) {
                    }
                    if (queryCondition.pcOS == SOLUTION_OS_OTHER) {
                    }
                    queryCondition.idfa = device.idfamd5();
                    queryCondition.imei = device.imeimd5();
                }
                if (queryCondition.mobileDevice != SOLUTION_DEVICE_OTHER) {
                    queryCondition.flowType = SOLUTION_FLOWTYPE_MOBILE;
                } else {
                    queryCondition.flowType = SOLUTION_FLOWTYPE_PC;
                }
                //                if (bidRequest_.has_site()) { // wap或pc页面
                //                    auto & site = bidRequest_.site();

                //                } else if (bidRequest_.has_app()) {
                //                    auto& app = bidRequest_.app();
                //                }
            } else {
                AdSelectCondition & first = queryConditions[0];
                queryCondition.mobileDevice = first.mobileDevice;
                queryCondition.pcOS = first.pcOS;
                queryCondition.idfa = first.idfa;
                queryCondition.imei = first.imei;
                queryCondition.flowType = first.flowType;
            }
        }
        if (!filterCb(this, queryConditions)) {
            return bidFailedReturn();
        }
        return isBidAccepted = true;
    }

    void MttyBiddingHandler::buildBidResult(const AdSelectCondition & queryCondition,
                                            const MT::common::SelectResult & result, int seq)
    {
        if (!bidResponse_.has_bid()) {
            bidResponse_.Clear();
            bidResponse_.set_bid(bidRequest.bid());
            bidResponse_.clear_seatbid();
        }
        const MT::common::Solution & finalSolution = result.solution;
        const MT::common::Banner & banner = result.banner;
        fillAdInfo(queryCondition, result, bidRequest_.has_user() ? bidRequest_.user().muid() : "");
        // auto & imp = bidRequest_.imp(seq);
        auto seatBid = bidResponse_.add_seatbid();
        auto bid = seatBid->add_bid();
        bid->set_price(result.bidPrice);
        bool isIOS = queryCondition.mobileDevice == SOLUTION_DEVICE_IPHONE || queryCondition.mobileDevice
                     || SOLUTION_DEVICE_IPAD;
        std::string strBannerJson = banner.json;
        cppcms::json::value bannerJson = bannerJson2HttpsIOS(isIOS, strBannerJson, banner.bannerType);
        const cppcms::json::array & mtlsArray = bannerJson["mtls"].array();
        if (banner.bannerType == BANNER_TYPE_PRIMITIVE) { //原生
            bid->set_admtype(0);
            auto native = bid->mutable_native();
            for (int i = 6; i < 9; i++) {
                std::string imgUrl = mtlsArray[0].get(std::string("p") + std::stoi(i), "");
                if (!imgUrl.empty()) {
                    auto imgAsset = native->add_assets();
                    imgAsset->set_type(3);
                    imgAsset->set_imgurl(imgUrl);
                }
            }
            auto titleAsset = native->add_assets();
            titleAsset->set_type(1);
            titleAsset->set_data(mtlsArray[0].get("p0", ""));
        } else if (banner.bannerType == BANNER_TYPE_HTML) { // HTML 动态创意
            bid->set_admtype(1);
            bid->set_adm(mtlsArray.get("p0", ""));
        } else { //普通图片创意
            bid->set_admtype(0);
            bid->set_adm(mtlsArray.get("p0", ""));
        }
        if (!queryCondition.dealId.empty() && queryCondition.dealId != "0") {
            bid->set_dealid(finalSolution.dDealId);
        }
    }

    void MttyBiddingHandler::match(adservice::utility::HttpResponse & response)
    {
        std::string result;
        if (!writeProtoBufObject(bidResponse_, &result)) {
            LOG_ERROR << "failed to write protobuf object in MttyBiddingHandler::match";
            reject(response);
            return;
        }
        response.status(200);
        response.set_content_header("application/octet-stream");
        response.set_body(result);
    }

    void MttyBiddingHandler::reject(adservice::utility::HttpResponse & response)
    {
        bidResponse_.Clear();
        bidResponse_.set_bid(bidRequest_.bid());
        std::string result;
        if (!writeProtoBufObject(bidResponse_, &result)) {
            LOG_ERROR << "failed to write protobuf object in MttyBiddingHandler::reject";
            return;
        }
        response.status(200);
        response.set_content_header("application/octet-stream");
        response.set_body(result);
    }
}
}
