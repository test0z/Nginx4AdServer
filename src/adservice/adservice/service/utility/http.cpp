#include "http.h"

namespace adservice {
namespace utility {

    void HttpResponse::set_body(const std::string & bodyMessage)
    {
        if (bodyMessage.length() == 0)
            return;
        bodyStream.str("");
        bodyStream << bodyMessage;
        headers[CONTENTLENGTH] = std::to_string(bodyStream.tellp());
    }

    std::string HttpResponse::get_body()
    {
        std::cerr << "bodyStream pos:" << bodyStream.tellp() << std::endl;
        if (bodyStream.tellp() > 0)
            return bodyStream.str();
        else
            return "";
    }
}
}
