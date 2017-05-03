#include "default_dsp_handler.h"
#include "core/core_ad_sizemap.h"
#include "logging.h"
#include "utility/utility.h"
#include <boost/algorithm/string/predicate.hpp>
#include <mtty/constants.h>
#include <mtty/mtuser.h>

namespace protocol {
namespace dsp {

    namespace {

        BidRequest_Device_OSType getOSType(adselectv2::AdSelectCondition & selectCondition)
        {
            switch (selectCondition.mobileDevice) {
            case SOLUTION_DEVICE_ANDROID:
            case SOLUTION_DEVICE_ANDROIDPAD:
                return BidRequest_Device_OSType::BidRequest_Device_OSType_OS_ANDROID;
            case SOLUTION_DEVICE_IPHONE:
            case SOLUTION_DEVICE_IPAD:
                return BidRequest_Device_OSType::BidRequest_Device_OSType_OS_IOS;
            default:
                switch (selectCondition.pcOS) {
                case SOLUTION_OS_ANDROID:
                    return BidRequest_Device_OSType::BidRequest_Device_OSType_OS_ANDROID;
                case SOLUTION_OS_IOS:
                    return BidRequest_Device_OSType::BidRequest_Device_OSType_OS_IOS;
                case SOLUTION_OS_WINDOWS:
                    return BidRequest_Device_OSType::BidRequest_Device_OSType_OS_WINDOWS;
                case SOLUTION_OS_LINUX:
                    return BidRequest_Device_OSType::BidRequest_Device_OSType_OS_LINUX;
                case SOLUTION_OS_MAC:
                    return BidRequest_Device_OSType::BidRequest_Device_OSType_OS_MAC;
                default:
                    return BidRequest_Device_OSType::BidRequest_Device_OSType_OS_UNKNOWN;
                }
            }
        }

        BidRequest_Device_DeviceType getDevType(adselectv2::AdSelectCondition & selectCondition)
        {
            switch (selectCondition.mobileDevice) {
            case SOLUTION_DEVICE_ANDROID:
                return BidRequest_Device_DeviceType::BidRequest_Device_DeviceType_DEVICE_ANDROIDPHONE;
            case SOLUTION_DEVICE_ANDROIDPAD:
                return BidRequest_Device_DeviceType::BidRequest_Device_DeviceType_DEVICE_ANDROIDPAD;
            case SOLUTION_DEVICE_IPHONE:
                return BidRequest_Device_DeviceType::BidRequest_Device_DeviceType_DEVICE_IPHONE;
            case SOLUTION_DEVICE_IPAD:
                return BidRequest_Device_DeviceType::BidRequest_Device_DeviceType_DEVICE_IPAD;
            case SOLUTION_DEVICE_TV:
                return BidRequest_Device_DeviceType::BidRequest_Device_DeviceType_DEVICE_TV;
            default:
                return BidRequest_Device_DeviceType::BidRequest_Device_DeviceType_DEVICE_UNKNOWN;
            }
        }

        std::string getFormatIdByMaterialUrl(const std::string & url)
        {
            if (boost::algorithm::ends_with(url, "png")) {
                return "1";
            } else if (boost::algorithm::ends_with(url, "jpg")) {
                return "2";
            } else if (boost::algorithm::ends_with(url, "gif")) {
                return "3";
            } else if (boost::algorithm::ends_with(url, "flv")) {
                return "4";
            } else if (boost::algorithm::ends_with(url, "swf")) {
                return "5";
            } else {
                return "6";
            }
        }
    }

    DSPPromisePtr DSPHandlerInterface::sendBid(adselectv2::AdSelectCondition & selectCondition,
                                               const MT::common::ADPlace & adplace)
    {
        this->dspResult_.resultOk = false;
        BidRequest request = std::move(conditionToBidRequest(selectCondition, adplace));
        std::string reqBody;
        utility::serialize::writeProtoBufObject(request, &reqBody);
        //异步回调
        //发送异步请求,确保在timeout之内一定调用callback,无论成功或失败
        auto httpClientProxy = adservice::utility::HttpClientProxy::getInstance();
        return httpClientProxy->postAsync(
            dspUrl_, reqBody, timeoutMs_, [this, &adplace](int error, int status, const std::string & res) {
                if (error == 0) {
                    BidResponse bidResponse;
                    if (utility::serialize::getProtoBufObject(bidResponse, res)) {
                        this->dspResult_ = std::move(this->bidResponseToDspResult(bidResponse, adplace));
                    }
                }
            });
    }

