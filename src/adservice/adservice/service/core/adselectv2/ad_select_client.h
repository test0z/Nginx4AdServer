#ifndef AD_SELECT_CLIENT_H
#define AD_SELECT_CLIENT_H

#include <memory>
#include <thread>

#include <unistd.h>

#include <mtty/requestcounter.h>
#include <mtty/types.h>
#include <mtty/zmq.h>

#include "ad_select_interface.h"
#include "logging.h"

extern MT::common::RequestCounter requestCounter;

namespace adservice {
namespace adselectv2 {

#define ADX_OTHER 0
#define ADSELECT_DEFAULT_TIMEOUT 15

    class AdSelectClient {
    public:
        AdSelectClient(const std::string & url);

        ~AdSelectClient();

        bool search(int seqId, bool isSSP, AdSelectCondition & selectCondition, MT::common::SelectResult & result);

        bool getBannerById(int64_t bannerId, MT::common::Banner & banner);

        void pushRequestCounter();

    private:
        std::string serverUrl_;

        zmq::context_t context_{ 1 };
        zmq::socket_t socket_{ context_, ZMQ_REQ };

        bool stopPushRequestCounterThread_{ false };
        std::thread pushRequestCounterThread_;
    };

    typedef std::shared_ptr<AdSelectClient> AdSelectClientPtr;
}
}

#endif
