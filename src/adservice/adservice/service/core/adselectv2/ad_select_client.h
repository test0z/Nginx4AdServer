#ifndef AD_SELECT_CLIENT_H
#define AD_SELECT_CLIENT_H

#include "ad_select_interface.h"
#include "logging.h"

#include <memory>

#include <unistd.h>

#include <mtty/types.h>
#include <mtty/zmq.hpp>

namespace adservice {
namespace adselectv2 {

#define ADX_OTHER 0
#define ADSELECT_DEFAULT_TIMEOUT 15

	class AdSelectClient {
	public:
		AdSelectClient(const std::string & url)
			: serverUrl_(url)
		{
			socket_.connect(url);
		}

		bool search(int seqId, bool isSSP, AdSelectCondition & selectCondition, MT::common::SelectResult & result);

		bool getBannerById(int64_t bannerId, MT::common::Banner & banner);

	private:
		std::string serverUrl_;

		zmq::context_t context_{ 1 };
		zmq::socket_t socket_{ context_, ZMQ_REQ };
    };

    typedef std::shared_ptr<AdSelectClient> AdSelectClientPtr;
}
}

#endif
