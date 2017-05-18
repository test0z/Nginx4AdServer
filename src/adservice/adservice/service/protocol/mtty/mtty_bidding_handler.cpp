//
// Created by guoze.lin on 16/5/3.
//
#include <sstream>

#include "core/core_ad_sizemap.h"
#include "core/core_ip_manager.h"
#include "core/core_typetable.h"
#include "logging.h"
#include "mtty/utils.h"
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
        return false;
    }

    bool MttyBiddingHandler::filter(const BiddingFilterCallback & filterCb)
    {
        if (bidRequest_.imp_size() == 0) {
            return isBidAccepted = false;
        }
        std::vector<AdSelectCondition> queryConditions(bidRequest_.imp_size());
        std::vector<PreSetAdplaceInfo> adplaceInfos(bidRequest_.imp_size());
        for (int i = 0; i < bidRequest_.imp_size(); i++) {
            auto & imp = bidRequest_.imp(i);
            AdSelectCondition & queryCondition = queryConditions[i];
            PreSetAdplaceInfo & presetAdplace = adplaceInfos[i];
            queryCondition.pAdplaceInfo = &presetAdplace;
            queryCondition.mttyPid = std::stoi(imp.tagid());
            queryCondition.adxid = ADX_SSP_MOBILE;
            queryCondition.ip = bidRequest_.ip();
            queryCondition.basePrice = imp.bidfloor();
            if (imp.has_banner()) { // banner
                auto & banner = imp.banner();
                queryCondition.width = banner.width();
                queryCondition.height = banner.height();
                presetAdplace.sizeArray.push_back({ banner.width(), banner.height() });
            } else { //原生
                auto & native = imp.native();
                const AdSizeMap & adsizeMap = AdSizeMap::getInstance();
                for (int ni = 0; ni < native.assets_size(); ni++) {
                    auto & asset = native.assets(ni);
                    if (!asset.has_img()) {
                        continue;
                    }
                    auto & img = asset.img();
                    auto sizes = adsizeMap.get({ img.w(), img.h() });
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
                for (int k = 0; k < pmp.deals_size(); k++) {
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
                if (queryCondition.flowType == SOLUTION_FLOWTYPE_MOBILE) {
                    queryCondition.adxid = ADX_SSP_MOBILE;
                } else {
                    queryCondition.adxid = ADX_SSP_PC;
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
                queryCondition.adxid = first.adxid;
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
        if (!bidResponse_.has_bidid()) {
            bidResponse_.Clear();
            bidResponse_.set_id(bidRequest_.bid());
            bidResponse_.set_bidid(bidRequest_.bid());
            bidResponse_.clear_seatbid();
        }
        // const MT::common::Solution & finalSolution = result.solution;
        const MT::common::Banner & banner = result.banner;
        fillAdInfo(queryCondition, result, bidRequest_.has_user() ? bidRequest_.user().muid() : "");
        auto & imp = bidRequest_.imp(seq);
        auto seatBid = bidResponse_.add_seatbid();
        auto bid = seatBid->add_bid();
        bid->set_id(adservice::utility::cypher::randomId(3));
        bid->set_impid(imp.impid());
        bid->set_price(result.bidPrice);
        bool isIOS = queryCondition.mobileDevice == SOLUTION_DEVICE_IPHONE
                     || queryCondition.mobileDevice == SOLUTION_DEVICE_IPAD;
        std::string strBannerJson = banner.json;
        cppcms::json::value bannerJson = bannerJson2HttpsIOS(isIOS, strBannerJson, banner.bannerType);
        const cppcms::json::array & mtlsArray = bannerJson["mtls"].array();
        std::string landingUrl;
        if (banner.bannerType == BANNER_TYPE_PRIMITIVE) { //原生
            landingUrl = mtlsArray[0].get("p9", "");
            bid->set_admtype(0);
            auto native = bid->mutable_native();
            const AdSizeMap & adsizeMap = AdSizeMap::getInstance();
            for (int i = 6; i < 9; i++) {
                std::string imgUrl = mtlsArray[0].get(std::string("p") + std::to_string(i), "");
                if (!imgUrl.empty()) {
                    auto imgAsset = native->add_assets();
                    imgAsset->set_type(3);
                    imgAsset->set_imgurl(imgUrl);
                    auto realSize = adsizeMap.rget({ banner.width, banner.height });
                    imgAsset->set_imgw(realSize.first);
                    imgAsset->set_imgh(realSize.second);
                    bid->set_adm(imgUrl);
                    bid->set_admtype(0);
                }
            }
            auto titleAsset = native->add_assets();
            titleAsset->set_type(1);
            titleAsset->set_data(mtlsArray[0].get("p0", ""));
        } else if (banner.bannerType == BANNER_TYPE_HTML) { // HTML 动态创意
            bid->set_admtype(1);
            bid->set_adm(mtlsArray[0].get("p0", ""));
            landingUrl = mtlsArray[0].get("p1", "");
        } else { //普通图片创意
            bid->set_admtype(0);
            bid->set_adm(mtlsArray[0].get("p0", ""));
            landingUrl = mtlsArray[0].get("p1", "");
        }
        if (!queryCondition.dealId.empty() && queryCondition.dealId != "0") {
            std::vector<int64_t> dealIds;
            MT::common::string2vecint(queryCondition.dealId, dealIds);
            bid->set_dealid(std::to_string(dealIds[0]));
        }
        url::URLHelper showUrl(getShowBaseUrl(isIOS), false);
        getShowPara(showUrl, bidRequest_.bid());
        showUrl.add(URL_IMP_OF, "3");
        showUrl.addMacro(URL_EXCHANGE_PRICE, "%%WINNING_PRICE%%");
        bid->set_nurl(showUrl.cipherUrl());
        url::URLHelper clickUrl(getClickBaseUrl(isIOS), false);
        getClickPara(clickUrl, bidRequest_.bid(), "", landingUrl);
        bid->set_clickm(clickUrl.cipherUrl());
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
        bidResponse_.set_id(bidRequest_.bid());
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
