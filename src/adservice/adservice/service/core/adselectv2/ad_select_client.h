#ifndef AD_SELECT_CLIENT_H
#define AD_SELECT_CLIENT_H

#include "ad_select_interface.h"
#include "logging.h"

#include <memory>

#include <mtty/types.h>
#include <mtty/zmq.hpp>

namespace adservice {
namespace adselectv2 {

#define ADX_OTHER		0
#define ADSELECT_DEFAULT_TIMEOUT 15

	class AdSelectClient {
	public:

		AdSelectClient(const std::string & url, std::map<int,int>& timeoutMap)
			: serverUrl_(url)
		{
			socket_.connect(url);
			pollitems_[0] = { socket_, 0, ZMQ_POLLIN, 0 };
			for(auto item : timeoutMap){
				selectTimeouts_.insert(std::make_pair(item.first,std::chrono::milliseconds(item.second)));
			}
			if(selectTimeouts_.find(ADX_OTHER)==selectTimeouts_.end()){
				selectTimeouts_.insert(std::make_pair(ADX_OTHER,std::chrono::milliseconds(ADSELECT_DEFAULT_TIMEOUT)));
			}
		}

		bool search(int seqId, bool isSSP, AdSelectCondition & selectCondition, MT::common::SelectResult & result);

		bool getBannerById(int64_t bannerId, MT::common::Banner & banner);

	private:
		std::string serverUrl_;
		std::map<int,std::chrono::milliseconds> selectTimeouts_;

		zmq::context_t context_{ 1 };
		zmq::socket_t socket_{ context_, ZMQ_DEALER };
		zmq::pollitem_t pollitems_[1];
    };

    typedef std::shared_ptr<AdSelectClient> AdSelectClientPtr;
}
}

#endif
