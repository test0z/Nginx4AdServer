#include "default_dsp_handler.h"
#include "utility/utility.h"

namespace protocol {
namespace dsp {

    DSPPromisePtr DSPHandlerInterface::sendBid(adselectv2::AdSelectCondition & selectCondition,
                                               const MT::common::ADPlace & adplace)
    {
        this->dspResult_.resultOk = false;
        BidRequest request = std::move(conditionToBidRequest(selectCondition, adplace));
        std::string reqBody;
        utility::serialize::writeProtoBufObject(request, &reqBody);
        //异步回调
        // operationPromise相当于一个异步操作的回执
        DSPPromisePtr operationPromise = std::make_shared<DSPPromise>();
        auto callback = [this](const std::string & res) {
            BidResponse bidResponse;
            utility::serialize::getProtoBufObject(bidResponse, res);
            this->dspResult_ = std::move(this->bidResponseToDspResult(bidResponse));
            operationPromise->done();
        };
        //发送异步请求,确保在timeout之内一定调用callback,无论成功或失败
        return operationPromise;
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
            auto newNative = imp->mutable_native();
        } else { // banner
            auto newBanner = imp->mutable_banner();
            newBanner->set_width(adplace.width);
            newBanner->set_height(adplace.height);
        }
        return bidRequest;
    }

    DSPBidResult DSPHandlerInterface::bidResponseToDspResult(BidResponse & bidResponse)
    {
        DSPBidResult result;
        result.resultOk = true;
        result.dspId = this->dspId_;
        return result;
    }
}
}
