#include "http.h"

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
}
}
