#include "ad_select_client.h"
#include "utility/utility.h"

#include <mtty/zmq.hpp>

#include <tuple>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/variant.hpp>

namespace adservice {
namespace adselectv2 {

    using namespace adservice::utility::rng;
    using namespace adservice::utility::ip;
    using namespace adservice::utility::time;

	MT::common::ListType makeSize(int64_t width, int64_t height)
    {
		MT::common::ListType list{ MT::common::TupleType{ width, height } };
        return list;
    }

	MT::common::ListType makeSize(std::vector<std::pair<int64_t, int64_t>> & sizes)
    {
		MT::common::ListType list;
        for (auto & item : sizes) {
			list.push_back(MT::common::TupleType{ item.first, item.second });
        }
        return list;
    }

	void makeRequest(bool isSSP, AdSelectCondition & selectCondition, MT::common::SelectRequest & request)
    {
		MT::common::SourceType & source = request.source;
		source.insert({ "d_adplacetype", (int64_t)selectCondition.adplaceType });
		source.insert({ "d_adexchange", (int64_t)selectCondition.adxid });
		source.insert({ "d_adplace", selectCondition.mttyPid });
		source.insert({ "n_adplace", selectCondition.mttyPid });
		source.insert({ "d_adxpid", selectCondition.adxpid });
		source.insert({ "d_dealid", selectCondition.dealId });
		source.insert({ "d_countrygeo", (int64_t)(selectCondition.dGeo - selectCondition.dGeo % AREACODE_MARGIN) });
		source.insert({ "d_geo", (int64_t)selectCondition.dGeo });
		source.insert({ "n_geo", (int64_t)selectCondition.dGeo });
		source.insert({ "d_hour", (int64_t)std::stoll(selectCondition.dHour) });
		source.insert({ "d_displaynumber", (int64_t)selectCondition.displayNumber });
		source.insert({ "d_flowtype", (int64_t)selectCondition.flowType });
		if (selectCondition.pAdplaceInfo == nullptr) {
			source.insert({ "height", (int64_t)selectCondition.height });
			source.insert({ "width", (int64_t)selectCondition.width });
			source.insert({ "sizes", makeSize(selectCondition.width, selectCondition.height) });
        } else {
			source.insert({ "sizes", makeSize(selectCondition.pAdplaceInfo->sizeArray) });
        }
		source.insert({ "d_device", (int64_t)selectCondition.mobileDevice });
		source.insert({ "d_mediatype", (int64_t)selectCondition.mediaType });
		source.insert({ "d_network", (int64_t)selectCondition.mobileNetwork });
		source.insert({ "d_ip", (int64_t)ipStringToInt(selectCondition.ip) });
		source.insert({ "d_os", (int64_t)selectCondition.pcOS });
		source.insert({ "d_contenttype", (int64_t)selectCondition.mttyContentType });
		source.insert({ "time", (int64_t)getCurrentTimeStamp() });
		source.insert({ "overflow_hour", (int64_t)(std::stoll(selectCondition.dHour) % 100) });
		request.fromSSP = isSSP;
		request.mttyPid = selectCondition.mttyPid;
		request.adxId = selectCondition.adxid;
		request.adxPid = selectCondition.adxpid;
		request.baseEcpm = selectCondition.basePrice;
    }

	std::string serialize(MT::common::SelectRequest & request)
    {
		std::ostringstream os;
		boost::archive::text_oarchive oa(os);
		oa << request;
		return os.str();
    }

	void deserialize(const std::string & in, MT::common::SelectResult & result)
    {
		try {
			std::stringstream ss;
			ss << in;
			LOG_TRACE << "inputstream:" << ss.str();
			boost::archive::text_iarchive ia(ss);
			ia >> result;
		} catch (std::exception & e) {
			LOG_ERROR << "error with deserialization," << e.what();
		}
		LOG_DEBUG << "done with response deserialization";
		return;
    }

	bool AdSelectClient::search(int seqId, bool isSSP, AdSelectCondition & selectCondition,
								MT::common::SelectResult & result)
	{
		MT::common::SelectRequest request;

		makeRequest(isSSP, selectCondition, request);
		std::string requestBin = serialize(request);

		zmq::message_t message(requestBin.length() + 1);
		uint8_t flag = (uint8_t)MT::common::MessageType::ADSELECT_REQUEST;
		memcpy(static_cast<char *>(message.data()), &flag, 1);
		memcpy(static_cast<char *>(message.data()) + 1, requestBin.c_str(), requestBin.length());

		try {
			socket_.send(message);
		} catch (zmq::error_t & e) {
			LOG_ERROR << "search请求发送失败：" << e.what();
			return false;
		}

		if (zmq::poll(pollitems_, 1, isSSP || selectCondition.adxid == ADX_NETEASE_MOBILE ? -1 : selectTimeout_.count())
			< 1) {
			LOG_WARN << "adselect timeout";
			return false;
		} else {
			if (pollitems_[0].revents & ZMQ_POLLIN) {
				zmq::message_t reply;
				try {
					socket_.recv(&reply);
				} catch (zmq::error_t & e) {
					LOG_ERROR << "接收search响应失败：" << e.what();
					return false;
				}
				std::string response((char *)reply.data() + 1, reply.size() - 1);
				deserialize(response, result);
				return (result.banner.bId > 0 && result.solution.sId > 0);
			}
		}

		return false;
	}

	bool AdSelectClient::getBannerById(int64_t bannerId, MT::common::Banner & banner)
	{
		try {
			zmq::message_t message(sizeof(bannerId) + 1);

			uint8_t flag = (uint8_t)MT::common::MessageType::GET_BANNER_REQUEST;
			memcpy(static_cast<char *>(message.data()), &flag, 1);
			memcpy(static_cast<char *>(message.data()) + 1, &bannerId, sizeof(bannerId));
			socket_.send(message);

			zmq::message_t reply;
			socket_.recv(&reply);

			std::string response((char *)reply.data() + 1, reply.size() - 1);

			std::stringstream ss;
			ss << response;

			boost::archive::text_iarchive archive(ss);
			archive >> banner;

			return true;
		} catch (zmq::error_t & e) {
			LOG_ERROR << "查询banner失败：" << e.what();
			return false;
		}
	}
}
}
