//
// Created by guoze.lin on 16/9/21.
//

#ifndef MTCORENGINX_HTTP_H
#define MTCORENGINX_HTTP_H

#include "core/core_executor.h"
#include "promise.h"
#include <atomic>
#include <booster/backtrace.h>
#include <curl/curl.h>
#include <ev.h>
#include <iostream>
#include <map>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>

namespace adservice {
namespace utility {

#define URI "URI"
#define QUERYSTRING "QUERY"
#define COOKIE "Cookie"
#define QUERYMETHOD "Method"
#define USERAGENT "User-Agent"
#define REMOTEADDR "X-Forwarded-For"
#define REFERER "Referer"
#define CONTENTTYPE "Content-Type"
#define CONTENTLENGTH "Content-Length"
#define SETCOOKIE "Set-Cookie"
#define CONTENTENCODING "Content-Encoding"
#define ACCEPTENCODING "Accept-Encoding"

    class HttpRequest {
    public:
        void set(const std::string & key, const std::string & value)
        {
            if (!headers.insert(std::make_pair(key, value)).second) {
                headers[key] = value;
            }
        }

        std::string get(const std::string & key) const
        {
            std::map<std::string, std::string>::const_iterator iter = headers.find(key);
            if (iter != headers.end()) {
                return iter->second;
            }
            return "";
        }

        bool isGzip()
        {
            return get(CONTENTENCODING).find("gzip") != std::string::npos;
        }

        bool isResponseNeedGzip()
        {
            return get(ACCEPTENCODING).find("gzip") != std::string::npos;
        }

        std::string path_info()
        {
            return get(URI);
        }

        std::string request_method()
        {
            return get(QUERYMETHOD);
        }

        std::string query_string()
        {
            return get(QUERYSTRING);
        }

        std::string http_cookie()
        {
            return get(COOKIE);
        }

        std::string http_user_agent()
        {
            return get(USERAGENT);
        }

        std::string remote_addr()
        {
            return get(REMOTEADDR);
        }

        std::string http_referer()
        {
            return get(REFERER);
        }

        void set_post_data(const std::string & data)
        {
            postdata = data;
        }

        const std::string & raw_post_data() const
        {
            return postdata;
        }

        std::string to_string()
        {
            std::stringstream ss;
            for (auto iter = headers.begin(); iter != headers.end(); iter++) {
                if (!iter->first.empty() && !iter->second.empty()) {
                    ss << iter->first << ":" << iter->second << "\r\n";
                }
            }
            if (postdata.length() > 0) {
                ss << "\r\n" << postdata;
            }
            return ss.str();
        }

    private:
        std::map<std::string, std::string> headers;
        std::string postdata;
    };

    class HttpResponse {
    public:
        HttpResponse()
        {
        }

        HttpResponse(const HttpResponse && other)
        {
            respStatus = other.respStatus;
            statusMsg = std::move(other.statusMsg);
            headers = std::move(other.headers);
            body = std::move(other.body);
            needGzipResponse = other.needGzipResponse;
            bodyStream << other.bodyStream.str();
        }

        void setResponseGzip(bool t)
        {
            needGzipResponse = t;
        }

        bool responseNeedGzip()
        {
            return needGzipResponse;
        }

        void set(const std::string & key, const std::string & value)
        {
            headers[key] = value;
        }

        void set_header(const std::string & key, const std::string & value)
        {
            headers[key] = value;
        }

        std::string get(const std::string & key) const
        {
            std::map<std::string, std::string>::const_iterator iter = headers.find(key);
            if (iter != headers.end()) {
                return iter->second;
            }
            return "";
        }

        void status(int code, const std::string & msg = "")
        {
            respStatus = code;
            statusMsg = msg;
        }

        int status() const
        {
            return respStatus;
        }

        const std::string & status_message() const
        {
            return statusMsg;
        }

        void set_content_header(const std::string & contenttype)
        {
            headers[CONTENTTYPE] = contenttype;
        }

        std::string content_header() const
        {
            std::map<std::string, std::string>::const_iterator iter;
            if ((iter = headers.find(CONTENTTYPE)) == headers.end())
                return "";
            return iter->second;
        }

        std::string cookies() const
        {
            std::map<std::string, std::string>::const_iterator iter;
            if ((iter = headers.find(SETCOOKIE)) == headers.end())
                return "";
            return iter->second;
        }

        void set_body(const std::string & bodyMessage);