    BidRequest DSPHandlerInterface::conditionToBidRequest(adselectv2::AdSelectCondition & selectCondition,
                                                          const MT::common::ADPlace & adplace)
    {
        BidRequest bidRequest;
        bidRequest.set_bid(adservice::utility::cypher::randomId(3));
        bidRequest.set_ip(selectCondition.ip);
        auto imp = bidRequest.add_imp();
        imp->set_impid("");
        imp->set_tagid(std::to_string(selectCondition.mttyPid));
        imp->set_bidfloor((float)selectCondition.basePrice);
        if (adplace.supportBanner.find(STR_BANNERTYPE_PRIMITIVE) != std::string::npos) { //原生
            auto & adSizeManager = adservice::utility::AdSizeMap::getInstance();
            auto newNative = imp->mutable_native();
            auto newAsset = newNative->add_assets();
            newAsset->mutable_title()->set_len(25);
            auto assetImg = newAsset->mutable_img();
            auto unmapSizePair = adSizeManager.rget({ adplace.width, adplace.height });
            assetImg->set_w(unmapSizePair.first);
            assetImg->set_h(unmapSizePair.second);
        } else { // banner
            auto newBanner = imp->mutable_banner();
            newBanner->set_width(adplace.width);
            newBanner->set_height(adplace.height);
        }
        if (!selectCondition.dealId.empty()) {
            std::vector<int64_t> dealIds;
            MT::common::string2vecint(selectCondition.dealId, dealIds);
            auto pmp = imp->mutable_pmp();
            for (auto dealId : dealIds) {
                auto deal = pmp->add_deals();
                deal->set_dealid(std::to_string(dealId));
                deal->set_bidfloor(adplace.basePrice);
            }
        }
        bool isIOS = selectCondition.pcOS == SOLUTION_OS_IOS;
        imp->set_secure(isIOS ? 1 : 0);
        if (adplace.flowType == 2) { // app
            auto app = bidRequest.mutable_app();
            app->set_appname(adplace.adPlaceName);
        } else { // pc or wap
            auto site = bidRequest.mutable_site();
            site->set_name(adplace.adPlaceName);
            site->set_ref(selectCondition.referer);
        }
        auto device = bidRequest.mutable_device();
        device->set_ua(selectCondition.userAgent);
        device->set_os(getOSType(selectCondition));
        device->set_devtype(getDevType(selectCondition));
        device->set_idfamd5(selectCondition.idfa);
        device->set_imeimd5(selectCondition.imei);

        auto user = bidRequest.mutable_user();
        MT::User::UserID userId(selectCondition.mtUserId, false);
        user->set_muid(userId.cipher());
        return bidRequest;
    }

    DSPBidResult DSPHandlerInterface::bidResponseToDspResult(BidResponse & bidResponse,
                                                             const MT::common::ADPlace & adplace)
    {
        DSPBidResult result;
        result.resultOk = true;
        result.dspId = this->dspId_;
        if (bidResponse.seatbid_size() > 0 && bidResponse.seatbid(0).bid_size() > 0) {
            const auto & bid = bidResponse.seatbid(0).bid(0);
            if (bid.price() <= adplace.basePrice) {
                LOG_WARN << "dsp " << this->dspId_ << " bid:" << bid.id()
                         << ",offer price < baseprice, offerprice:" << bid.price()
                         << ",baseprice:" << adplace.basePrice;
                result.resultOk = false;
                return result;
            }
            MT::common::Banner & banner = result.banner;
            cppcms::json::value bannerJson;
            bannerJson["advid"] = std::to_string(dspId_);
            // bannerJson["cid"]
            bannerJson["mtls"] = cppcms::json::array();
            cppcms::json::value mtlsInst;
            if (adplace.supportBanner.find(STR_BANNERTYPE_PRIMITIVE) != std::string::npos) {
                bannerJson["ctype"] = STR_BANNERTYPE_PRIMITIVE;
                banner.bannerType = BANNER_TYPE_PRIMITIVE;
                auto & native = bid.native();
                int64_t photoIdx = 0;
                auto & adSizeManager = adservice::utility::AdSizeMap::getInstance();
                for (int i = 0; i < native.assets_size(); i++) {
                    auto & asset = native.assets(i);
                    if (asset.type() == 1) { // title
                        mtlsInst["p0"] = asset.data();
                    } else if (asset.type() == 2) { // logo
                        mtlsInst["p15"] = asset.imgurl();
                    } else if (asset.type() == 3) { // image
                        mtlsInst[std::string("p") + std::to_string(6 + photoIdx++)] = asset.imgurl();
                        int w = asset.imgw();
                        int h = asset.imgh();
                        auto sizes = adSizeManager.get({ w, h });
                        for (auto sizeIter : sizes) {
                            mtlsInst["p2"] = std::to_string(sizeIter.first);
                            break;
                        }
                        bannerJson["formatid"] = getFormatIdByMaterialUrl(asset.imgurl());
                    } else if (asset.type() == 4) { // description
                        mtlsInst["p5"] = asset.data();
                    }
                }
                if (mtlsInst["p2"] != "3" && photoIdx == 3) {
                    mtlsInst["p2"] = "3";
                }
                mtlsInst["p9"] = bid.clickm();
            } else {
                bannerJson["ctype"] = adplace.supportBanner;
                if (bid.admtype() == 0) { //静态创意
                    mtlsInst["p0"] = bid.adm();
                    mtlsInst["p1"] = bid.clickm();
                    mtlsInst["p3"] = std::to_string(adplace.width);
                    mtlsInst["p4"] = std::to_string(adplace.height);
                    if (adplace.flowType == 0) {
                        banner.bannerType = BANNER_TYPE_PIC;
                    } else {
                        banner.bannerType = BANNER_TYPE_MOBILE;
                    }
                    bannerJson["formatid"] = getFormatIdByMaterialUrl(bid.adm());
                } else { //动态创意
                    bannerJson["formatid"] = "6";
                    bannerJson["p0"] = bid.adm();
                    banner.bannerType = BANNER_TYPE_HTML;
                }
            }
            bannerJson["mtls"].array().push_back(mtlsInst);
            result.dealId = bid.dealid();
            if (!bid.nurl().empty()) {
                result.laterAccessUrls.push_back(bid.nurl());
            }
            cppcms::json::array tviews;
            for (uint32_t i = 0; bid.pvm_size(); i++) {
                const std::string & url = bid.pvm(i);
                if (!url.empty()) {
                    tviews.push_back(url);
                }
            }
            bannerJson["tviews"] = tviews;
            banner.json = adservice::utility::json::toJson(bannerJson);
        } else {
            result.resultOk = false;
        }
        return result;
    }
}
}
