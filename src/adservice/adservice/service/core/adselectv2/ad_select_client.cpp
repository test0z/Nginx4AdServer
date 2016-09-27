#include "ad_select_client.h"
#include "utility/utility.h"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

namespace adservice {
namespace adselectv2 {

    using namespace adservice::utility::rng;
    using namespace adservice::utility::ip;
    using namespace adservice::utility::time;

    boost::once_flag AdSelectClient::onceFlag = BOOST_ONCE_INIT;

    struct curl_read_param {
        char buffer[20480];
        int cur;
        curl_read_param()
        {
            cur = 0;
        };
    };

    static size_t curl_read_cb(void * data, size_t s, size_t nmemb, void * param)
    {
        curl_read_param & p = *(curl_read_param *)param;
        size_t dataSize = s * nmemb;
        size_t realDataSize = dataSize;
        if (realDataSize > sizeof(p.buffer) - p.cur - 1) {
            realDataSize = sizeof(p.buffer) - p.cur - 1;
        }
        memcpy(p.buffer + p.cur, data, realDataSize);
        p.cur += realDataSize;
        p.buffer[p.cur] = '\0';
        return dataSize;
    }

    ListType makeSize(int width, int height)
    {
        ListType list{ TupleType{ (int64_t)width, (int64_t)height } };
        return list;
    }

    ListType makeSize(std::vector<std::tuple<int, int>> & sizes)
    {
        ListType list;
        for (auto & item : sizes) {
            list.push_back(TupleType{ (int64_t)std::get<0>(item), (int64_t)std::get<1>(item) });
        }
        return list;
    }

    void makeRequest(bool isSSP, AdSelectCondition & selectCondition, AdSelectRequest & request)
    {
        Source & source = request.source;
        source.insert(std::make_pair("d_adplacetype", (int64_t)selectCondition.adplaceType));
        source.insert(std::make_pair("d_adexchange", (int64_t)selectCondition.adxid));
        source.insert(std::make_pair("d_adplace", selectCondition.mttyPid));
        source.insert(std::make_pair("n_adplace", selectCondition.mttyPid));
        source.insert(std::make_pair("d_adxpid", selectCondition.adxpid));
        source.insert(std::make_pair("d_dealid", selectCondition.dealId));
        source.insert(
            std::make_pair("d_countrygeo", (int64_t)(selectCondition.dGeo - selectCondition.dGeo % AREACODE_MARGIN)));
        source.insert(std::make_pair("d_geo", (int64_t)selectCondition.dGeo));
        source.insert(std::make_pair("n_geo", (int64_t)selectCondition.dGeo));
        source.insert(std::make_pair("d_hour", (int64_t)std::stoll(selectCondition.dHour)));
        source.insert(std::make_pair("d_displaynumber", (int64_t)selectCondition.displayNumber));
        source.insert(std::make_pair("d_flowtype", (int64_t)selectCondition.flowType));
        if (selectCondition.pAdplaceInfo == NULL) {
            source.insert(std::make_pair("height", (int64_t)selectCondition.height));
            source.insert(std::make_pair("width", (int64_t)selectCondition.width));
            source.insert(std::make_pair("sizes", makeSize(selectCondition.width, selectCondition.height)));
        } else {
            source.insert(std::make_pair("sizes", makeSize(selectCondition.pAdplaceInfo->sizeArray)));
        }
        source.insert(std::make_pair("d_device", (int64_t)selectCondition.mobileDevice));
        source.insert(std::make_pair("d_mediatype", (int64_t)selectCondition.mediaType));
        source.insert(std::make_pair("d_network", (int64_t)selectCondition.mobileNetwork));
        source.insert(std::make_pair("d_ip", (int64_t)ipStringToInt(selectCondition.ip)));
        source.insert(std::make_pair("d_os", (int64_t)selectCondition.pcOS));
        source.insert(std::make_pair("d_contenttype", (int64_t)selectCondition.mttyContentType));
        source.insert(std::make_pair("time", (int64_t)getCurrentTimeStamp()));
        source.insert(std::make_pair("overflow_hour", (int64_t)(std::stoll(selectCondition.dHour) % 100)));
        LogicParam & logicParam = request.param;
        logicParam.fromSSP = isSSP;
        logicParam.mttyPid = selectCondition.mttyPid;
        logicParam.adxId = selectCondition.adxid;
        logicParam.adxPid = selectCondition.adxpid;
    }