        std::string get_body() const;

        std::string get_debug_message();

        void append_debug(const std::string & more)
        {
            bodyStream << more;
        }

        template <typename T>
        HttpResponse & operator<<(const T & t)
        {
            bodyStream << t;
            return *this;
        }

        std::string to_string()
        {
            std::stringstream ss;
            ss << "HTTP/1.1 " << respStatus << " " << statusMsg << "\r\n";
            for (auto iter = headers.begin(); iter != headers.end(); iter++) {
                if (!iter->first.empty() && !iter->second.empty()) {
                    ss << iter->first << ":" << iter->second << "\r\n";
                }
            }
            if (body.length() > 0) {
                ss << "\r\n" << body;
            }
            return ss.str();
        }

        const std::map<std::string, std::string> & get_headers() const
        {
            return headers;
        };

    private:
        int respStatus;
        std::string statusMsg;
        std::map<std::string, std::string> headers;
        std::string body;
        std::stringstream bodyStream;
        bool needGzipResponse{ false };
    };

    class HttpClientException : public booster::exception {
    public:
        HttpClientException(const std::string & what)
            : what_(what)
        {
        }

        const char * what() const noexcept override
        {
            return what_.c_str();
        }

    private:
        std::string what_;
    };

    typedef std::function<void(int, int, const std::string &)> HttpResponseCallback;

    class HttpClientCallbackPack {
    public:
        int64_t deadline{ 0 };
        std::atomic<bool> callbackCalled{ false };
        HttpResponseCallback transferFinishedCallback;
        future::PromisePtr promisePtr;

        HttpClientCallbackPack()
        {
            promisePtr = std::make_shared<future::Promise>();
        }
    };

    typedef std::shared_ptr<HttpClientCallbackPack> HttpClientCallbackPackPtr;

    class HttpClientCallbackPackPtrCMP {
    public:
        bool operator()(const HttpClientCallbackPackPtr & x, const HttpClientCallbackPackPtr & y) const
        {
            return x->deadline > y->deadline;
        }
    };

    typedef std::priority_queue<HttpClientCallbackPackPtr, std::vector<HttpClientCallbackPackPtr>,
                                HttpClientCallbackPackPtrCMP>
        TimeoutQueue;

    class HttpClientProxy {
    public:
        static std::shared_ptr<HttpClientProxy> getInstance()
        {
            return instance_;
        }

        static std::shared_ptr<HttpClientProxy> instance_;

        ~HttpClientProxy();
        HttpClientProxy();

    public:
        HttpResponse getSync(const std::string & url, uint32_t timeoutMs);

        future::PromisePtr getAsync(const std::string & url, uint32_t timeoutMS,
                                    const HttpResponseCallback & callback = nullptr);

        HttpResponse postSync(const std::string & url, const std::string & postData, const std::string & constentType,
                              uint32_t timeoutMs);

        future::PromisePtr postAsync(const std::string & url, const std::string & postData,
                                     const std::string & contentType, uint32_t timeoutMs,
                                     const HttpResponseCallback & callback = defaultResponseCallback_);

        CURLM * getCurlM()
        {
            return multiCurl_;
        }

        struct ev_loop * loop()
        {
            return loop_;
        }

        struct ev_timer * wakeupTimer()
        {
            return &wakeupTimer_;
        }

        std::mutex & taskMutex()
        {
            return asyncCurlMutex_;
        }

        std::vector<CURL *> & pendingCurls()
        {
            return pendingCurls_;
        }

        std::shared_ptr<adservice::server::Executor> getExecutor()
        {
            return executor_;
        }

        TimeoutQueue & getTimeoutQueue()
        {
            return timeoutList_;
        }

        void setVerbose(bool verbose)
        {
            verbose_ = verbose;
        }

        bool isVerbose()
        {
            return verbose_;
        }

    private:
        static HttpResponseCallback defaultResponseCallback_;
        struct ev_loop * loop_;
        struct ev_timer wakeupTimer_;
        struct ev_timer taskTimer_;
        CURLM * multiCurl_;
        int presudoFd_;
        struct ev_io presudoWatcher_;
        std::vector<CURL *> pendingCurls_;
        std::mutex asyncCurlMutex_;
        std::shared_ptr<adservice::server::Executor> executor_;
        TimeoutQueue timeoutList_;
        bool verbose_{ false };
    };
}
}

#endif // MTCORENGINX_HTTP_H
