#ifndef DEFAULT_DSP_HANDLER_H__
#define DEFAULT_DSP_HANDLER_H__

#include "core/adselectv2/ad_select_interface.h"
#include "mtty/httpclient.h"
#include "protocol/dsps/mtty_bidding.pb.h"
#include "utility/utility.h"
#include <mtty/types.h>
#include <string>

namespace protocol {
namespace dsp {

    using namespace adservice;
    using namespace adservice::utility::future;

    class DSPBidResult {
    public:
        void reset()
        {
            dspId = 0;
            resultOk = false;
            bidPrice = 0.0;
            dealId = "";
            banner = MT::common::Banner();
            laterAccessUrls.clear();
        }
        int64_t dspId;
        bool resultOk{ false };
        double bidPrice{ 0.0 };
        MT::common::Banner banner;
        std::string dealId;
        std::vector<std::string> laterAccessUrls;
    };

    typedef PromiseListener DSPPromiseListener;

    typedef Promise DSPPromise;

    typedef PromisePtr DSPPromisePtr;

    class DSPHandlerInterface {
    public:
        DSPHandlerInterface(int64_t dspId,
                            const std::string & targetUrl,
                            const std::string & cookiemappingUrl,
                            int64_t timeout)
            : dspUrl_(targetUrl)
            , cookiemappingUrl_(cookiemappingUrl)
            , dspId_(dspId)
            , timeoutMs_(timeout)
        {
        }

        /**
         * 给DSP发送异步 BID 请求
         * @brief sendBid
         * @param selectCondition
         */
        virtual DSPPromisePtr sendBid(adselectv2::AdSelectCondition & selectCondition,
                                      const MT::common::ADPlace & adplace);

        virtual std::shared_ptr<MT::common::HttpRequest> buildRequest(adselectv2::AdSelectCondition & selectCondition,
                                                                      const MT::common::ADPlace & adplace);

        virtual DSPBidResult & parseResponse(std::shared_ptr<MT::common::HttpResponse> response,
                                             const MT::common::ADPlace & adplace);

        const DSPBidResult & getResult()
        {
            return dspResult_;
        }

        const std::string & getDspUrl()
        {
            return dspUrl_;
        }

        const std::string & getCookieMappingUrl()
        {
            return cookiemappingUrl_;
        }

        int64_t getDSPId()
        {
            return dspId_;
        }

        int64_t getTimeout()
        {
            return timeoutMs_;
        }

    protected:
        BidRequest conditionToBidRequest(adselectv2::AdSelectCondition & selectCondition,
                                         const MT::common::ADPlace & adplace);

        DSPBidResult bidResponseToDspResult(BidResponse & bidResponse, const MT::common::ADPlace & adplace);

    protected:
        std::string dspUrl_;
        std::string cookiemappingUrl_;
        int64_t dspId_;
        int64_t timeoutMs_;
        DSPBidResult dspResult_;
    };

    typedef DSPHandlerInterface DefaultDSPHandler;
}
}

#endif
