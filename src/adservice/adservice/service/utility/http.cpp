#include "http.h"
#include "url.h"
#include <cpprest/http_client.h>

namespace adservice {
namespace utility {

    void HttpResponse::set_body(const std::string & bodyMessage)
    {
        if (bodyMessage.length() == 0)
            return;
        body = bodyMessage;
    }

    std::string HttpResponse::get_body()
    {
        return body;
    }

    std::string HttpResponse::get_debug_message()
    {
        if (bodyStream.tellp() > 0)
            return bodyStream.str();
        else
            return "";
    }

    std::shared_ptr<HttpClientProxy> HttpClientProxy::instance_ = std::make_shared<HttpClientProxy>();

    HttpClientProxy::ResponseCallback HttpClientProxy::defaultResponseCallback_ = [](int, int, const std::string &) {};

    bool HttpClientProxy::registerClient(const std::string & host, uint32_t timeoutMs)
    {
        url::URLHelper hostUrl(host, false);
        const std::string & protocol = hostUrl.getProtocol();
        if (protocol.empty() || protocol != "http" || protocol != "https") {
            return false;
        }
        std::string hostName = hostUrl.toUrlNoParam();
        bool firstRegister = true;
        spinlock_lock(&slock_);
        auto iter = cachedClients.find(hostName);
        if (iter != cachedClients.end()) {
            firstRegister = false;
        } else {
            web::http::client::http_client_config clientConfig;
            clientConfig.set_timeout(std::chrono::milliseconds(timeoutMs));
            cachedClients.insert({ hostName, std::make_shared<web::http::client::http_client>(host, clientConfig) });
        }
        spinlock_unlock(&slock_);
        return firstRegister;
    }

    std::shared_ptr<web::http::client::http_client> HttpClientProxy::getClient(const std::string & host)
    {
        auto iter = cachedClients.find(host);
        if (iter == cachedClients.end()) {
            registerClient(host);
            iter = cachedClients.find(host);
        }
        return iter->second;
    }

    HttpClientProxy::HttpClientProxy()
    {
    }

    HttpClientProxy::~HttpClientProxy()
    {
    }

    HttpResponse HttpClientProxy::getSync(const std::string & url, uint32_t timeoutMs)
    {
        utility::url::URLHelper requestUrl(url, false);
        std::string hostName = requestUrl.toUrlParam();
        auto client = getClient(hostName);
        client->client_config().set_timeout(std::chrono::milliseconds(timeoutMs));
        web::http::http_response resp = client->request(web::http::methods::GET, requestUrl.toQuery()).get();
        HttpResponse response;
        response.set_body(resp.extract_string().get());
        auto & headers = resp.headers();
        for (auto iter = headers.begin(); iter != headers.end(); iter++) {
            response.set_header(iter->first, iter->second);
        }
        return response;
    }

    void HttpClientProxy::getAsync(const std::string & url, uint32_t timeoutMs, const ResponseCallback & callback)
    {
        utility::url::URLHelper requestUrl(url, false);
        std::string hostName = requestUrl.toUrlParam();
        auto client = getClient(hostName);
        client->client_config().set_timeout(std::chrono::milliseconds(timeoutMs));
        client->request(web::http::methods::GET, requestUrl.toQuery())
            .then([&callback](web::http::http_response response) {
                response.content_ready().then([&callback](web::http::http_response response) {
                    callback(false, response.status_code(), response.extract_string().get());
                })
            });
    }

    void HttpClientProxy::postAsync(const std::string & url, const std::string & postData,
                                    const std::string & contentType, uint32_t timeoutMs,
                                    const ResponseCallback & callback)
    {
        utility::url::URLHelper requestUrl(url, false);
        std::string hostName = requestUrl.toUrlParam();
        auto client = getClient(hostName);
        client->client_config().set_timeout(std::chrono::milliseconds(timeoutMs));
        client->request(web::http::methods::POST, requestUrl.toQuery(), postData)
            .then([&callback](web::http::http_response response) {
                response.content_ready().then([&callback](web::http::http_response response) {
                    callback(false, response.status_code(), response.extract_string().get());
                })
            });
    }
}
}