    std::string serialize(AdSelectRequest & request)
    {
        std::ostringstream os;
        boost::archive::text_oarchive oa(os);
        oa << request;
        return os.str();
    }

    void deserialize(const std::string & in, AdSelectResponse & response)
    {
        try {
            std::stringstream ss;
            ss << in;
            LOG_TRACE << "inputstream:" << ss.str();
            boost::archive::text_iarchive ia(ss);
            ia >> response;
        } catch (std::exception & e) {
            LOG_ERROR << "error with deserialization," << e.what();
        }
        LOG_DEBUG << "done with response deserialization";
        return;
    }

    template <typename Iterator>
    void urlencode_impl(char const * b, char const * e, Iterator out)
    {
        while (b != e) {
            char c = *b++;
            if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9')) {
                *out++ = c;
            } else {
                switch (c) {
                case '-':
                case '_':
                case '.':
                case '~':
                    *out++ = c;
                    break;
                default: {
                    static char const hex[] = "0123456789abcdef";
                    unsigned char uc = c;
                    *out++ = '%';
                    *out++ = hex[(uc >> 4) & 0xF];
                    *out++ = hex[uc & 0xF];
                }
                };
            }
        };
    }

    void urlencode(char const * b, char const * e, std::ostream & out)
    {
        std::ostream_iterator<char> it(out);
        urlencode_impl(b, e, it);
    }
    std::string urlencode(std::string const & s)
    {
        std::string content;
        content.reserve(3 * s.size());
        std::back_insert_iterator<std::string> out(content);
        urlencode_impl(s.c_str(), s.c_str() + s.size(), out);
        return content;
    }

    bool AdSelectClient::doRequest(int seqId, bool isSSP, AdSelectCondition & selectCondition,
                                   AdSelectResponse & response)
    {
        AdSelectRequest request;
        char buf[1024] = { '\0' };
        strcat(buf, "data=");
        makeRequest(isSSP, selectCondition, request);
        std::string requestBin = serialize(request);
        std::string urlencodeBin = urlencode(requestBin);
        memcpy(buf + 5, urlencodeBin.data(), urlencodeBin.size());
        buf[urlencodeBin.size() + 5] = '\0';
        checkCurlSeqId(seqId);
        curl_read_param param;
        curl_easy_setopt(curl[seqId], CURLOPT_WRITEFUNCTION, curl_read_cb);
        curl_easy_setopt(curl[seqId], CURLOPT_WRITEDATA, &param);
        curl_easy_setopt(curl[seqId], CURLOPT_POSTFIELDS, buf);
        curl_easy_setopt(curl[seqId], CURLOPT_TIMEOUT_MS, selectTimeout);
        auto ret = curl_easy_perform(curl[seqId]);
        if (ret == CURLE_OK && param.cur > 0) {
            int64_t httpCode;
            curl_easy_getinfo(curl[seqId], CURLINFO_RESPONSE_CODE, &httpCode);
            if (httpCode != 200) {
                LOG_DEBUG << "adselect server return code:" << httpCode;
                return false;
            }
            deserialize(param.buffer, response);
            AdSelectResult & result = response.result;
            if (result.solution.isValid() && result.banner.isValid()) {
                return true;
            } else {
                LOG_DEBUG << "response solution or banner not acceptable,bid failed";
                return false;
            }
        } else {
            if (ret == CURLE_OPERATION_TIMEDOUT) {
                LOG_WARN << "adselect timeout";
            } else if (ret == CURLE_COULDNT_CONNECT) {
                LOG_WARN << "adselect down";
            }
            return false;
        }
    }
}
}
