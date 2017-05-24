#include "ad_select_client.h"
#include "utility/utility.h"

#include <mtty/zmq.h>

#include <chrono>
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
        MT::common::ListType list{ MT::common::TupleType{ { width, height } } };
        return list;
    }

    MT::common::ListType makeSize(std::vector<std::pair<int64_t, int64_t>> & sizes)
    {
        MT::common::ListType list;
        for (auto & item : sizes) {
            list.push_back(MT::common::TupleType{ { item.first, item.second } });
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
            request.nativeAdFlow = false;
        } else {
            source.insert({ "sizes", makeSize(selectCondition.pAdplaceInfo->sizeArray) });
            request.nativeAdFlow = selectCondition.pAdplaceInfo->isAdFlow;
        }
        source.insert({ "d_device", (int64_t)selectCondition.mobileDevice });
        source.insert({ "d_mediatype", (int64_t)selectCondition.mediaType });
        source.insert({ "d_network", (int64_t)selectCondition.mobileNetwork });
        source.insert({ "d_ip", (int64_t)ipStringToInt(selectCondition.ip) });
        source.insert({ "d_os", (int64_t)selectCondition.pcOS });
        source.insert({ "d_contenttype", (int64_t)selectCondition.mttyContentType });
        source.insert({ "time", (int64_t)getCurrentTimeStamp() });
        source.insert({ "overflow_hour", (int64_t)(std::stoll(selectCondition.dHour) % 100) });
        source.insert({ "banner_type", (int64_t)selectCondition.bannerType });
        source.insert({ "mtuser", selectCondition.mtUserId });
        source.insert({ "max_creative_level", (int64_t)selectCondition.requiredCreativeLevel });
        request.fromSSP = isSSP || selectCondition.isFromSSP;
        request.mttyPid = selectCondition.mttyPid;
        request.adxId = selectCondition.adxid;
        request.adxPid = selectCondition.adxpid;
        request.baseEcpm = selectCondition.basePrice;
    }

    namespace {

        bool filterDebugSessionRequest(uint8_t & flag, std::string & requestBin)
        {
            if (inDebugSession) {
                MT::common::SelectDebugRequest debugRequest;
                debugRequest.originMsgType = flag;
                flag = (uint8_t)MT::common::MessageType::ADSELECT_DEBUG;
                debugRequest.requestData = requestBin;
                requestBin = MT::common::serialize(debugRequest);
                return true;
            }
            return false;
        }

        bool filterDebugSessionResponse(std::string & responseBin)
        {
            if (inDebugSession) {
                MT::common::SelectDebugResponse debugResponse;
                MT::common::deserialize(responseBin, debugResponse);
                LOG_DEBUG << "adselect module debug output:\n" << debugResponse.debugMessage;
                responseBin = debugResponse.responseData;
                return true;
            }
            return false;
        }
    }

    AdSelectClient::AdSelectClient(const std::string & url)
        : serverUrl_(url)
        , pushRequestCounterThread_(std::bind(&AdSelectClient::pushRequestCounter, this))
    {
        socket_.connect(url);

        pushRequestCounterThread_.detach();
    }

    AdSelectClient::~AdSelectClient()
    {
        stopPushRequestCounterThread_ = true;
    }

    bool AdSelectClient::search(int seqId, bool isSSP, AdSelectCondition & selectCondition,
                                MT::common::SelectResult & result)
    {
        MT::common::SelectRequest request;

        makeRequest(isSSP, selectCondition, request);
        uint8_t flag = (uint8_t)MT::common::MessageType::ADSELECT_REQUEST;
        std::string requestBin = serialize(request);

        filterDebugSessionRequest(flag, requestBin);

        zmq::message_t message(requestBin.length() + 1);
        memcpy(static_cast<char *>(message.data()), &flag, 1);
        memcpy(static_cast<char *>(message.data()) + 1, requestBin.c_str(), requestBin.length());

        try {
            socket_.send(message);
        } catch (zmq::error_t & e) {
            LOG_ERROR << "search请求发送失败：" << e.what();
            if (e.num() == 88) {
                socket_.close();
                socket_ = zmq::socket_t(context_, ZMQ_REQ);
                socket_.connect(this->serverUrl_);
            }
            return false;
        } catch (std::exception & e) {
            LOG_ERROR << "search 请求发送失败，可能是未知zmq 异常，e:" << e.what();
            return false;
        }

        zmq::message_t reply;
        try {
            if (!socket_.pollAndRecv(&reply, 5000)) {
                LOG_WARN << "adselect timeout";
                socket_.close();
                socket_ = zmq::socket_t(context_, ZMQ_REQ);
                socket_.connect(this->serverUrl_);
                return false;
            }
        } catch (zmq::error_t & e) {
            LOG_ERROR << "接收search响应失败：" << e.what();
            return false;
        } catch (std::exception & e) {
            LOG_ERROR << "接收search 响应失败，可能是未知zmq 异常，e:" << e.what();
            return false;
        }
        std::string response((char *)reply.data() + 1, reply.size() - 1);
        filterDebugSessionResponse(response);

        deserialize(response, result);
        if ((!selectCondition.adxpid.empty() && result.adplace.adxId != 0 && !result.adplace.adxPId.empty())
            && (selectCondition.adxid != result.adplace.adxId || selectCondition.adxpid != result.adplace.adxPId)) {
            LOG_ERROR << "query adxid:" << selectCondition.adxid << ",result adxid:" << result.adplace.adxId
                      << ",query adxpid:" << selectCondition.adxpid << ",result adxpid:" << result.adplace.adxPId;
        }

        return (result.banner.bId > 0 && result.solution.sId > 0);
    }

    bool AdSelectClient::getBannerById(int64_t bannerId, MT::common::Banner & banner)
    {
        try {
            uint8_t flag = (uint8_t)MT::common::MessageType::GET_BANNER_REQUEST;
            std::string requestBin
                = std::string((const char *)(&bannerId), (const char *)(&bannerId) + sizeof(bannerId));
            zmq::message_t message(requestBin.length() + 1);

            filterDebugSessionRequest(flag, requestBin);

            memcpy(static_cast<char *>(message.data()), &flag, 1);
            memcpy(static_cast<char *>(message.data()) + 1, requestBin.c_str(), requestBin.length());
            socket_.send(message);

            zmq::message_t reply;
            socket_.recv(&reply);

            std::string response((char *)reply.data() + 1, reply.size() - 1);
            filterDebugSessionResponse(response);
            if (response.empty()) {
                LOG_ERROR << "getBannerById,response empty,bannerId:" << bannerId;
                return false;
            }
            std::stringstream ss;
            ss << response;

            boost::archive::text_iarchive archive(ss);
            archive >> banner;

            return true;
        } catch (zmq::error_t & e) {
            LOG_ERROR << "查询banner失败：" << e.what();
            return false;
        } catch (std::exception & e) {
            LOG_ERROR << "AdSelectClient::getBannerById error,e:" << e.what() << ",bannerId:" << bannerId;
            return false;
        }
    }

    void AdSelectClient::pushRequestCounter()
    {
        zmq::socket_t socket(context_, ZMQ_REQ);
        socket.connect(serverUrl_);

        while (!stopPushRequestCounterThread_) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
                        .count()
                    % 60
                != 0) {
                continue;
            }

            std::stringstream ss;
            boost::archive::text_oarchive archive(ss);
            archive << requestCounter;
            std::string content = ss.str();

            zmq::message_t message(content.length() + 1);
            uint8_t flag = (uint8_t)MT::common::MessageType::PUSH_REQUEST_COUNT;
            memcpy(static_cast<char *>(message.data()), &flag, 1);
            memcpy(static_cast<char *>(message.data()) + 1, content.c_str(), content.length());

            try {
                socket.send(message);

                zmq::message_t reply;
                socket.recv(&reply);
            } catch (std::exception & e) {
                LOG_ERROR << e.what();
            }
        }
    }
}
}
