#ifndef AD_SELECT_CLIENT_H
#define AD_SELECT_CLIENT_H

#include "ad_select_interface.h"
#include <boost/functional.hpp>
#include <boost/thread/once.hpp>
#include <curl/curl.h>
#include <curl/easy.h>
#include <map>
#include <mutex>
#include <thread>
#include <memory>
#include "logging.h"

namespace adservice {
namespace adselectv2 {

    using namespace Logging;


#define ADSELECT_URL_FORMAT "http://%s/select"
#define ADSELECT_DEFAULT_CONN 48
#define MAX_CURLOBJ_SIZE 1024

    class AdSelectClient {
    public:
        static void init()
        {
            curl_global_init(CURL_GLOBAL_ALL);
        }

    public:
        AdSelectClient(const std::string& server, int adSelectTimeout = 10, bool useUnixSokcet = false,
                       const std::string& unixSocketFile = "")
        {
            // char buf[256];
            // snprintf(buf,sizeof(buf),ADSELECT_URL_FORMAT,server.data());
            serverUrl = server;
            for (int i = 0; i < MAX_CURLOBJ_SIZE; i++) {
                curl[i] = NULL;
            }
            boost::call_once(onceFlag, &AdSelectClient::init);
            for (int i = 0; i < ADSELECT_DEFAULT_CONN; i++) {
                curl[i] = newCurl();
            }
            selectTimeout = adSelectTimeout;
            bUseUnixSocket = useUnixSokcet;
            sUnixSocketFile = unixSocketFile;
            LOG_INFO << "adselect url:" << serverUrl << ",useUnixSocket:" << (bUseUnixSocket ? "true" : "false")
                     << ",unixSocketFile:" << sUnixSocketFile;
        }

        CURL * newCurl()
        {
            CURL * c = curl_easy_init();
            //            if(bUseUnixSocket){
            //                auto ret = curl_easy_setopt(c, CURLOPT_UNIX_SOCKET_PATH, sUnixSocketFile.data());
            //                if(ret!=CURLE_OK){
            //                    LOG_WARN<<"curl not support unix socket";
            //                }
            //            }
            auto ret = curl_easy_setopt(c, CURLOPT_URL, serverUrl.data());
            if (ret != CURLE_OK) {
                LOG_ERROR << "curl bind url failed";
            }
            return c;
        }

        void checkCurlSeqId(int & seqId)
        {
            if (seqId >= MAX_CURLOBJ_SIZE) {
                seqId = seqId % MAX_CURLOBJ_SIZE;
            }
            if (curl[seqId] == NULL) {
                curl[seqId] = newCurl();
            }
        }

        bool doRequest(int seqId, bool isSSP, AdSelectCondition & selectCondition, AdSelectResponse & response);

        void setSelectTimeout(int newTimeout)
        {
            selectTimeout = newTimeout;
        }

    private:
        static boost::once_flag onceFlag;

    private:
        CURL * curl[MAX_CURLOBJ_SIZE];
        std::string serverUrl;
        int selectTimeout;
        bool bUseUnixSocket;
        std::string sUnixSocketFile;
    };

    typedef std::shared_ptr<AdSelectClient> AdSelectClientPtr;
}
}

#endif
