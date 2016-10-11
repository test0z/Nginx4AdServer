#ifndef AD_SELECT_CLIENT_H
#define AD_SELECT_CLIENT_H

#include "ad_select_interface.h"
#include "logging.h"

#include <memory>

#include <mtty/types.h>
#include <mtty/zmq.hpp>

namespace adservice {
namespace adselectv2 {

	class AdSelectClient {
	public:
		template <class Rep, class Period>
		AdSelectClient(const std::string & url, const std::chrono::duration<Rep, Period> & timeout)
			: serverUrl_(url)
			, selectTimeout_(timeout)
		{
			socket_.connect(url);
		}

		bool doRequest(int seqId, bool isSSP, AdSelectCondition & selectCondition, MT::common::SelectResult & result);

	private:
		std::string serverUrl_;
		std::chrono::milliseconds selectTimeout_;

		zmq::context_t context_{ 1 };
		zmq::socket_t socket_{ context_, ZMQ_DEALER };
		zmq::pollitem_t pollitems_[1]{ { socket_, 0, ZMQ_POLLIN, 0 } };
    };

    typedef std::shared_ptr<AdSelectClient> AdSelectClientPtr;
}
}

#endif
