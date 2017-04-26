//
// Created by guoze.lin on 16/9/21.
//

#ifndef MTCORENGINX_HTTP_H
#define MTCORENGINX_HTTP_H

#include <iostream>
#include <map>
#include <sstream>
#include <string>

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

        std::string get_body();

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

    class HttpClientProxy {
    public:
        static std::shared_ptr<HttpClientProxy> getInstance()
        {
            return instance_;
        }

    private:
        static std::shared_ptr<HttpClientProxy> instance_{ nullptr };

    private:
        HttpClientProxy()
        {
        }

    public:
        typedef std::function<void(int, int, const std::string &)> ResponseCallback;

        HttpResponse getSync(const std::string & url, uint32_t timeoutMs);

        void getAsync(const std::string & url, uint32_t timeoutMS,
                      const ResponseCallback & callback = defaultResponseCallback_);

        HttpResponse postSync(const std::string & url, uint32_t timeoutMs, const std::string & postData);

        void postAsync(const std::string & url, uint32_t timeoutMs,
                       const ResponseCallback & callback = defaultResponseCallback_);

    public:
        static ResponseCallback defaultResponseCallback_;
    };
}
}

#endif // MTCORENGINX_HTTP_H
